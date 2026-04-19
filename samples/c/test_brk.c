#include <unistd.h>

void print_str(const char *s) {
  unsigned long len = 0;
  while (s[len])
    len++;
  write(1, s, len);
}

void print_hex(unsigned long val) {
  char buf[19] = "0x";
  char *p = buf + 2;
  for (int i = 60; i >= 0; i -= 4) {
    int digit = (val >> i) & 0xF;
    *p++ = digit < 10 ? '0' + digit : 'a' + digit - 10;
  }
  *p++ = '\n';
  write(1, buf, p - buf);
}

int main() {
  print_str("Testing brk/sbrk...\n");

  // Get initial break
  void *initial = sbrk(0);
  print_str("Initial brk: ");
  print_hex((unsigned long)initial);

  // Allocate 1KB
  void *p1 = sbrk(1024);
  print_str("After sbrk(1024): ");
  print_hex((unsigned long)p1);

  // Try to allocate 1MB (should succeed with -m 4 and -m 2)
  void *p2 = sbrk(1024 * 1024);
  print_str("After sbrk(1MB): ");
  print_hex((unsigned long)p2);

  // Try to allocate 1MB (should succeed with -m 4, fail with -m 2)
  void *p3 = sbrk(1024 * 1024);
  print_str("After sbrk(1MB): ");
  print_hex((unsigned long)p3);

  // try to allocate 1KB
  void *p4 = sbrk(1024);
  print_str("After sbrk(1KB): ");
  print_hex((unsigned long)p4);

  // Try to set brk below _end (should fail)
  int ret = brk((void *)0x1000);
  print_str("brk(0x1000) returned: ");
  print_hex(ret);

  print_str("Test complete!\n");
  return 0;
}
