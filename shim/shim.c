#include <stddef.h>
#include <stdint.h>

#define SHM_ADDR_PORT 0x10
#define SHM_LEN_PORT 0x11
#define MESSAGE_BUFFER_SIZE 4096

static char message_buffer[MESSAGE_BUFFER_SIZE];

// copy src to dest
void *memcpy(void *dest, const void *src, size_t n) {
  char *d = dest;
  const char *s = src;
  while (n--) {
    *d++ = *s++;
  }
  return dest;
}

// get length of string
size_t strlen(const char *s) {
  size_t len = 0;
  while (s[len] != '\0') {
    len++;
  }
  return len;
}

// 32-bit hardware port interaction
static inline void outl(uint16_t port, uint32_t value) {
  asm volatile("outl %0, %1" : : "a"(value), "Nd"(port) : "memory");
}

// intercept write
long write(int fd, const void *buf, size_t count) {
  (void)fd;
  // copy user data in to message buffer
  memcpy(message_buffer, buf, count);

  // tell host the address of buffer via port 0x10
  outl(SHM_ADDR_PORT, (uint32_t)(uintptr_t)message_buffer);

  // tell host the length of the data via port 0x11
  outl(SHM_LEN_PORT, count);

  return count; // Return the number of bytes successfully written
}
