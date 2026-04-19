#include <string.h>
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

int main() {
  print_str("Testing string.h...\n");

  // Test strlen
  const char *s1 = "hello";
  print_str("strlen(\"hello\"): ");
  print_int(strlen(s1));

  // Test strcpy
  char buf1[20];
  strcpy(buf1, "world");
  print_str("strcpy: ");
  print_str(buf1);
  print_str("\n");

  // Test strncpy
  char buf2[20] = {0};
  strncpy(buf2, "testing", 4);
  print_str("strncpy(4): ");
  print_str(buf2);
  print_str("\n");

  // Test strcat
  char buf3[20] = "foo";
  strcat(buf3, "bar");
  print_str("strcat: ");
  print_str(buf3);
  print_str("\n");

  // Test strcmp
  print_str("strcmp(\"abc\", \"abc\"): ");
  print_int(strcmp("abc", "abc"));
  print_str("strcmp(\"abc\", \"abd\"): ");
  print_int(strcmp("abc", "abd"));

  // Test strncmp
  print_str("strncmp(\"abc\", \"abd\", 2): ");
  print_int(strncmp("abc", "abd", 2));

  // Test strchr
  const char *p = strchr("hello", 'l');
  print_str("strchr('l'): ");
  print_str(p ? p : "(null)");
  print_str("\n");

  // Test strrchr
  p = strrchr("hello", 'l');
  print_str("strrchr('l'): ");
  print_str(p ? p : "(null)");
  print_str("\n");

  // Test strstr
  p = strstr("hello world", "wor");
  print_str("strstr(\"wor\"): ");
  print_str(p ? p : "(null)");
  print_str("\n");

  // Test memcpy
  char buf4[10];
  memcpy(buf4, "memcpy", 7);
  print_str("memcpy: ");
  print_str(buf4);
  print_str("\n");

  // Test memmove
  char buf5[10] = "overlap";
  memmove(buf5 + 2, buf5, 5);
  print_str("memmove: ");
  print_str(buf5);
  print_str("\n");

  // Test memset
  char buf6[10];
  memset(buf6, 'X', 5);
  buf6[5] = '\0';
  print_str("memset: ");
  print_str(buf6);
  print_str("\n");

  // Test memcmp
  print_str("memcmp(\"abc\", \"abc\", 3): ");
  print_int(memcmp("abc", "abc", 3));
  print_str("memcmp(\"abc\", \"abd\", 3): ");
  print_int(memcmp("abc", "abd", 3));

  print_str("All tests passed!\n");
  return 0;
}
