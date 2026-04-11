#ifndef IAI_COMMON_H
#define IAI_COMMON_H

#include <stdint.h>

/* I/O Ports */
#define SHM_ADDR_PORT 0x10
#define SHM_LEN_PORT 0x11
#define HYPERCALL_PORT 0x12

/* Global debug flag (used in Host Loader) */
extern int iai_debug;

/* Operation IDs */
enum iai_op {
    IAI_WRITE = 1,
    IAI_SOCKET,
    IAI_CONNECT,
    IAI_SEND,
    IAI_RECV,
    IAI_CLOSE,
    IAI_GETHOSTBYNAME,
};

/* Request Header for Proxy Syscalls */
struct iai_req {
    uint32_t op;        /* Operation ID */
    int32_t  ret;       /* Return value (Host to Guest) */
    int32_t  err;       /* Errno (Host to Guest) */
    uint32_t args[4];   /* Generic arguments */
    uint32_t data_len;  /* Length of payload following the header */
};

#endif /* IAI_COMMON_H */
