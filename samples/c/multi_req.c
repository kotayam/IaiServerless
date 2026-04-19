#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/* Three locations: Tokyo, London, New York */
static const char *PATHS[] = {
    "/v1/forecast?latitude=35.6895&longitude=139.6917&current_weather=true",
    "/v1/forecast?latitude=51.5074&longitude=-0.1278&current_weather=true",
    "/v1/forecast?latitude=40.7128&longitude=-74.0060&current_weather=true",
};
#define NUM_REQS 3

static int do_request(const char *hostname, struct in_addr *addr,
                      const char *path, int idx) {
    char request[512];
    char buffer[1024];

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return -1;

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(80);
    memcpy(&serv_addr.sin_addr, addr, sizeof(*addr));

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sockfd);
        return -2;
    }

    int req_len = 0;
    const char *p;
    for (p = "GET ";            *p; ) request[req_len++] = *p++;
    for (p = path;              *p; ) request[req_len++] = *p++;
    for (p = " HTTP/1.1\r\nHost: "; *p; ) request[req_len++] = *p++;
    for (p = hostname;          *p; ) request[req_len++] = *p++;
    for (p = "\r\nConnection: close\r\n\r\n"; *p; ) request[req_len++] = *p++;

    send(sockfd, request, req_len, 0);

    char label[32];
    int llen = snprintf(label, sizeof(label), "\n[req %d] ", idx + 1);
    write(1, label, llen);

    long n;
    while ((n = recv(sockfd, buffer, sizeof(buffer), 0)) > 0)
        write(1, buffer, (size_t)n);

    close(sockfd);
    return 0;
}

int main() {
    const char *hostname = "api.open-meteo.com";

    struct hostent *he = gethostbyname(hostname);
    if (!he) {
        const char *err = "Error: Failed to resolve hostname\n";
        write(1, err, strlen(err));
        return 1;
    }

    write(1, "HTTP/1.1 200 OK\r\n\r\n", 19);

    for (int i = 0; i < NUM_REQS; i++) {
        if (do_request(hostname, (struct in_addr *)he->h_addr_list[0],
                       PATHS[i], i) < 0) {
            const char *err = "Error: request failed\n";
            write(1, err, strlen(err));
            return 2;
        }
    }

    write(1, "\n", 1);
    return 0;
}
