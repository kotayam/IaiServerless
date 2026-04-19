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
Working on porting MicroPython to run as a unikernel on IaiServerless.

**Recent Progress (2026-04-19):**
- ✅ Completed all missing shim functions for MicroPython compatibility
- ✅ Added stdlib: realloc, abort, exit, atoi, strtol (with glibc 2.38+ alias)
- ✅ Added stdio: printf, snprintf, vsnprintf, putchar, putc, fputc with FILE/stdout/stderr support
- ✅ Added setjmp/longjmp for exception handling
- ✅ All implementations include security hardening (bounds checking, overflow protection)
- ✅ Created comprehensive tests: test_stdlib.c, test_stdio.c, test_setjmp.c
- ✅ Added MicroPython as git submodule in external/micropython/
- 🔄 Created out-of-tree port in external/python-port/ (currently building)

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
