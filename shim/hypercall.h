#ifndef SHIM_INTERNAL_H
#define SHIM_INTERNAL_H

#include <stdint.h>

/* Internal platform-specific definitions - not for user code */

#define MESSAGE_BUFFER_SIZE 4096

/* Shared message buffer for hypercalls */
extern char message_buffer[MESSAGE_BUFFER_SIZE];

/* Hypercall interface */
long hypercall(uint32_t op, uint32_t arg0, uint32_t arg1, uint32_t arg2,
               uint32_t arg3, const void *payload, uint32_t payload_len);

#endif /* SHIM_INTERNAL_H */
