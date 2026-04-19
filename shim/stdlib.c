#include <stddef.h>
#include <stdint.h>

static char *current_brk = NULL;
static size_t guest_mem_size = 0;
extern char _end;

void _init_memory(size_t mem_size) {
  guest_mem_size = mem_size;
  current_brk = &_end;
}

int brk(void *addr) {
  if (!current_brk)
    current_brk = &_end;

  if (addr < (void *)&_end)
    return -1;
  if (addr >= (void *)(size_t)guest_mem_size)
    return -1;

  current_brk = addr;
  return 0;
}

void *sbrk(long increment) {
  if (!current_brk)
    current_brk = &_end;

  char *new_brk = current_brk + increment;

  if (new_brk < &_end)
    return (void *)-1;
  if (new_brk >= (char *)(size_t)guest_mem_size)
    return (void *)-1;

  char *old_brk = current_brk;
  current_brk = new_brk;
  return old_brk;
}

// Simple free-list allocator
#define BLOCK_MAGIC 0xDEADBEEF

struct block {
  uint32_t magic;
  size_t size;
  struct block *next;
  int free;
};

#define BLOCK_SIZE sizeof(struct block)

static struct block *free_list = NULL;

void *malloc(size_t size) {
  if (size == 0)
    return NULL;

  // Align size to 8 bytes
  size = (size + 7) & ~7;

  // Search free list for suitable block
  struct block *curr = free_list;
  struct block *prev = NULL;

  while (curr) {
    if (curr->free && curr->size >= size) {
      curr->free = 0;
      return (void *)(curr + 1);
    }
    prev = curr;
    curr = curr->next;
  }

  // No suitable block found, request more memory
  struct block *new_block = sbrk(BLOCK_SIZE + size);
  if (new_block == (void *)-1)
    return NULL;

  new_block->magic = BLOCK_MAGIC;
  new_block->size = size;
  new_block->next = NULL;
  new_block->free = 0;

  // Add to free list
  if (prev) {
    prev->next = new_block;
  } else {
    free_list = new_block;
  }

  return (void *)(new_block + 1);
}

void free(void *ptr) {
  if (!ptr)
    return;

  struct block *block = (struct block *)ptr - 1;

  // Validate pointer is within heap bounds
  if ((char *)block < &_end || (char *)block >= current_brk)
    return;

  // Validate magic number (detects misaligned/corrupted pointers)
  if (block->magic != BLOCK_MAGIC)
    return;

  // Check if already freed (double-free detection)
  if (block->free)
    return;

  block->free = 1;
}

void *realloc(void *ptr, size_t size) {
  if (!ptr)
    return malloc(size);
  if (size == 0) {
    free(ptr);
    return NULL;
  }

  struct block *block = (struct block *)ptr - 1;
  if (block->magic != BLOCK_MAGIC)
    return NULL;

  if (block->size >= size)
    return ptr;

  void *new_ptr = malloc(size);
  if (!new_ptr)
    return NULL;

  size_t copy_size = block->size < size ? block->size : size;
  char *dst = new_ptr, *src = ptr;
  for (size_t i = 0; i < copy_size; i++)
    dst[i] = src[i];

  free(ptr);
  return new_ptr;
}

void abort(void) {
  asm volatile("hlt");
  __builtin_unreachable();
}

void exit(int status) {
  (void)status;
  asm volatile("hlt");
  __builtin_unreachable();
}

int atoi(const char *s) {
  int n = 0, neg = 0;
  while (*s == ' ' || *s == '\t')
    s++;
  if (*s == '-') {
    neg = 1;
    s++;
  } else if (*s == '+')
    s++;
  while (*s >= '0' && *s <= '9')
    n = n * 10 + (*s++ - '0');
  return neg ? -n : n;
}

long strtol(const char *s, char **endptr, int base) {
  long n = 0, neg = 0;
  while (*s == ' ' || *s == '\t')
    s++;
  if (*s == '-') {
    neg = 1;
    s++;
  } else if (*s == '+')
    s++;
  if (base == 0) {
    if (*s == '0' && (s[1] == 'x' || s[1] == 'X')) {
      base = 16;
      s += 2;
    } else if (*s == '0') {
      base = 8;
      s++;
    } else
      base = 10;
  }
  while (*s) {
    int digit;
    if (*s >= '0' && *s <= '9')
      digit = *s - '0';
    else if (*s >= 'a' && *s <= 'z')
      digit = *s - 'a' + 10;
    else if (*s >= 'A' && *s <= 'Z')
      digit = *s - 'A' + 10;
    else
      break;
    if (digit >= base)
      break;
    n = n * base + digit;
    s++;
  }
  if (endptr)
    *endptr = (char *)s;
  return neg ? -n : n;
}

// Alias for newer glibc versions
long __isoc23_strtol(const char *s, char **endptr, int base)
    __attribute__((alias("strtol")));
