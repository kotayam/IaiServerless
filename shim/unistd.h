#ifndef SHIM_UNISTD_H
#define SHIM_UNISTD_H

#include <stddef.h>

// I/O operations
long write(int fd, const void *buf, size_t count);
long read(int fd, void *buf, size_t count);
int close(int fd);

// Network operations
int socket(int domain, int type, int protocol);
int connect(int sockfd, const void *addr, unsigned int addrlen);
long send(int sockfd, const void *buf, size_t len, int flags);
long recv(int sockfd, void *buf, size_t len, int flags);

#endif /* SHIM_UNISTD_H */
