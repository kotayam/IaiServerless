#include <stddef.h>

size_t strlen(const char *s) {
  size_t len = 0;
  while (s[len] != '\0') len++;
  return len;
}

char *strcpy(char *dest, const char *src) {
  char *d = dest;
  while ((*d++ = *src++));
  return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
  size_t i;
  for (i = 0; i < n && src[i]; i++) dest[i] = src[i];
  for (; i < n; i++) dest[i] = '\0';
  return dest;
}

char *strcat(char *dest, const char *src) {
  char *d = dest;
  while (*d) d++;
  while ((*d++ = *src++));
  return dest;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 && *s1 == *s2) { s1++; s2++; }
  return (unsigned char)*s1 - (unsigned char)*s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  while (n-- && *s1 && *s1 == *s2) { s1++; s2++; }
  if (n == (size_t)-1) return 0;
  return (unsigned char)*s1 - (unsigned char)*s2;
}

char *strchr(const char *s, int c) {
  while (*s) { if (*s == (char)c) return (char *)s; s++; }
  return (c == '\0') ? (char *)s : (void *)0;
}

char *strrchr(const char *s, int c) {
  const char *last = (void *)0;
  do { if (*s == (char)c) last = s; } while (*s++);
  return (char *)last;
}

char *strstr(const char *h, const char *n) {
  if (!*n) return (char *)h;
  for (; *h; h++) {
    const char *p = h, *q = n;
    while (*p && *q && *p == *q) { p++; q++; }
    if (!*q) return (char *)h;
  }
  return (void *)0;
}

void *memcpy(void *dest, const void *src, size_t n) {
  char *d = dest; const char *s = src;
  while (n--) *d++ = *s++;
  return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
  char *d = dest; const char *s = src;
  if (d < s) { while (n--) *d++ = *s++; }
  else { d += n; s += n; while (n--) *--d = *--s; }
  return dest;
}

void *memset(void *s, int c, size_t n) {
  unsigned char *p = s;
  while (n--) *p++ = (unsigned char)c;
  return s;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *a = s1, *b = s2;
  while (n--) { if (*a != *b) return *a - *b; a++; b++; }
  return 0;
}
