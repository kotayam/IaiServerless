# IaiServerless

A unikernel-based serverless runtime that executes functions inside lightweight KVM virtual machines, achieving near-process cold start latency with full VM-level isolation.

## Directories

- **host/** — KVM-based host loader that boots a microVM, loads an ELF binary, and proxies syscalls via hypercalls
- **shim/** — Freestanding shim library linked into guest binaries, intercepting standard library calls and forwarding them to the host via I/O ports
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

To specify the number of requests:

```bash
./scripts/benchmark.py 100
```

## Testing

Run a single function with the KVM runtime:

```bash
./scripts/run_sample.sh <binary_name.elf>
```
