#ifndef IAI_H
#define IAI_H

#include <stddef.h>
#include <stdint.h>

/* Constants */
#define AF_INET 2
#define SOCK_STREAM 1

/* Standard Lib Replacements */
long write(int fd, const void *buf, size_t count);
size_t strlen(const char *s);
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);

/* Network Proxy Syscalls */
int socket(int domain, int type, int protocol);
int connect(int sockfd, const void *addr, uint32_t addrlen);
long send(int sockfd, const void *buf, size_t len, int flags);
long recv(int sockfd, void *buf, size_t len, int flags);
int close(int fd);
int gethostbyname_r(const char *name, uint32_t *addr);

/* Networking Structs */
struct guest_sockaddr_in {
    uint16_t sin_family;
    uint16_t sin_port;
    uint32_t sin_addr;
    char sin_zero[8];
};

/* Byte Order Helpers */
static inline uint16_t htons(uint16_t hostshort) {
    return (hostshort << 8) | (hostshort >> 8);
}

static inline uint32_t inet_addr(const char *cp) {
    uint32_t addr = 0;
    uint32_t part = 0;
    while (*cp) {
        if (*cp == '.') {
            addr = (addr << 8) | (part & 0xFF);
            part = 0;
        } else {
            part = part * 10 + (*cp - '0');
        }
        cp++;
    }
    addr = (addr << 8) | (part & 0xFF);
    uint32_t res = 0;
    res |= (addr & 0x000000FF) << 24;
    res |= (addr & 0x0000FF00) << 8;
    res |= (addr & 0x00FF0000) >> 8;
    res |= (addr & 0xFF000000) >> 24;
    return res;
}

#endif /* IAI_H */
