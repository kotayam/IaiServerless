# IaiServerless

<p align="center">
  <img src="assets/logo.png" alt="IaiServerless" width="180"/>
</p>

A unikernel-based serverless runtime that executes functions inside lightweight KVM virtual machines, achieving near-process cold start latency with full VM-level isolation.

## Features

- **Fast Cold Starts**: KVM unikernel boots in ~20ms, beating native Python process launch (43ms) by 2.2x
- **VM Isolation**: Full KVM-level isolation without container overhead
- **Multi-Language**: Supports C and Python (via MicroPython unikernel)
- **Stdin Support**: Functions accept input via HTTP POST body
- **Inter-Function Calls**: Functions can call other functions via HTTP
- **Three Runtime Modes**: Compare KVM, native process, and Docker side-by-side

## Directories

- **host/** — KVM-based host loader that boots a microVM, loads an ELF binary, and proxies syscalls via hypercalls
- **shim/** — Freestanding shim library linked into guest binaries, providing implementations of standard C library headers (`string.h`, `stdlib.h`, `stdio.h`, `unistd.h`, `netdb.h`, `setjmp.h`) that forward calls to the host via I/O port hypercalls
- **samples/** — Sample serverless functions in C and Python, built for three targets: KVM (`.elf`), native process (`_proc`), and Docker
- **gateway/** — HTTP gateway that dispatches function invocations to the configured runtime (native, kvm, docker)
- **scripts/** — Benchmark and test utilities
- **external/** — MicroPython port for unikernel execution

## Sample Functions

**C Functions**: `hello`, `prime`, `weather`, `weather_fmt`, `json_builder`, `alloc_stress`, `multi_req`

**Python Functions**: `hello`, `prime`, `weather`, `weather_fmt`, `alloc_stress`, `multi_req`, `weather_parser`

## Building

Requires: `gcc`, `ld`, `make`, `go`, `docker`

```bash
make
```

This builds the host loader, shim library, MicroPython unikernel, and all sample binaries.

To build native process and Docker images:

```bash
make -C samples proc docker-all
```

## Quick Start

Run the interactive demo UI:

```bash
./scripts/demo.sh
```

Then open http://localhost:8081 in your browser to compare runtimes side-by-side.

## Running the Benchmark

```bash
./scripts/benchmark.py
```

Runs 30 requests per function across all runtimes and prints a comparative report of cold start, execution, and end-to-end latency.

To specify the number of requests or a custom gateway port:

```bash
./scripts/benchmark.py 100
./scripts/benchmark.py --port 9090
./scripts/benchmark.py 100 --port 9090
```

## Performance Highlights

**KVM vs Native Python** (cold start):
- python/hello: **19.8ms** (KVM) vs 43.5ms (native) — **2.2x faster**
- python/weather: **21.3ms** (KVM) vs 44.9ms (native) — **2.1x faster**

**Native C Process** (cold start):
- c/hello: **0.5ms** — 40x faster than KVM, 87x faster than native Python

**Docker** (cold start):
- 130-180ms across all functions — slowest option

## Testing

Start the gateway manually, then run the test script:

```bash
cd gateway && go run main.go -runtime=kvm
./scripts/test_gateway.py
```

To test specific endpoints or use a custom port:

```bash
./scripts/test_gateway.py c/hello python/prime
./scripts/test_gateway.py --port 9090
```

To run a single function with the KVM runtime directly:

```bash
./scripts/run_sample.sh <binary_name.elf>
```

## Architecture

- **Guest Memory**: 2MB default (configurable via `-m` flag)
- **Hypercall Protocol**: addr→port 0x10, len→port 0x11, trigger→port 0x12
- **Syscalls Supported**: write, read, socket, connect, send, recv, close, gethostbyname, brk
- **Security**: W^X enforcement, no filesystem access, full VM isolation
