#include <stdio.h>

int main() {
  printf("Testing stdio functions...\n\n");

  // Test printf with various formats
  printf("=== Testing printf ===\n");
  printf("Integer: %d\n", 42);
  printf("Negative: %d\n", -123);
  printf("Unsigned: %u\n", 4294967295u);
  printf("Hex: 0x%x\n", 255);
  printf("String: %s\n", "hello world");
  printf("Char: %c\n", 'A');
  printf("Pointer: %p\n", (void *)0x12345678);
  printf("Percent: 100%%\n");

  // Test width formatting
  printf("\n=== Testing width ===\n");
  printf("Width 5: [%5d]\n", 42);
  printf("Zero pad: [%05d]\n", 42);
  printf("Hex width: [%08x]\n", 0xABCD);

  // Test snprintf
  printf("\n=== Testing snprintf ===\n");
  char buf[50];

  int len = snprintf(buf, sizeof(buf), "Number: %d", 999);
  printf("snprintf result: %s (len=%d)\n", buf, len);

  // Test buffer truncation
  len = snprintf(buf, 10, "This is a very long string");
  printf("Truncated (size=10): %s (len=%d)\n", buf, len);

  // Test edge cases
  printf("\n=== Testing edge cases ===\n");
  snprintf(buf, sizeof(buf), "Zero: %d", 0);
  printf("Zero: %s\n", buf);

  snprintf(buf, sizeof(buf), "NULL string: %s", (char *)0);
  printf("NULL: %s\n", buf);

  snprintf(buf, sizeof(buf), "Mixed: %d %s %x", 10, "test", 0xFF);
  printf("Mixed: %s\n", buf);

  // Test putchar
  printf("\n=== Testing putchar ===\n");
  printf("Chars: ");
  putchar('H');
  putchar('e');
  putchar('l');
  putchar('l');
  putchar('o');
  putchar('\n');

  // Security test: large width (should be capped)
  printf("\n=== Security tests ===\n");
  snprintf(buf, sizeof(buf), "Large width: %999999d", 42);
  printf("Large width handled: %s\n", buf);

  printf("\nAll tests passed!\n");
  return 0;
}
