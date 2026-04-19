#include "../iai_common.h"
#include "hypercall.h"
#include "string.h"
#include <stddef.h>
#include <stdint.h>

// intercept write
long write(int fd, const void *buf, size_t count) {
  return hypercall(IAI_WRITE, (uint32_t)fd, 0, 0, 0, buf, (uint32_t)count);
}

int socket(int domain, int type, int protocol) {
  return (int)hypercall(IAI_SOCKET, (uint32_t)domain, (uint32_t)type,
                        (uint32_t)protocol, 0, NULL, 0);
}

int connect(int sockfd, const void *addr, uint32_t addrlen) {
  return (int)hypercall(IAI_CONNECT, (uint32_t)sockfd, 0, 0, 0, addr, addrlen);
}

long send(int sockfd, const void *buf, size_t len, int flags) {
  return hypercall(IAI_SEND, (uint32_t)sockfd, (uint32_t)flags, 0, 0, buf,
                   (uint32_t)len);
}

long recv(int sockfd, void *buf, size_t len, int flags) {
  long ret = hypercall(IAI_RECV, (uint32_t)sockfd, (uint32_t)len,
                       (uint32_t)flags, 0, NULL, 0);
  if (ret > 0) {
    memcpy(buf, message_buffer + sizeof(struct iai_req), (size_t)ret);
  }
  return ret;
}

int close(int fd) {
  return (int)hypercall(IAI_CLOSE, (uint32_t)fd, 0, 0, 0, NULL, 0);
}
