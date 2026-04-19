#ifndef SHIM_STDIO_H
#define SHIM_STDIO_H

#include <stdarg.h>
#include <stddef.h>

// Dummy FILE structure for stdout/stderr
typedef struct {
  int fd;
} FILE;

extern FILE *stdout;
extern FILE *stderr;

int printf(const char *fmt, ...);
int snprintf(char *str, size_t size, const char *fmt, ...);
int vsnprintf(char *str, size_t size, const char *fmt, va_list ap);
int putchar(int c);
int putc(int c, FILE *stream);
int fputc(int c, FILE *stream);

#endif /* SHIM_STDIO_H */
