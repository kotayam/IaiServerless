#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  const char *hostname = "open-meteo.com";
  /* Tokyo Latitude: 35.6895, Longitude: 139.6917 */
  const char *path =
      "/v1/forecast?latitude=35.6895&longitude=139.6917&current_weather=true";
  char request[512];
  char buffer[1024];

  /* 1. Resolve hostname */
  struct hostent *he = gethostbyname(hostname);
  if (!he) {
    const char *err = "Error: Failed to resolve hostname\n";
    write(1, err, strlen(err));
    return 1;
  }

  /* 2. Create socket */
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    return 2;

  /* 3. Connect to API server (Port 80) */
  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(80);
  memcpy(&serv_addr.sin_addr, he->h_addr_list[0], he->h_length);

  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    const char *err = "Error: Failed to connect to API server\n";
    write(1, err, strlen(err));
    close(sockfd);
    return 3;
  }

  /* 4. Construct and send HTTP GET Request */
  int req_len = 0;
  const char *req_p1 = "GET ";
  while (*req_p1)
    request[req_len++] = *req_p1++;
  const char *req_path = path;
  while (*req_path)
    request[req_len++] = *req_path++;
  const char *req_p2 = " HTTP/1.1\r\nHost: ";
  while (*req_p2)
    request[req_len++] = *req_p2++;
  const char *req_host = hostname;
  while (*req_host)
    request[req_len++] = *req_host++;
  const char *req_p3 = "\r\nConnection: close\r\n\r\n";
  while (*req_p3)
    request[req_len++] = *req_p3++;

  send(sockfd, request, req_len, 0);

  /* 5. Receive and print response */
  long n;
  while ((n = recv(sockfd, buffer, sizeof(buffer), 0)) > 0) {
    write(1, buffer, (size_t)n);
  }

  close(sockfd);
  return 0;
}
