#ifndef SHIM_STDIO_H
#define SHIM_STDIO_H

#include <stdarg.h>
#include <stddef.h>

int printf(const char *fmt, ...);
int snprintf(char *str, size_t size, const char *fmt, ...);
int vsnprintf(char *str, size_t size, const char *fmt, va_list ap);
int putchar(int c);

#endif /* SHIM_STDIO_H */
