#ifndef SHIM_NETDB_H
#define SHIM_NETDB_H

#include <stdint.h>

struct hostent {
    char  *h_name;
    char **h_aliases;
    int    h_addrtype;
    int    h_length;
    char **h_addr_list;
};

struct hostent *gethostbyname(const char *name);
int gethostbyname_r(const char *name, uint32_t *addr);
uint32_t inet_addr(const char *cp);

#endif /* SHIM_NETDB_H */
