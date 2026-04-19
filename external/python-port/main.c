#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "py/compile.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/frozenmod.h"

// Heap for MicroPython GC
static char heap[MICROPY_HEAP_SIZE];

int main(void) {
    // Initialize MicroPython
    mp_init();

    // Initialize GC
    gc_init(heap, heap + sizeof(heap));

    // Execute frozen module (will be added later)
    printf("MicroPython initialized on IaiServerless\n");

    // Clean up
    mp_deinit();
    
    return 0;
}

// Required: GC collect callback
void gc_collect(void) {
    gc_collect_start();
    gc_collect_end();
}

// Required: Handle uncaught exceptions
void nlr_jump_fail(void *val) {
    printf("FATAL: uncaught exception\n");
    exit(1);
}

// Required: Assert handler
void __assert_func(const char *file, int line, const char *func, const char *expr) {
    printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    exit(1);
}

// Alias for glibc assert
void __assert_fail(const char *expr, const char *file, unsigned int line, const char *func) {
    printf("Assertion '%s' failed, at file %s:%u\n", expr, file, line);
    exit(1);
}

// Required: HAL stdout function
void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    write(1, str, len);
}
