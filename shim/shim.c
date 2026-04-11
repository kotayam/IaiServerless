#include <stddef.h>
#include <stdint.h>
#include "../iai_common.h"

#define MESSAGE_BUFFER_SIZE 4096

static char message_buffer[MESSAGE_BUFFER_SIZE];

// 32-bit hardware port interaction
static inline void outl(uint16_t port, uint32_t value) {
  asm volatile("outl %0, %1" : : "a"(value), "Nd"(port) : "memory");
}

// get length of string
size_t strlen(const char *s) {
  size_t len = 0;
  while (s[len] != '\0') {
    len++;
  }
  return len;
}

// copy src to dest
void *memcpy(void *dest, const void *src, size_t n) {
  char *d = dest;
  const char *s = src;
  while (n--) {
    *d++ = *s++;
  }
  return dest;
}

// set memory
void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

static long hypercall(uint32_t op, uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, const void *payload, uint32_t payload_len) {
    struct iai_req *req = (struct iai_req *)message_buffer;
    req->op = op;
    req->args[0] = arg0;
    req->args[1] = arg1;
    req->args[2] = arg2;
    req->args[3] = arg3;
    req->data_len = payload_len;

    if (payload && payload_len > 0) {
        if (payload_len + sizeof(struct iai_req) > MESSAGE_BUFFER_SIZE) {
            return -1; 
        }
        memcpy(message_buffer + sizeof(struct iai_req), payload, payload_len);
    }

    // tell host the address of buffer
    outl(SHM_ADDR_PORT, (uint32_t)(uintptr_t)message_buffer);
    // trigger the host hypercall
    outl(HYPERCALL_PORT, 0);

    return (long)req->ret;
}

// intercept write
long write(int fd, const void *buf, size_t count) {
    return hypercall(IAI_WRITE, (uint32_t)fd, 0, 0, 0, buf, (uint32_t)count);
}

int socket(int domain, int type, int protocol) {
    return (int)hypercall(IAI_SOCKET, (uint32_t)domain, (uint32_t)type, (uint32_t)protocol, 0, NULL, 0);
}

int connect(int sockfd, const void *addr, uint32_t addrlen) {
    return (int)hypercall(IAI_CONNECT, (uint32_t)sockfd, 0, 0, 0, addr, addrlen);
}

long send(int sockfd, const void *buf, size_t len, int flags) {
    return hypercall(IAI_SEND, (uint32_t)sockfd, (uint32_t)flags, 0, 0, buf, (uint32_t)len);
}

long recv(int sockfd, void *buf, size_t len, int flags) {
    long ret = hypercall(IAI_RECV, (uint32_t)sockfd, (uint32_t)len, (uint32_t)flags, 0, NULL, 0);
    if (ret > 0) {
        memcpy(buf, message_buffer + sizeof(struct iai_req), (size_t)ret);
    }
    return ret;
}

int close(int fd) {
    return (int)hypercall(IAI_CLOSE, (uint32_t)fd, 0, 0, 0, NULL, 0);
}

int gethostbyname_r(const char *name, uint32_t *addr) {
    long ret = hypercall(IAI_GETHOSTBYNAME, 0, 0, 0, 0, name, (uint32_t)strlen(name) + 1);
    if (ret == 0) {
        memcpy(addr, message_buffer + sizeof(struct iai_req), 4);
    }
    return (int)ret;
}
