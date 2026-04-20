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
   - Provides standard C library headers (string.h, stdlib.h, stdio.h, unistd.h, netdb.h, setjmp.h)
   - Forwards syscalls to host via I/O port hypercalls (addr→0x10, len→0x11, trigger→0x12)
   - Implements malloc/free with simple free-list allocator
   - Implements brk/sbrk for heap management
   - Entry point: `start.S` → `_init_memory()` → `main()`

3. **Gateway** (`gateway/`) - HTTP dispatcher written in Go:
   - Routes function invocations to configured runtime (native, python, kvm, process, docker)
   - `native` mode auto-selects: `process` for c/*, `python3` for python/*
   - Measures cold start, execution time, and end-to-end latency
   - Parses X-Exec-Time header from function output

4. **Sample Functions** (`samples/`) - Serverless functions in C and Python:
   - Built for three targets: KVM (.elf), native process (_proc), Docker
   - C examples: hello, prime, weather, weather_fmt, json_builder, alloc_stress, multi_req, bare_hello
   - Python examples: hello, prime, weather, weather_fmt, alloc_stress, multi_req, weather_parser
   - Test binaries: test_malloc, test_brk, test_string, test_stdio, test_stdlib, test_setjmp, test_wx_write, test_wx_exec

## Tech Stack
- **Languages**: C (host, shim, samples), Go (gateway), Assembly (shim entry point)
- **Build Tools**: gcc, ld, make, go, docker
- **Runtime**: KVM (Linux kernel virtualization)
- **Architecture**: x86_64 long mode with 4-level paging
- **Memory Model**: Freestanding (no OS, no libc)

## Current Branch: add-read-support
Python support is complete and benchmarked.

**Status (2026-04-19): Inter-function calls working. Demo UI complete. Prime function optimized for constant memory. No active tasks.**

### What's Done
- MicroPython runs as a freestanding unikernel linked against `libshim.a`
- Python source files embedded into ELF via `objcopy` (`_binary_code_py_start`)
- `socket` module implemented in `external/python-port/modsocket.c` — standard `import socket` works
- `shim/netdb.h` added; NLR exception handling; `% string formatting` enabled
- `input()` builtin enabled via `mp_hal_stdin_rx_chr` + `mp_iai_readline`
- `read()` hypercall added (`IAI_READ`) — guest reads from host stdin via fd 0
- Gateway pipes `r.Body` to `cmd.Stdin` — functions receive request body via stdin
- `native` runtime mode: auto-selects `process` for `c/*`, `python3` for `python/*`
- Sample functions: `hello.py`, `prime.py`, `weather.py`, `weather_fmt.py`, `alloc_stress.py`, `multi_req.py`, `weather_parser.py`
- `python/prime`: finds the **Nth prime** (not all primes up to N) — constant memory, matches C version behavior
- `c/weather_fmt`: fetches open-meteo JSON, POSTs to `python/weather_parser` via gateway (inter-function call demo)
- Demo UI: `gateway/static/index.html`, `scripts/demo.sh` (native:8081, kvm:8082, docker:8083)
- Dockerfile and proc targets; benchmark extended with all Python samples

### Python Coding Constraints (ROM_LEVEL_MINIMUM)
- No `str.encode()`/`bytes.decode()` — use `bytes("...", "utf-8")` and `str(b, "utf-8")`
- No float — use integer arithmetic
- No string slicing (`data[i:end]`) — use character-by-character accumulation
- No `any()`, `enumerate()` builtins
- `% formatting` and `import socket` and `input()` all work

### Benchmark Results (2026-04-19, 30 requests each)

**Python Functions:**
| Function              | KVM cold | Native cold | Docker cold | KVM exec | Native exec | Docker exec |
|-----------------------|----------|-------------|-------------|----------|-------------|-------------|
| python/hello          | 19.8 ms  | 43.5 ms     | 171.7 ms    | 0.3 ms   | 0.0 ms      | 0.2 ms      |
| python/prime (N=10k)  | 19.4 ms  | 43.6 ms     | 178.6 ms    | 109.0 ms | 78.0 ms     | 92.0 ms     |
| python/weather        | 21.3 ms  | 44.9 ms     | 180.1 ms    | 339.3 ms | 341.5 ms    | 352.6 ms    |
| python/weather_fmt    | 18.5 ms  | 45.7 ms     | 174.1 ms    | 367.0 ms | 398.7 ms    | 537.3 ms    |

**C Functions:**
| Function              | Process cold | KVM cold | Docker cold | Process exec | KVM exec | Docker exec |
|-----------------------|--------------|----------|-------------|--------------|----------|-------------|
| c/hello               | 0.5 ms       | 19.0 ms  | 129.9 ms    | 0.0 ms       | 0.1 ms   | 0.0 ms      |
| c/prime (N=10k)       | 0.5 ms       | 19.8 ms  | 131.0 ms    | 5.6 ms       | 6.2 ms   | 5.5 ms      |
| c/weather             | 1.0 ms       | 21.1 ms  | 137.5 ms    | 824.6 ms     | 351.0 ms | 343.5 ms    |
| c/weather_fmt         | 0.9 ms       | 21.4 ms  | 133.4 ms    | 347.8 ms     | 378.4 ms | 533.5 ms    |

**Key Insights:**
- KVM cold start beats native Python process launch (2.2x faster)
- Native C process has sub-millisecond cold start (40x faster than KVM)
- Docker consistently slowest (130-180ms cold start)
- Network-bound functions converge across runtimes (execution time dominated by I/O)

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
