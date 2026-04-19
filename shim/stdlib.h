#ifndef SHIM_STDLIB_H
#define SHIM_STDLIB_H

#include <stddef.h>

void _init_memory(size_t mem_size);
int brk(void *addr);
void *sbrk(long increment);
void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);
void abort(void) __attribute__((noreturn));
void exit(int status) __attribute__((noreturn));
int atoi(const char *s);
long strtol(const char *s, char **endptr, int base);

// alloca - stack allocation (compiler builtin)
#define alloca(size) __builtin_alloca(size)

#endif /* SHIM_STDLIB_H */
