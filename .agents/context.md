# IaiServerless Project Context

## Project Overview
IaiServerless is a unikernel-based serverless runtime that executes functions inside lightweight KVM virtual machines, achieving near-process cold start latency with full VM-level isolation.

## Architecture

### Core Components
1. **Host Loader** (`host/`) - KVM-based loader that:
   - Boots a microVM with minimal overhead
   - Loads ELF binaries into guest memory
   - Proxies syscalls via hypercalls through I/O ports
   - Implements hypercall handlers for write, read, socket, connect, send, recv, close, gethostbyname

2. **Shim Library** (`shim/`) - Freestanding C library linked into guest binaries:
   - Provides standard C library headers (string.h, stdlib.h, stdio.h, unistd.h, netdb.h, setjmp.h)
   - Forwards syscalls to host via I/O port hypercalls (addr→0x10, len→0x11, trigger→0x12)
   - Implements malloc/free with simple free-list allocator
   - Implements brk/sbrk for heap management
   - Entry point: `start.S` → `_init_memory()` → `main()`
   - Build flags include `-U_FORTIFY_SOURCE` for CloudLab/Ubuntu compatibility

3. **Gateway** (`gateway/`) - HTTP dispatcher written in Go:
   - Routes function invocations to configured runtime: native, python, kvm, process, docker, junction
   - `native` mode auto-selects: `process` for c/*, `python3` for python/*
   - `junction` mode: runs functions under Junction libOS via `junction_run`
   - Measures cold start, execution time, and end-to-end latency
   - Parses X-Exec-Time from stderr (most runtimes) or stdout (junction)
   - `nativeBinPath()` helper shared between native and junction runtimes
   - Flags: `-runtime`, `-port`, `-junction-build`

4. **Sample Functions** (`samples/`) - Serverless functions in C and Python:
   - Built for three targets: KVM (.elf), native process (_proc), Docker
   - C examples: hello, prime, weather, weather_fmt, json_builder, alloc_stress, multi_req, bare_hello
   - Python examples: hello, prime, weather, weather_fmt, alloc_stress, multi_req, weather_parser
   - Test binaries: test_malloc, test_brk, test_string, test_stdio, test_stdlib, test_setjmp, test_wx_write, test_wx_exec
   - Docker images use Alpine (multi-stage for C, python:3.11-alpine for Python)

## Tech Stack
- **Languages**: C (host, shim, samples), Go (gateway), Assembly (shim entry point)
- **Build Tools**: gcc, ld, make, go, docker
- **Runtimes**: KVM (Linux kernel virtualization), Junction (Caladan-based libOS)
- **Architecture**: x86_64 long mode with 4-level paging
- **Memory Model**: Freestanding (no OS, no libc)

## Current Branch: add-read-support
Python support is complete and benchmarked. Junction runtime added.

**Status (2026-04-25): Junction runtime integrated. Benchmark includes 5 runtimes (process, kvm, docker, python, junction). Docker images optimized with Alpine.**

### What's Done
- MicroPython runs as a freestanding unikernel linked against `libshim.a`
- Python source files embedded into ELF via `objcopy` (`_binary_code_py_start`)
- `socket` module implemented in `external/python-port/modsocket.c` — standard `import socket` works
- `shim/netdb.h` added; NLR exception handling; `% string formatting` enabled
- `input()` builtin enabled via `mp_hal_stdin_rx_chr` + `mp_iai_readline`
- `read()` hypercall added (`IAI_READ`) — guest reads from host stdin via fd 0
- Gateway pipes `r.Body` to `cmd.Stdin` — functions receive request body via stdin
- `native` runtime mode: auto-selects `process` for `c/*`, `python3` for `python/*`
- Junction runtime: runs native binaries under Junction libOS via `junction_run`
- Sample functions: `hello.py`, `prime.py`, `weather.py`, `weather_fmt.py`, `alloc_stress.py`, `multi_req.py`, `weather_parser.py`
- `python/prime`: finds the **Nth prime** (not all primes up to N) — constant memory, matches C version behavior
- `c/weather_fmt`: fetches open-meteo JSON, POSTs to `python/weather_parser` via gateway (inter-function call demo)
- Demo UI: `gateway/static/index.html`, `scripts/demo.sh` (native:8081, kvm:8082, docker:8083)
- Docker images optimized: C uses multi-stage Alpine (3.5 MB), Python uses python:3.11-alpine (19.4 MB)
- Benchmark reports binary/image sizes per runtime
- `test_gateway.py` displays cold start, exec time, and E2E latency headers
- `-U_FORTIFY_SOURCE` added to shim and samples Makefiles for CloudLab compatibility

### Junction Runtime
- Gateway flag: `-runtime=junction -junction-build=<path>`
- Uses `nativeBinPath()` to resolve binaries (same as native: `_proc` for C, `python3` for Python)
- Junction resolves `python3` to absolute path (Junction doesn't search $PATH)
- Junction merges child stderr into stdout — gateway parses X-Exec-Time from stdout
- **Limitations**: No stdin forwarding, no networking (setsockopt unsupported)
- Benchmarkable samples: c/hello, c/json_builder, c/alloc_stress, python/hello, python/alloc_stress

### Python Coding Constraints (ROM_LEVEL_MINIMUM)
- No `str.encode()`/`bytes.decode()` — use `bytes("...", "utf-8")` and `str(b, "utf-8")`
- No float — use integer arithmetic
- No string slicing (`data[i:end]`) — use character-by-character accumulation
- No `any()`, `enumerate()` builtins
- `% formatting` and `import socket` and `input()` all work

### Benchmark Results (2026-04-25, 30 requests each, CloudLab)

**C Functions:**
| Function         | Process cold | KVM cold  | Docker cold | Junction cold | Process exec | KVM exec  | Docker exec | Junction exec |
|------------------|-------------|-----------|-------------|---------------|-------------|-----------|-------------|---------------|
| c/hello          | 1.3 ms      | 20.2 ms   | 177.1 ms    | 30.6 ms       | 0.02 ms     | 0.13 ms   | 0.01 ms     | 0.006 ms      |
| c/prime (N=10k)  | 1.2 ms      | 21.3 ms   | 177.5 ms    | SKIPPED       | 8.7 ms      | 8.4 ms    | 7.9 ms      | SKIPPED       |
| c/weather        | 1.3 ms      | 18.3 ms   | 174.0 ms    | SKIPPED       | 355.6 ms    | 344.3 ms  | 343.8 ms    | SKIPPED       |
| c/weather_fmt    | 1.3 ms      | 16.0 ms   | 182.7 ms    | SKIPPED       | 344.5 ms    | 368.2 ms  | 624.5 ms    | SKIPPED       |
| c/json_builder   | 1.3 ms      | 21.4 ms   | 178.3 ms    | 30.7 ms       | 0.05 ms     | 0.22 ms   | 0.02 ms     | 0.04 ms       |
| c/alloc_stress   | 1.2 ms      | 21.2 ms   | 178.2 ms    | 31.4 ms       | 0.10 ms     | 0.18 ms   | 0.04 ms     | 0.05 ms       |
| c/multi_req      | 1.3 ms      | 19.1 ms   | 186.0 ms    | SKIPPED       | 1035.9 ms   | 1032.0 ms | 1154.4 ms   | SKIPPED       |

**Python Functions:**
| Function              | Python cold | KVM cold  | Docker cold | Junction cold | Python exec | KVM exec  | Docker exec | Junction exec |
|-----------------------|------------|-----------|-------------|---------------|------------|-----------|-------------|---------------|
| python/hello          | 21.2 ms    | 20.5 ms   | 276.8 ms    | 63.1 ms       | 0.05 ms    | 0.46 ms   | 0.34 ms     | 0.08 ms       |
| python/prime (N=10k)  | 21.1 ms    | 10.6 ms   | 279.6 ms    | SKIPPED       | 205.9 ms   | 313.0 ms  | 235.7 ms    | SKIPPED       |
| python/weather        | 22.3 ms    | 17.5 ms   | 278.5 ms    | SKIPPED       | 357.3 ms   | 348.1 ms  | 379.5 ms    | SKIPPED       |
| python/weather_fmt    | 24.8 ms    | 17.0 ms   | 296.3 ms    | SKIPPED       | 378.0 ms   | 369.3 ms  | 663.6 ms    | SKIPPED       |
| python/alloc_stress   | 21.1 ms    | 20.0 ms   | 278.3 ms    | 64.0 ms       | 0.33 ms    | 1.48 ms   | 0.89 ms     | 0.37 ms       |
| python/multi_req      | 29.4 ms    | 18.5 ms   | 287.5 ms    | SKIPPED       | 1038.4 ms  | 1032.4 ms | 1086.3 ms   | SKIPPED       |

**Binary/Image Sizes:**
| Runtime          | C binary  | Python binary |
|------------------|-----------|---------------|
| Process/Junction | ~16 KB    | ~7.7 MB (CPython + script) |
| KVM              | 8-15 KB   | 262.4 KB (MicroPython unikernel) |
| Docker (Alpine)  | 3.5 MB    | 19.4 MB |

**Key Insights:**
- KVM cold start (~20ms) beats native Python process launch (~21ms) and Junction Python (~63ms)
- Junction C cold start (~31ms) is 1.5x slower than KVM (~20ms) due to Caladan boot + ld.so loading
- Junction Python cold start (~63ms) is 3x slower than KVM — pays for both Junction boot AND CPython startup
- KVM MicroPython unikernel (262 KB) is 30x smaller than CPython (7.7 MB) with comparable cold start
- Native C process has sub-1.5ms cold start (15-20x faster than KVM)
- Docker consistently slowest (177-296ms cold start); Alpine optimization reduced C images from ~150MB to 3.5MB
- Network-bound functions converge across runtimes (execution time dominated by I/O)
- Junction does not forward stdin to child processes
- Junction does not support networking (setsockopt unsupported)

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
  - `-ffreestanding -nostdlib -m64 -O2 -Wall -Wextra -U_FORTIFY_SOURCE`
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
- **Runtime**: Linux with KVM support (/dev/kvm), Junction (optional, for junction runtime)
- **Testing**: Python 3 (for benchmark.py, test_gateway.py)

## Testing Infrastructure
- `scripts/benchmark.py` - Runs 30 requests per function across all runtimes; `--junction <build_path>` enables Junction
- `scripts/test_gateway.py` - Tests specific endpoints, displays cold start/exec/E2E timing
- `scripts/run_sample.sh` - Runs single function with KVM runtime
- `test/mock_http_server.py` - Mock HTTP server for network testing

## Known Limitations
- No filesystem support (by design)
- No threading/multiprocessing
- Limited to x86_64 architecture
- Guest memory must be pre-allocated
- No dynamic library loading
- Junction: no stdin forwarding, no networking (setsockopt)
