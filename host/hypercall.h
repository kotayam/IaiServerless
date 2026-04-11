#ifndef IAI_HYPERCALL_H
#define IAI_HYPERCALL_H

#include <stdint.h>

/* Initialize the FD mapping table */
void init_fd_map();

/* Dispatch and handle the hypercall request in the guest shared buffer */
void handle_hypercall(char *mem, uint32_t msg_buf_addr);

#endif /* IAI_HYPERCALL_H */
