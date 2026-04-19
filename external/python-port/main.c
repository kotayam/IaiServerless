#include <stdio.h>
#include <stdlib.h>
#include "py/compile.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "py/mperrno.h"

// Heap for MicroPython GC
static char heap[MICROPY_HEAP_SIZE];

int main(void) {
    // Initialize MicroPython
    mp_init();
    mp_obj_list_init(MP_OBJ_TO_PTR(mp_sys_path), 0);
    mp_obj_list_init(MP_OBJ_TO_PTR(mp_sys_argv), 0);

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
