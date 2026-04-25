#!/usr/bin/env python3
import requests
import time
import subprocess
import sys
import os
import csv
import statistics
import signal
import argparse

GATEWAY_PORT = 8080
C_RUNTIMES = ["process", "kvm", "docker"]
PYTHON_RUNTIMES = ["python", "kvm", "docker"]
C_SAMPLES = ["c/hello", "c/prime", "c/weather", "c/weather_fmt", "c/json_builder", "c/alloc_stress", "c/multi_req"]
PYTHON_SAMPLES = ["python/hello", "python/prime", "python/weather", "python/weather_fmt", "python/alloc_stress", "python/multi_req"]

# Samples that work under Junction (no networking)
JUNCTION_C_SAMPLES = ["c/hello", "c/json_builder", "c/alloc_stress"]
JUNCTION_PYTHON_SAMPLES = ["python/hello", "python/alloc_stress"]

class BenchmarkResult:
    def __init__(self, name, cold_start, exec_time, e2e_latency):
        self.name = name
        self.cold_start = cold_start
        self.exec_time = exec_time
        self.e2e_latency = e2e_latency

def _python3_size():
    """Return the size of the python3 interpreter binary, cached."""
    if not hasattr(_python3_size, "_val"):
        result = subprocess.run(["which", "python3"], capture_output=True, text=True)
        if result.returncode == 0:
            _python3_size._val = os.path.getsize(os.path.realpath(result.stdout.strip()))
        else:
            _python3_size._val = 0
    return _python3_size._val

def get_binary_size(runtime, sample):
    """Return binary/image size in bytes for a given runtime and sample, or None."""
    try:
        if runtime in ("process", "junction", "python"):
            if sample.startswith("python/"):
                runner = os.path.getsize("samples/python/runner.py")
                return _python3_size() + runner + os.path.getsize(f"samples/{sample}.py")
            else:
                return os.path.getsize(f"samples/{sample}_proc")
        elif runtime == "kvm":
            return os.path.getsize(f"samples/{sample}.elf")
        elif runtime == "docker":
            name = f"iai_{sample.replace('/', '_')}"
            result = subprocess.run(
                ["docker", "inspect", "--format", "{{.Size}}", name],
                capture_output=True, text=True)
            if result.returncode == 0:
                return int(result.stdout.strip())
    except (OSError, ValueError):
        pass
    return None

def format_size(size_bytes):
    """Format bytes into human-readable string."""
    if size_bytes is None:
        return "N/A"
    if size_bytes < 1024:
        return f"{size_bytes} B"
    elif size_bytes < 1024 * 1024:
        return f"{size_bytes / 1024:.1f} KB"
    else:
        return f"{size_bytes / (1024 * 1024):.1f} MB"

def start_gateway(runtime, port, junction_build=None):
    print(f"[*] Starting Gateway with runtime: {runtime}")
    # Run 'go build' first to ensure we aren't measuring compilation time
    subprocess.run(["go", "build", "-o", "gateway_bin", "main.go"], cwd="gateway", check=True)
    
    cmd = ["./gateway_bin", f"-runtime={runtime}", f"-port={port}"]
    if junction_build:
        cmd.append(f"-junction-build={os.path.abspath(junction_build)}")
    process = subprocess.Popen(cmd, cwd="gateway", stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    
    # Wait for gateway to be ready
    for _ in range(20):
        try:
            requests.get(GATEWAY_URL)
            return process
        except requests.exceptions.ConnectionError:
            if process.poll() is not None:
                out, err = process.communicate()
                print(f"Error: Gateway exited immediately:\n{err}")
                sys.exit(1)
            time.sleep(0.5)
    
    print("Error: Gateway failed to start.")
    process.terminate()
    sys.exit(1)

def run_benchmark(sample, num_requests, gateway_port):
    results = []
    print(f"    - Benchmarking /{sample} ({num_requests} requests)...", end="", flush=True)
    
    # Determine if POST is needed
    is_prime = sample.endswith("/prime")
    is_weather_fmt = sample.endswith("/weather_fmt")
    
    # Warm up request
    try:
        if is_prime:
            requests.post(f"{GATEWAY_URL}/{sample}", data="10000", timeout=10)
        elif is_weather_fmt:
            requests.post(f"{GATEWAY_URL}/{sample}", data=str(gateway_port), timeout=10)
        else:
            requests.get(f"{GATEWAY_URL}/{sample}", timeout=10)
    except:
        pass

    for _ in range(num_requests):
        try:
            t0 = time.time()
            if is_prime:
                resp = requests.post(f"{GATEWAY_URL}/{sample}", data="10000", timeout=10)
            elif is_weather_fmt:
                resp = requests.post(f"{GATEWAY_URL}/{sample}", data=str(gateway_port), timeout=10)
            else:
                resp = requests.get(f"{GATEWAY_URL}/{sample}", timeout=10)
            client_e2e = (time.time() - t0) * 1000 # ms
            
            if resp.status_code == 200:
                cold = float(resp.headers.get("X-Cold-Start", 0))
                exec_t = float(resp.headers.get("X-Exec-Time", 0))
                
                if _ == 0: # Print first request headers for debug
                    pass 

                results.append(BenchmarkResult(sample, cold, exec_t, client_e2e))
            else:
                print(f"\n      [!] Warning: Request to /{sample} failed with {resp.status_code}")
        except Exception as e:
            print(f"\n      [!] Error during benchmark: {e}")
            break
    
    print(" Done.")
    return results

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("num_requests", nargs="?", type=int, default=30)
    parser.add_argument("--port", type=int, default=GATEWAY_PORT)
    parser.add_argument("--junction", metavar="BUILD_PATH", help="Run Junction benchmark using the given build path")
    args = parser.parse_args()

    global GATEWAY_URL
    GATEWAY_URL = f"http://localhost:{args.port}"
    num_req = args.num_requests

    # Ensure everything is built
    print("[*] Preparing binaries and Docker images...")
    build_proc = subprocess.run(["make", "-C", "samples", "proc", "docker-all"])
    if build_proc.returncode != 0:
        print("Error: Build failed. Please check the output above.")
        sys.exit(1)

    final_report = {}

    for rt in C_RUNTIMES:
        gw_proc = start_gateway(rt, args.port)
        final_report[rt] = {}
        for sample in C_SAMPLES:
            results = run_benchmark(sample, num_req, args.port)
            final_report[rt][sample] = results
        gw_proc.terminate()
        try:
            gw_proc.wait(timeout=5)
        except:
            gw_proc.kill()
        time.sleep(1)

    for rt in PYTHON_RUNTIMES:
        gw_proc = start_gateway(rt, args.port)
        final_report.setdefault(rt, {})
        for sample in PYTHON_SAMPLES:
            results = run_benchmark(sample, num_req, args.port)
            final_report[rt][sample] = results
        gw_proc.terminate()
        try:
            gw_proc.wait(timeout=5)
        except:
            gw_proc.kill()
        time.sleep(1)

    if args.junction:
        gw_proc = start_gateway("junction", args.port, junction_build=args.junction)
        final_report["junction"] = {}
        for sample in JUNCTION_C_SAMPLES + JUNCTION_PYTHON_SAMPLES:
            results = run_benchmark(sample, num_req, args.port)
            final_report["junction"][sample] = results
        gw_proc.terminate()
        try:
            gw_proc.wait(timeout=5)
        except:
            gw_proc.kill()
        time.sleep(1)

    # Generate Comparative Table
    print("\n" + "="*70)
    print("                 IAI SERVERLESS BENCHMARK REPORT")
    print("="*70)

    print("\n--- C FUNCTIONS ---")
    c_runtimes_display = C_RUNTIMES + (["junction"] if args.junction else [])
    for sample in C_SAMPLES:
        print(f"\nFUNCTION: /{sample}")
        print(f"Runtime   | Cold Start (Avg) | Execution (Avg) | E2E Client (Avg) | Binary Size")
        print(f"----------|------------------|-----------------|------------------|------------")
        for rt in c_runtimes_display:
            res = final_report.get(rt, {}).get(sample, [])
            size = format_size(get_binary_size(rt, sample))
            if res:
                avg_cold = statistics.mean([r.cold_start for r in res])
                avg_exec = statistics.mean([r.exec_time for r in res])
                avg_e2e  = statistics.mean([r.e2e_latency for r in res])
                print(f"{rt:9} | {avg_cold:13.3f} ms | {avg_exec:12.3f} ms | {avg_e2e:14.3f} ms | {size}")
            elif rt == "junction" and sample not in JUNCTION_C_SAMPLES:
                print(f"{rt:9} | {'SKIPPED':>16} | {'SKIPPED':>15} | {'SKIPPED':>16} | {size}")
            else:
                print(f"{rt:9} | {'FAILED':>16} | {'FAILED':>15} | {'FAILED':>16} | {size}")

    print("\n--- PYTHON FUNCTIONS ---")
    py_runtimes_display = PYTHON_RUNTIMES + (["junction"] if args.junction else [])
    for sample in PYTHON_SAMPLES:
        print(f"\nFUNCTION: /{sample}")
        print(f"Runtime   | Cold Start (Avg) | Execution (Avg) | E2E Client (Avg) | Binary Size")
        print(f"----------|------------------|-----------------|------------------|------------")
        for rt in py_runtimes_display:
            res = final_report.get(rt, {}).get(sample, [])
            size = format_size(get_binary_size(rt, sample))
            if res:
                avg_cold = statistics.mean([r.cold_start for r in res])
                avg_exec = statistics.mean([r.exec_time for r in res])
                avg_e2e  = statistics.mean([r.e2e_latency for r in res])
                print(f"{rt:9} | {avg_cold:13.3f} ms | {avg_exec:12.3f} ms | {avg_e2e:14.3f} ms | {size}")
            elif rt == "junction" and sample not in JUNCTION_PYTHON_SAMPLES:
                print(f"{rt:9} | {'SKIPPED':>16} | {'SKIPPED':>15} | {'SKIPPED':>16} | {size}")
            else:
                print(f"{rt:9} | {'FAILED':>16} | {'FAILED':>15} | {'FAILED':>16} | {size}")

    # Export CSV
    csv_path = "scripts/benchmark_result.csv"
    with open(csv_path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["function", "runtime", "cold_start_ms", "exec_time_ms", "e2e_ms", "binary_size_bytes"])
        all_samples = [(s, c_runtimes_display) for s in C_SAMPLES] + [(s, py_runtimes_display) for s in PYTHON_SAMPLES]
        for sample, runtimes in all_samples:
            for rt in runtimes:
                res = final_report.get(rt, {}).get(sample, [])
                size = get_binary_size(rt, sample)
                if res:
                    w.writerow([sample, rt,
                        f"{statistics.mean([r.cold_start for r in res]):.3f}",
                        f"{statistics.mean([r.exec_time for r in res]):.3f}",
                        f"{statistics.mean([r.e2e_latency for r in res]):.3f}",
                        size or ""])
                else:
                    w.writerow([sample, rt, "", "", "", size or ""])
    print(f"\n[*] Results saved to {csv_path}")

if __name__ == "__main__":
    main()
