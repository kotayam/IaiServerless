#include "../iai_common.h"
#include "hypercall.h"
#include "string.h"
#include <stddef.h>
#include <stdint.h>

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
