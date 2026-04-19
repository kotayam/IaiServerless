#include <stdint.h>

// Minimal MicroPython configuration for IaiServerless unikernel

#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_MINIMUM)

// Disable compiler - we only run frozen modules
#define MICROPY_ENABLE_COMPILER     (0)

#define MICROPY_QSTR_EXTRA_POOL           mp_qstr_frozen_const_pool
#define MICROPY_ENABLE_GC                 (1)
#define MICROPY_HELPER_REPL               (0)
#define MICROPY_MODULE_FROZEN_MPY         (1)
#define MICROPY_ENABLE_EXTERNAL_IMPORT    (0)

// Disable filesystem
#define MICROPY_VFS                       (0)
#define MICROPY_PY_IO                     (0)

// Disable float support
#define MICROPY_FLOAT_IMPL                (MICROPY_FLOAT_IMPL_NONE)

// Disable threading
#define MICROPY_PY_THREAD                 (0)

// Disable sys module features
#define MICROPY_PY_SYS_MODULES            (0)
#define MICROPY_PY_SYS_EXIT               (0)
#define MICROPY_PY_SYS_PATH               (0)
#define MICROPY_PY_SYS_ARGV               (0)

// Minimal heap for serverless functions
#define MICROPY_HEAP_SIZE                 (64 * 1024)  // 64KB

typedef long mp_off_t;

#define MICROPY_HW_BOARD_NAME "iaiserverless"
#define MICROPY_HW_MCU_NAME "x86_64-kvm"

#define MP_STATE_PORT MP_STATE_VM
