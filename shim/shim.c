#include <stddef.h>

// implement strlen
size_t strlen(const char *s) {
  size_t len = 0;
  while (s[len] != '\0') {
    len++;
  }
  return len;
}

// hardware port interaction
static inline void outb(unsigned short port, unsigned char value) {
  asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

// intercept write
long write(int fd, const void *buf, size_t count) {
  (void)fd; // We are ignoring file descriptors (like stdout = 1) for now

  const char *char_buf = (const char *)buf;

  // Send each character to port 0xE9 for the Host Loader to catch
  for (size_t i = 0; i < count; i++) {
    outb(0xE9, char_buf[i]);
  }

  return count; // Return the number of bytes successfully written
}
