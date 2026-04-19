# IaiServerless

A unikernel-based serverless runtime that executes functions inside lightweight KVM virtual machines, achieving near-process cold start latency with full VM-level isolation.

## Directories

- **host/** — KVM-based host loader that boots a microVM, loads an ELF binary, and proxies syscalls via hypercalls
- **shim/** — Freestanding shim library linked into guest binaries, providing implementations of standard C library headers (`string.h`, `stdlib.h`, `unistd.h`, `netdb.h`) that forward calls to the host via I/O port hypercalls
- **samples/** — Sample serverless functions written in standard C, built for three targets: KVM (`.elf`), native process (`_proc`), and Docker
- **gateway/** — HTTP gateway that dispatches function invocations to the configured runtime (kvm, process, or docker)
- **scripts/** — Benchmark and test utilities
- **test/** — Mock HTTP server for local network testing

## Building

Requires: `gcc`, `ld`, `make`, `go`, `docker`

```bash
make
```

This builds the host loader, shim library, and all sample binaries.

To build native process and Docker images:

```bash
make -C samples proc docker-all
```

## Running the Benchmark

```bash
./scripts/benchmark.py
```

Runs 30 requests per function across all three runtimes and prints a comparative report of cold start, execution, and end-to-end latency.

To specify the number of requests or a custom gateway port:

```bash
./scripts/benchmark.py 100
./scripts/benchmark.py --port 9090
./scripts/benchmark.py 100 --port 9090
```

## Testing

Start the gateway manually, then run the test script:

```bash
cd gateway && go run main.go -runtime=kvm
./scripts/test_gateway.py
```

To test specific endpoints or use a custom port:

```bash
./scripts/test_gateway.py c/hello c/prime
./scripts/test_gateway.py --port 9090
```

To run a single function with the KVM runtime directly:

```bash
./scripts/run_sample.sh <binary_name.elf>
```
