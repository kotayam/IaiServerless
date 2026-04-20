#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  const char *hostname = "api.open-meteo.com";
  const char *path =
      "/v1/forecast?latitude=35.6895&longitude=139.6917&current_weather=true";
  char request[512];
  char buffer[1024];

  struct hostent *he = gethostbyname(hostname);
  if (!he) { write(1, "Error: DNS failed\n", 18); return 1; }

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) return 2;

  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(80);
  memcpy(&serv_addr.sin_addr, he->h_addr_list[0], he->h_length);

  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    write(1, "Error: connect failed\n", 22);
    close(sockfd);
    return 3;
  }

  int req_len = 0;
  const char *p;
  for (p = "GET ";                          *p;) request[req_len++] = *p++;
  for (p = path;                            *p;) request[req_len++] = *p++;
  for (p = " HTTP/1.1\r\nHost: ";           *p;) request[req_len++] = *p++;
  for (p = hostname;                        *p;) request[req_len++] = *p++;
  for (p = "\r\nConnection: close\r\n\r\n"; *p;) request[req_len++] = *p++;
  send(sockfd, request, req_len, 0);

  long n;
  while ((n = recv(sockfd, buffer, sizeof(buffer), 0)) > 0)
    write(1, buffer, (size_t)n);

  close(sockfd);
  return 0;
}
