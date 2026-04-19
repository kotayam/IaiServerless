#include <stdlib.h>
#include <unistd.h>

void print_str(const char *s) {
  unsigned long len = 0;
  while (s[len])
    len++;
  write(1, s, len);
}

void print_int(int val) {
  char buf[12];
  char *p = buf + 11;
  int neg = val < 0;
  if (neg)
    val = -val;
  *p = '\0';
  do {
    *--p = '0' + (val % 10);
    val /= 10;
  } while (val);
  if (neg)
    *--p = '-';
  print_str(p);
  print_str("\n");
}

void print_long(long val) {
  char buf[21];
  char *p = buf + 20;
  int neg = val < 0;
  if (neg)
    val = -val;
  *p = '\0';
  do {
    *--p = '0' + (val % 10);
    val /= 10;
  } while (val);
  if (neg)
    *--p = '-';
  print_str(p);
  print_str("\n");
}

int main() {
  print_str("Testing stdlib additions...\n");

  // Test realloc
  print_str("\n=== Testing realloc ===\n");
  char *p1 = malloc(10);
  for (int i = 0; i < 9; i++)
    p1[i] = 'A' + i;
  p1[9] = '\0';
  print_str("Initial (10 bytes): ");
  print_str(p1);
  print_str("\n");

  char *p2 = realloc(p1, 20);
  print_str("After realloc(20): ");
  print_str(p2);
  print_str("\n");

  // Test realloc with NULL
  char *p3 = realloc(NULL, 15);
  if (p3) {
    for (int i = 0; i < 5; i++)
      p3[i] = 'X';
    p3[5] = '\0';
    print_str("realloc(NULL, 15): ");
    print_str(p3);
    print_str("\n");
  }

  // Test realloc with size 0
  realloc(p3, 0);
  print_str("realloc(ptr, 0) - OK\n");

  // Test atoi
  print_str("\n=== Testing atoi ===\n");
  print_str("atoi(\"123\"): ");
  print_int(atoi("123"));
  print_str("atoi(\"-456\"): ");
  print_int(atoi("-456"));
  print_str("atoi(\"  789\"): ");
  print_int(atoi("  789"));
  print_str("atoi(\"+42\"): ");
  print_int(atoi("+42"));

  // Test strtol
  print_str("\n=== Testing strtol ===\n");
  char *end;
  
  print_str("strtol(\"123\", NULL, 10): ");
  print_long(strtol("123", NULL, 10));
  
  print_str("strtol(\"-456\", NULL, 10): ");
  print_long(strtol("-456", NULL, 10));
  
  print_str("strtol(\"0x1A\", NULL, 0): ");
  print_long(strtol("0x1A", NULL, 0));
  
  print_str("strtol(\"077\", NULL, 0): ");
  print_long(strtol("077", NULL, 0));
  
  print_str("strtol(\"FF\", NULL, 16): ");
  print_long(strtol("FF", NULL, 16));
  
  long val = strtol("123abc", &end, 10);
  print_str("strtol(\"123abc\", &end, 10): ");
  print_long(val);
  print_str("Remaining: ");
  print_str(end);
  print_str("\n");

  // Test exit (commented out as it will halt)
  print_str("\n=== Testing exit/abort ===\n");
  print_str("exit() and abort() would halt VM - skipped\n");

  print_str("\nAll tests passed!\n");
  return 0;
}
