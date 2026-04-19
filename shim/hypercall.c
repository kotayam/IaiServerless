#include "hypercall.h"
#include "../iai_common.h"
#include "string.h"
#include <stddef.h>
#include <stdint.h>

char message_buffer[MESSAGE_BUFFER_SIZE];

// 32-bit hardware port interaction
static inline void outl(uint16_t port, uint32_t value) {
  asm volatile("outl %0, %1" : : "a"(value), "Nd"(port) : "memory");
}

long hypercall(uint32_t op, uint32_t arg0, uint32_t arg1, uint32_t arg2,
               uint32_t arg3, const void *payload, uint32_t payload_len) {
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
