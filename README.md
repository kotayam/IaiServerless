# IaiServerless

<p align="center">
  <img src="assets/logo.png" alt="IaiServerless" width="180"/>
</p>

A unikernel-based serverless runtime that executes functions inside lightweight KVM virtual machines, achieving near-process cold start latency with full VM-level isolation.

## Features

- **Fast Cold Starts**: KVM unikernel boots in ~20ms, beating native Python process launch (21ms) with full VM isolation
- **VM Isolation**: Full KVM-level isolation without container overhead
- **Multi-Language**: Supports C and Python (via MicroPython unikernel)
- **Stdin Support**: Functions accept input via HTTP POST body
- **Inter-Function Calls**: Functions can call other functions via HTTP
- **Five Runtime Modes**: Compare KVM, native process, Docker, Junction (libOS), and native Python side-by-side

## Directories

- **host/** — KVM-based host loader that boots a microVM, loads an ELF binary, and proxies syscalls via hypercalls
- **shim/** — Freestanding shim library linked into guest binaries, providing implementations of standard C library headers (`string.h`, `stdlib.h`, `stdio.h`, `unistd.h`, `netdb.h`, `setjmp.h`) that forward calls to the host via I/O port hypercalls
- **samples/** — Sample serverless functions in C and Python, built for three targets: KVM (`.elf`), native process (`_proc`), and Docker
- **gateway/** — HTTP gateway that dispatches function invocations to the configured runtime (native, kvm, docker, junction)
- **scripts/** — Benchmark, test, and graphing utilities
- **external/** — MicroPython port for unikernel execution

## Sample Functions

**C Functions**: `hello`, `prime`, `weather`, `weather_fmt`, `json_builder`, `alloc_stress`, `multi_req`

**Python Functions**: `hello`, `prime`, `weather`, `weather_fmt`, `alloc_stress`, `multi_req`, `weather_parser`

## Prerequisites

- `gcc`, `ld`, `make`, `go`, `docker`, `python3`
- Linux with KVM support (`/dev/kvm`)
- (Optional) [Junction](https://github.com/JunctionOS/junction) for the junction runtime

## Building

```bash
# Initialize MicroPython submodule (first time only)
git submodule update --init external/micropython

# Build host loader, shim library, MicroPython unikernel, and all KVM sample binaries
make

# Build native process binaries and Docker images
make -C samples proc docker-all
```

## Quick Start

### Run the Interactive Demo UI

```bash
./scripts/demo.sh
```

Open http://localhost:8081 in your browser to compare native, KVM, and Docker runtimes side-by-side.

### Run a Single Function (KVM)

```bash
./scripts/run_sample.sh c/hello.elf
```

### Try the Serverless Experience

```bash
cd gateway && ./gateway -runtime=kvm
```

Then invoke functions via HTTP:

```bash
# GET request
curl http://localhost:8080/c/hello

# POST with input (e.g., find the 10000th prime)
curl -X POST -d "10000" http://localhost:8080/c/prime
```

## Gateway Runtimes

The gateway supports multiple runtimes via the `-runtime` flag:

| Flag | Description |
|------|-------------|
| `-runtime=native` | Auto-selects: `process` for `c/*`, `python3` for `python/*` |
| `-runtime=kvm` | Runs `.elf` binaries in KVM microVM |
| `-runtime=process` | Runs `_proc` binaries as native Linux processes |
| `-runtime=python` | Runs `.py` scripts with system `python3` |
| `-runtime=docker` | Runs functions in Docker containers |
| `-runtime=junction` | Runs native binaries under Junction libOS (requires `-junction-build`) |

**Junction example:**

```bash
cd gateway && go run main.go -runtime=junction -junction-build=/path/to/junction/build
```

## Running the Benchmark

```bash
# Standard benchmark (native, KVM, Docker) — 30 requests per function
./scripts/benchmark.py

# Custom number of requests
./scripts/benchmark.py 100

# Include Junction runtime
./scripts/benchmark.py --junction /path/to/junction/build

# Custom port
./scripts/benchmark.py --port 9090

# All options combined
./scripts/benchmark.py 100 --junction /path/to/junction/build --port 9090
```

The benchmark automatically:
1. Builds native process binaries and Docker images
2. Starts/stops gateways for each runtime
3. Prints a comparative report with cold start, execution time, E2E latency, and binary size
4. Exports results to `scripts/benchmark_result.csv`

### Generating Graphs

After running the benchmark (or using an existing CSV):

```bash
python3 scripts/plot_benchmark.py
```

This generates four PNG graphs in `scripts/`:
- `bench_cold_start.png` — Cold start latency comparison
- `bench_exec_time.png` — Execution time comparison
- `bench_e2e.png` — End-to-end latency comparison
- `bench_binary_size.png` — Binary/image size comparison

To plot from a specific CSV:

```bash
python3 scripts/plot_benchmark.py path/to/benchmark_result.csv
```

Requires `matplotlib` (`sudo apt install python3-matplotlib`).

## Testing

```bash
# Test all default endpoints against a running gateway
./scripts/test_gateway.py

# Test specific endpoints
./scripts/test_gateway.py c/hello python/prime

# Custom port
./scripts/test_gateway.py --port 9090
```

Each test displays cold start, execution time, and E2E latency.

## Performance Highlights (30 requests, CloudLab)

**Cold Start:**

| Runtime | c/hello | python/hello |
|---------|---------|-------------|
| Native process | 1.3 ms | 21.2 ms |
| **KVM** | **20.2 ms** | **20.5 ms** |
| Junction | 30.6 ms | 63.1 ms |
| Docker | 177.1 ms | 276.8 ms |

**Key Insights:**
- KVM cold start (~20ms) matches native Python and provides full VM isolation
- KVM MicroPython unikernel is 262 KB vs 7.7 MB for CPython — 30x smaller
- Native C process has sub-1.5ms cold start
- Docker is 9-14x slower than KVM on cold start
- Network-bound functions converge across runtimes (execution time dominated by I/O)

## Architecture

- **Guest Memory**: 2MB default (configurable via `-m` flag)
- **Hypercall Protocol**: addr→port 0x10, len→port 0x11, trigger→port 0x12
- **Syscalls Supported**: write, read, socket, connect, send, recv, close, gethostbyname, brk
- **Security**: W^X enforcement, no filesystem access, full VM isolation
