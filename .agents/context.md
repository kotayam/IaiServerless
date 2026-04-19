# IaiServerless Project Context

## Project Overview
IaiServerless is a unikernel-based serverless runtime that executes functions inside lightweight KVM virtual machines, achieving near-process cold start latency with full VM-level isolation.

## Architecture

### Core Components
1. **Host Loader** (`host/`) - KVM-based loader that:
   - Boots a microVM with minimal overhead
   - Loads ELF binaries into guest memory
   - Proxies syscalls via hypercalls through I/O ports
   - Implements hypercall handlers for write, socket, connect, send, recv, close, gethostbyname

2. **Shim Library** (`shim/`) - Freestanding C library linked into guest binaries:
   - Provides standard C library headers (string.h, stdlib.h, unistd.h, netdb.h)
   - Forwards syscalls to host via I/O port hypercalls (ports 0x10, 0x11, 0x12)
   - Implements malloc/free with simple free-list allocator
   - Implements brk/sbrk for heap management
   - Entry point: `start.S` → `_init_memory()` → `main()`

3. **Gateway** (`gateway/`) - HTTP dispatcher written in Go:
   - Routes function invocations to configured runtime (kvm, process, docker)
   - Measures cold start, execution time, and end-to-end latency
   - Parses X-Exec-Time header from function output

4. **Sample Functions** (`samples/`) - Serverless functions in C/C++:
   - Built for three targets: KVM (.elf), native process (_proc), Docker
   - Examples: hello, prime, weather, json_builder, net_query
   - Test binaries: test_malloc, test_brk, test_string, test_wx_write, test_wx_exec

## Tech Stack
- **Languages**: C (host, shim, samples), Go (gateway), Assembly (shim entry point)
- **Build Tools**: gcc, ld, make, go, docker
- **Runtime**: KVM (Linux kernel virtualization)
- **Architecture**: x86_64 long mode with 4-level paging
- **Memory Model**: Freestanding (no OS, no libc)

## Current Branch: add-python-support
Python support is complete and benchmarked.

**Status (2026-04-19): All Python tasks done. No active tasks.**

### What's Done
- MicroPython runs as a freestanding unikernel linked against `libshim.a`
- Python source files embedded into ELF via `objcopy` (`_binary_code_py_start`)
- `socket` module implemented in `external/python-port/modsocket.c`, registered as `socket` — standard `import socket` works
- `shim/netdb.h` added (was missing)
- NLR exception handling in `main.c` prints tracebacks instead of silent crash
- `% string formatting` enabled (`MICROPY_PY_BUILTINS_STR_OP_MODULO`)
- Sample functions: `hello.py`, `prime.py`, `weather.py` (live HTTP to open-meteo.com)
- Build system: `samples/python/Makefile` + `build.sh`, hooked into root `make`
- Dockerfile for Python samples (`python:3.11-slim`)
- Python process runtime support in gateway (`python` runtime mode)
- Benchmark extended with `python/hello`, `python/prime`, `python/weather`

### Benchmark Results (2026-04-19, 30 requests each)
| Function       | KVM cold start | Python process cold start | Docker cold start |
|----------------|---------------|--------------------------|-------------------|
| python/hello   | 18.9 ms       | 41.0 ms                  | 162.3 ms          |
| python/prime   | 18.5 ms       | 42.2 ms                  | 169.7 ms          |
| python/weather | 25.0 ms       | 45.0 ms                  | 175.1 ms          |

KVM cold start beats native `python3` process launch. Execution times are near-identical across runtimes (network-bound for weather.py at ~340ms).

### Python Coding Constraints (ROM_LEVEL_MINIMUM)
- No `str.encode()` / `bytes.decode()` — use `bytes("...", "utf-8")` and `str(b, "utf-8")`
- No float — use integer arithmetic (e.g. `isqrt` pattern)
- `% formatting` available
- `import socket` works; `socket.socket()`, `s.connect((host, port))`, `s.send(b)`, `s.recv(n)`, `s.close()`

## Key Design Decisions

### Memory Management
- Default guest memory: 2MB (configurable via `-m` flag)
- Page size: 4KB
- Heap starts at `_end` symbol, grows via sbrk
- Simple free-list allocator for malloc/free

### Hypercall Protocol
- Guest writes buffer address to port 0x10
- Guest triggers hypercall via port 0x12
- Request structure: `struct iai_req` with op, ret, err, args[4], data_len
- Host processes syscall and writes result back to shared buffer

### Security Features
- W^X enforcement: test_wx_write and test_wx_exec validate that writable pages are not executable
- Full VM isolation via KVM
- No filesystem access (functions are self-contained ELF binaries)

## Coding Standards
- **C Code**: 
  - Freestanding (no standard library dependencies in shim)
  - `-ffreestanding -nostdlib -m64 -O2 -Wall -Wextra`
  - Manual memory management
  - Explicit error handling (return codes, no exceptions)
  - Security: bounds checking, integer overflow protection, pointer validation
  
- **Naming Conventions**:
  - Hypercall ops: `IAI_*` (e.g., IAI_WRITE, IAI_SOCKET)
  - I/O ports: `*_PORT` (e.g., HYPERCALL_PORT)
  - Shim functions: standard C library names (write, malloc, strlen, etc.)
  - Compiler optimization aliases: Add `_` prefix and glibc version variants as needed

- **Build System**:
  - Root Makefile orchestrates host, shim, samples, gateway
  - Shim builds libshim.a static archive
  - Samples link against shim with custom linker script (link.ld)

## Dependencies
- **Build-time**: gcc, ld, ar, make, go 1.x, docker
- **Runtime**: Linux with KVM support (/dev/kvm)
- **Testing**: Python 3 (for benchmark.py, test_gateway.py)

## Testing Infrastructure
- `scripts/benchmark.py` - Runs 30 requests per function across all runtimes
- `scripts/test_gateway.py` - Tests specific endpoints
- `scripts/run_sample.sh` - Runs single function with KVM runtime
- `test/mock_http_server.py` - Mock HTTP server for network testing

## Known Limitations
- No filesystem support (by design)
- No threading/multiprocessing
- Limited to x86_64 architecture
- Guest memory must be pre-allocated
- No dynamic library loading
