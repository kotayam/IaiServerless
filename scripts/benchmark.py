#!/usr/bin/env python3
import requests
import time
import subprocess
import sys
import os
import statistics
import signal
import argparse

GATEWAY_PORT = 8080
RUNTIMES = ["process", "kvm", "docker"]
SAMPLES = ["c/hello", "c/prime", "c/net_query", "c/weather", "c/json_builder"]

class BenchmarkResult:
    def __init__(self, name, cold_start, exec_time, e2e_latency):
        self.name = name
        self.cold_start = cold_start
        self.exec_time = exec_time
        self.e2e_latency = e2e_latency

def start_gateway(runtime, port):
    print(f"[*] Starting Gateway with runtime: {runtime}")
    # Run 'go build' first to ensure we aren't measuring compilation time
    subprocess.run(["go", "build", "-o", "gateway_bin", "main.go"], cwd="gateway", check=True)
    
    cmd = ["./gateway_bin", f"-runtime={runtime}", f"-port={port}"]
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

def run_benchmark(sample, num_requests):
    results = []
    print(f"    - Benchmarking /{sample} ({num_requests} requests)...", end="", flush=True)
    
    # Warm up request
    try:
        requests.get(f"{GATEWAY_URL}/{sample}")
    except:
        pass

    for _ in range(num_requests):
        try:
            t0 = time.time()
            resp = requests.get(f"{GATEWAY_URL}/{sample}")
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
    args = parser.parse_args()

    global GATEWAY_URL
    GATEWAY_URL = f"http://localhost:{args.port}"
    num_req = args.num_requests

    # Ensure everything is built
    print("[*] Preparing binaries and Docker images...")
    # Fix: Redirect build output to stderr so we see errors if they happen
    build_proc = subprocess.run(["make", "-C", "samples", "proc", "docker-all"])
    if build_proc.returncode != 0:
        print("Error: Build failed. Please check the output above.")
        sys.exit(1)
    
    final_report = {}

    for rt in RUNTIMES:
        gw_proc = start_gateway(rt, args.port)
        final_report[rt] = {}
        
        for sample in SAMPLES:
            results = run_benchmark(sample, num_req)
            final_report[rt][sample] = results
            
        # Stop gateway
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
    
    for sample in SAMPLES:
        print(f"\nFUNCTION: /{sample}")
        print(f"Runtime   | Cold Start (Avg) | Execution (Avg) | E2E Client (Avg)")
        print(f"----------|------------------|-----------------|------------------")
        for rt in RUNTIMES:
            res = final_report[rt][sample]
            if res:
                avg_cold = statistics.mean([r.cold_start for r in res])
                avg_exec = statistics.mean([r.exec_time for r in res])
                avg_e2e = statistics.mean([r.e2e_latency for r in res])
                print(f"{rt:9} | {avg_cold:13.3f} ms | {avg_exec:12.3f} ms | {avg_e2e:14.3f} ms")
            else:
                print(f"{rt:9} | {'FAILED':>16} | {'FAILED':>15} | {'FAILED':>17}")

if __name__ == "__main__":
    main()
