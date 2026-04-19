#include "../iai_common.h"
#include <stddef.h>
#include <stdint.h>

#define MESSAGE_BUFFER_SIZE 4096

static char message_buffer[MESSAGE_BUFFER_SIZE];

// 32-bit hardware port interaction
static inline void outl(uint16_t port, uint32_t value) {
  asm volatile("outl %0, %1" : : "a"(value), "Nd"(port) : "memory");
}

// get length of string
size_t strlen(const char *s) {
  size_t len = 0;
  while (s[len] != '\0') {
    len++;
  }
  return len;
}

// copy src to dest
void *memcpy(void *dest, const void *src, size_t n) {
  char *d = dest;
  const char *s = src;
  while (n--) {
    *d++ = *s++;
  }
  return dest;
}

// set memory
void *memset(void *s, int c, size_t n) {
  unsigned char *p = s;
  while (n--) {
    *p++ = (unsigned char)c;
  }
  return s;
}

static long hypercall(uint32_t op, uint32_t arg0, uint32_t arg1, uint32_t arg2,
                      uint32_t arg3, const void *payload,
                      uint32_t payload_len) {
  struct iai_req *req = (struct iai_req *)message_buffer;
  req->op = op;
  req->args[0] = arg0;
  req->args[1] = arg1;
  req->args[2] = arg2;
  req->args[3] = arg3;
  req->data_len = payload_len;

  if (payload && payload_len > 0) {
    if (payload_len + sizeof(struct iai_req) > MESSAGE_BUFFER_SIZE) {
      return -1;
    }
    memcpy(message_buffer + sizeof(struct iai_req), payload, payload_len);
  }

  // tell host the address of buffer
  outl(SHM_ADDR_PORT, (uint32_t)(uintptr_t)message_buffer);
  // trigger the host hypercall
  outl(HYPERCALL_PORT, 0);

  return (long)req->ret;
}

// intercept write
long write(int fd, const void *buf, size_t count) {
  return hypercall(IAI_WRITE, (uint32_t)fd, 0, 0, 0, buf, (uint32_t)count);
}

int socket(int domain, int type, int protocol) {
  return (int)hypercall(IAI_SOCKET, (uint32_t)domain, (uint32_t)type,
                        (uint32_t)protocol, 0, NULL, 0);
}

int connect(int sockfd, const void *addr, uint32_t addrlen) {
  return (int)hypercall(IAI_CONNECT, (uint32_t)sockfd, 0, 0, 0, addr, addrlen);
}

long send(int sockfd, const void *buf, size_t len, int flags) {
  return hypercall(IAI_SEND, (uint32_t)sockfd, (uint32_t)flags, 0, 0, buf,
                   (uint32_t)len);
}

long recv(int sockfd, void *buf, size_t len, int flags) {
  long ret = hypercall(IAI_RECV, (uint32_t)sockfd, (uint32_t)len,
                       (uint32_t)flags, 0, NULL, 0);
  if (ret > 0) {
    memcpy(buf, message_buffer + sizeof(struct iai_req), (size_t)ret);
  }
  return ret;
}

int close(int fd) {
  return (int)hypercall(IAI_CLOSE, (uint32_t)fd, 0, 0, 0, NULL, 0);
}

uint32_t inet_addr(const char *cp) {
  uint32_t addr = 0, part = 0;
  while (*cp) {
    if (*cp == '.') {
      addr = (addr << 8) | (part & 0xFF);
      part = 0;
    } else {
      part = part * 10 + (*cp - '0');
    }
    cp++;
  }
  addr = (addr << 8) | (part & 0xFF);
  /* convert to network byte order */
  return ((addr & 0xFF) << 24) | ((addr & 0xFF00) << 8) |
         ((addr >> 8) & 0xFF00) | ((addr >> 24) & 0xFF);
}

int gethostbyname_r(const char *name, uint32_t *addr) {
  long ret = hypercall(IAI_GETHOSTBYNAME, 0, 0, 0, 0, name,
                       (uint32_t)strlen(name) + 1);
  if (ret == 0) {
    memcpy(addr, message_buffer + sizeof(struct iai_req), 4);
  }
  return (int)ret;
}

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

struct hostent {
  char *h_name;
  char **h_aliases;
  int h_addrtype;
  int h_length;
  char **h_addr_list;
};

struct hostent *gethostbyname(const char *name) {
  static uint32_t addr;
  static char *addr_ptr = (char *)&addr;
  static char *aliases = NULL;
  static struct hostent he = {
      .h_addrtype = 2, /* AF_INET */
      .h_length = 4,
      .h_aliases = &aliases,
      .h_addr_list = &addr_ptr,
  };
  if (gethostbyname_r(name, &addr) < 0)
    return (void *)0;
  return &he;
}
