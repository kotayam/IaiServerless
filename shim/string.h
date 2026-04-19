#ifndef SHIM_STRING_H
#define SHIM_STRING_H

#include <stddef.h>

size_t strlen(const char *s);
char *strcpy(char *dest, const char *src);
char *strcat(char *dest, const char *src);
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);

#endif /* SHIM_STRING_H */
