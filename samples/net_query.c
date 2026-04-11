#include <stddef.h>
#include <stdint.h>

/* Constants needed for the guest environment */
#define AF_INET 2
#define SOCK_STREAM 1

/* Declarations for the Shim library functions */
long write(int fd, const void *buf, size_t count);
size_t strlen(const char *s);
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);

int socket(int domain, int type, int protocol);
int connect(int sockfd, const void *addr, uint32_t addrlen);
long send(int sockfd, const void *buf, size_t len, int flags);
long recv(int sockfd, void *buf, size_t len, int flags);
int close(int fd);

/* Minimal sockaddr_in for guest */
struct guest_sockaddr_in {
    uint16_t sin_family;
    uint16_t sin_port;
    uint32_t sin_addr;
    char sin_zero[8];
};

/* Helper to convert port to network byte order (Big Endian) */
static inline uint16_t htons(uint16_t hostshort) {
    return (hostshort << 8) | (hostshort >> 8);
}

/* Helper to convert IP string to 32-bit integer (e.g., "127.0.0.1") */
static uint32_t inet_addr(const char *cp) {
    uint32_t addr = 0;
    uint32_t part = 0;
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
    
    /* Swap to network byte order (Big Endian) */
    uint32_t res = 0;
    res |= (addr & 0x000000FF) << 24;
    res |= (addr & 0x0000FF00) << 8;
    res |= (addr & 0x00FF0000) >> 8;
    res |= (addr & 0xFF000000) >> 24;
    return res;
}

int main() {
    const char *msg = "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
    char buffer[1024];

    /* 1. Create socket */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        const char *err = "Error: Failed to create socket\n";
        write(1, err, strlen(err));
        return 1;
    }

    /* 2. Connect to localhost:8000 */
    struct guest_sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8000);
    serv_addr.sin_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, &serv_addr, (uint32_t)sizeof(serv_addr)) < 0) {
        const char *err = "Error: Failed to connect to 127.0.0.1:8000\n";
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
