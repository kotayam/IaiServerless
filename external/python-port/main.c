#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "py/compile.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/frozenmod.h"
#include "py/lexer.h"
#include "py/builtin.h"

// Heap for MicroPython GC
static char heap[MICROPY_HEAP_SIZE];

int main(void) {
    // Initialize GC first
    gc_init(heap, heap + sizeof(heap));
    
    // Initialize MicroPython
    mp_init();

    // Read Python code from stdin (embedded in ELF as data section)
    extern char _binary_code_py_start[];
    extern char _binary_code_py_end[];
    
    size_t code_len = _binary_code_py_end - _binary_code_py_start;
    
    if (code_len > 0) {
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {
            mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_,
                                                          _binary_code_py_start,
                                                          code_len, 0);
            qstr source_name = lex->source_name;
            mp_parse_tree_t parse_tree = mp_parse(lex, MP_PARSE_FILE_INPUT);
            mp_obj_t module_fun = mp_compile(&parse_tree, source_name, false);
            mp_call_function_0(module_fun);
            mp_obj_t handler_obj = mp_load_global(MP_QSTR_handler);
            if (handler_obj != MP_OBJ_NULL) {
                mp_call_function_0(handler_obj);
            }
            nlr_pop();
        } else {
            mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
        }
    }

    // Clean up
    mp_deinit();
    
    return 0;
}

// Required: no filesystem, so file-based imports always fail
mp_import_stat_t mp_import_stat(const char *path) {
    (void)path;
    return MP_IMPORT_STAT_NO_EXIST;
}

mp_lexer_t *mp_lexer_new_from_file(qstr filename) {
    (void)filename;
    mp_raise_OSError(1);
}


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

// Required: Frozen module support (empty for now)
const char mp_frozen_names[] = {0};
const uint32_t mp_frozen_mpy_content[] = {0};
