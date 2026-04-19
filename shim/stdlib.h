#ifndef SHIM_STDLIB_H
#define SHIM_STDLIB_H

#include <stddef.h>

void _init_memory(size_t mem_size);
int brk(void *addr);
void *sbrk(long increment);
void *malloc(size_t size);
void free(void *ptr);

#endif /* SHIM_STDLIB_H */
