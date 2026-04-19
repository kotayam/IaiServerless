#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  const char *msg =
      "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
  char buffer[1024];

  /* 1. Create socket */
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    const char *err = "Error: Failed to create socket\n";
    write(1, err, strlen(err));
    return 1;
  }

  /* 2. Connect to localhost:8000 */
  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(8000);
  serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    const char *err = "Error: Failed to connect to 127.0.0.1:8001\n";
    write(1, err, strlen(err));
    close(sockfd);
    return 2;
  }

  /* 3. Send HTTP Request */
  if (send(sockfd, msg, strlen(msg), 0) < 0) {
    const char *err = "Error: Failed to send data\n";
    write(1, err, strlen(err));
    close(sockfd);
    return 3;
  }

  /* 4. Receive Response in a loop */
  long n;
  int received_any = 0;
  while ((n = recv(sockfd, buffer, sizeof(buffer), 0)) > 0) {
    write(1, buffer, (size_t)n);
    received_any = 1;
  }

  if (n < 0) {
    const char *err = "\nError: recv failed\n";
    write(1, err, strlen(err));
    close(sockfd);
    return 4;
  }

  if (!received_any) {
    const char *err = "\nError: No data received from host\n";
    write(1, err, strlen(err));
    close(sockfd);
    return 5;
  }

  /* 5. Clean up */
  close(sockfd);
  return 0;
}
