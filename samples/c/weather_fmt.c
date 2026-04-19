#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static int http_get(struct in_addr *addr, int port, const char *host,
                    const char *path, char *buf, int bufsz) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) return -1;
  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  memcpy(&sa.sin_addr, addr, sizeof(*addr));
  if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) { close(fd); return -2; }
  char req[512]; int rlen = 0;
  const char *p;
  for (p = "GET ";                          *p;) req[rlen++] = *p++;
  for (p = path;                            *p;) req[rlen++] = *p++;
  for (p = " HTTP/1.1\r\nHost: ";           *p;) req[rlen++] = *p++;
  for (p = host;                            *p;) req[rlen++] = *p++;
  for (p = "\r\nConnection: close\r\n\r\n"; *p;) req[rlen++] = *p++;
  send(fd, req, rlen, 0);
  int total = 0; long n;
  while ((n = recv(fd, buf + total, bufsz - total - 1, 0)) > 0) total += n;
  buf[total] = '\0';
  close(fd);
  return total;
}

static void http_post(int port, const char *path,
                      const char *body, int blen) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) return;
  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr.s_addr = 0x0100007f; /* 127.0.0.1 */
  if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) { close(fd); return; }

  /* Content-Length digits */
  char clen[12]; int ci = 0, v = blen;
  if (!v) { clen[ci++] = '0'; }
  else { while (v) { clen[ci++] = '0' + v % 10; v /= 10; } }
  /* reverse */
  for (int i = 0, j = ci-1; i < j; i++, j--) {
    char t = clen[i]; clen[i] = clen[j]; clen[j] = t;
  }
  clen[ci] = '\0';

  char hdr[256]; int hlen = 0;
  const char *p;
  for (p = "POST ";                                          *p;) hdr[hlen++] = *p++;
  for (p = path;                                             *p;) hdr[hlen++] = *p++;
  for (p = " HTTP/1.1\r\nHost: localhost\r\nContent-Length: "; *p;) hdr[hlen++] = *p++;
  for (p = clen;                                             *p;) hdr[hlen++] = *p++;
  for (p = "\r\nConnection: close\r\n\r\n";                 *p;) hdr[hlen++] = *p++;
  send(fd, hdr, hlen, 0);
  send(fd, body, blen, 0);

  char resp[2048]; int total = 0; long n;
  while ((n = recv(fd, resp + total, sizeof(resp) - total - 1, 0)) > 0) total += n;
  resp[total] = '\0';
  close(fd);

  const char *start = strstr(resp, "\r\n\r\n");
  if (start) {
    start += 4;
    /* skip chunked size line */
    if (*start && *start != 'T') {
      const char *nl = strchr(start, '\n');
      if (nl) start = nl + 1;
    }
    write(1, start, strlen(start));
  }
}

int main() {
  /* Read gateway port from stdin */
  char port_buf[16]; int plen = 0; long n;
  while (plen < 15 && (n = read(0, port_buf + plen, 15 - plen)) > 0) plen += n;
  port_buf[plen] = '\0';
  /* parse int */
  int gw_port = 0;
  for (int i = 0; port_buf[i] >= '0' && port_buf[i] <= '9'; i++)
    gw_port = gw_port * 10 + (port_buf[i] - '0');
  if (gw_port <= 0) gw_port = 8082;

  const char *hostname = "api.open-meteo.com";
  const char *path =
      "/v1/forecast?latitude=35.6895&longitude=139.6917&current_weather=true";
  char response[2048];

  struct hostent *he = gethostbyname(hostname);
  if (!he) { write(1, "Error: DNS failed\n", 18); return 1; }
  struct in_addr addr;
  memcpy(&addr, he->h_addr_list[0], sizeof(addr));

  if (http_get(&addr, 80, hostname, path, response, sizeof(response)) < 0) {
    write(1, "Error: fetch failed\n", 20); return 2;
  }

  const char *json = strstr(response, "\r\n\r\n");
  if (!json) { write(1, "Error: no body\n", 15); return 3; }
  json += 4;
  if (*json != '{') {
    const char *nl = strchr(json, '\n');
    if (nl) json = nl + 1;
  }

  write(1, "HTTP/1.1 200 OK\r\n\r\n", 19);
  http_post(gw_port, "/python/weather_parser", json, strlen(json));
  return 0;
}
