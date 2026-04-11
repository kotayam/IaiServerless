#include "hypercall.h"
#include "../iai_common.h"
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_GUEST_FDS 1024
static int guest_to_host_fds[MAX_GUEST_FDS];

#define IAI_LOG(op, fmt, ...) \
    do { if (iai_debug) fprintf(stderr, "[HYPER] %-8s " fmt "\n", op, ##__VA_ARGS__); } while (0)

void init_fd_map() {
    for (int i = 0; i < MAX_GUEST_FDS; i++) {
        guest_to_host_fds[i] = -1;
    }
    // Map standard streams
    guest_to_host_fds[0] = STDIN_FILENO;
    guest_to_host_fds[1] = STDOUT_FILENO;
    guest_to_host_fds[2] = STDERR_FILENO;
}

void handle_hypercall(char *mem, uint32_t msg_buf_addr) {
    struct iai_req *req = (struct iai_req *)(mem + msg_buf_addr);
    void *payload = (void *)(mem + msg_buf_addr + sizeof(struct iai_req));

    switch (req->op) {
    case IAI_WRITE: {
        int fd = (req->args[0] < MAX_GUEST_FDS) ? guest_to_host_fds[req->args[0]] : -1;
        if (fd == -1) {
            req->ret = -1;
            req->err = EBADF;
        } else {
            req->ret = write(fd, payload, req->data_len);
            req->err = errno;
        }
        IAI_LOG("WRITE", "guest_fd=%-2d len=%-4u -> ret=%-4d err=%d", req->args[0], req->data_len, req->ret, req->err);
        break;
    }
    case IAI_SOCKET: {
        int host_fd = socket(req->args[0], req->args[1], req->args[2]);
        if (host_fd < 0) {
            req->ret = -1;
            req->err = errno;
        } else {
            int guest_fd = -1;
            for (int i = 3; i < MAX_GUEST_FDS; i++) {
                if (guest_to_host_fds[i] == -1) {
                    guest_fd = i;
                    break;
                }
            }
            if (guest_fd == -1) {
                close(host_fd);
                req->ret = -1;
                req->err = EMFILE;
            } else {
                guest_to_host_fds[guest_fd] = host_fd;
                req->ret = guest_fd;
                req->err = 0;
            }
        }
        IAI_LOG("SOCKET", "domain=%-2d type=%-2d proto=%-2d -> guest_fd=%-2d err=%d", req->args[0], req->args[1], req->args[2], req->ret, req->err);
        break;
    }
    case IAI_CONNECT: {
        int fd = (req->args[0] < MAX_GUEST_FDS) ? guest_to_host_fds[req->args[0]] : -1;
        if (fd == -1) {
            req->ret = -1;
            req->err = EBADF;
        } else {
            req->ret = connect(fd, (struct sockaddr *)payload, req->data_len);
            req->err = errno;
        }
        IAI_LOG("CONNECT", "guest_fd=%-2d addrlen=%-2u -> ret=%-2d err=%d", req->args[0], req->data_len, req->ret, req->err);
        break;
    }
    case IAI_SEND: {
        int fd = (req->args[0] < MAX_GUEST_FDS) ? guest_to_host_fds[req->args[0]] : -1;
        if (fd == -1) {
            req->ret = -1;
            req->err = EBADF;
        } else {
            req->ret = send(fd, payload, req->data_len, req->args[1]);
            req->err = errno;
        }
        IAI_LOG("SEND", "guest_fd=%-2d len=%-4u flags=%-2d -> ret=%-4d err=%d", req->args[0], req->data_len, req->args[1], req->ret, req->err);
        break;
    }
    case IAI_RECV: {
        int fd = (req->args[0] < MAX_GUEST_FDS) ? guest_to_host_fds[req->args[0]] : -1;
        if (fd == -1) {
            req->ret = -1;
            req->err = EBADF;
        } else {
            req->ret = recv(fd, payload, req->args[1], req->args[2]);
            req->err = errno;
        }
        IAI_LOG("RECV", "guest_fd=%-2d maxlen=%-4u flags=%-2d -> ret=%-4d err=%d", req->args[0], req->args[1], req->args[2], req->ret, req->err);
        break;
    }
    case IAI_CLOSE: {
        int fd = (req->args[0] < MAX_GUEST_FDS) ? guest_to_host_fds[req->args[0]] : -1;
        if (fd == -1) {
            req->ret = -1;
            req->err = EBADF;
        } else {
            req->ret = close(fd);
            req->err = errno;
            guest_to_host_fds[req->args[0]] = -1;
        }
        IAI_LOG("CLOSE", "guest_fd=%-2d -> ret=%-2d err=%d", req->args[0], req->ret, req->err);
        break;
    }
    case IAI_GETHOSTBYNAME: {
        struct hostent *he = gethostbyname((char *)payload);
        if (!he) {
            req->ret = -1;
            req->err = h_errno;
        } else {
            /* Copy the first IPv4 address (4 bytes) back to guest buffer */
            memcpy(payload, he->h_addr_list[0], 4);
            req->ret = 0;
            req->err = 0;
        }
        IAI_LOG("GETHOST", "hostname=%s -> ret=%d", (char *)payload, req->ret);
        break;
    }
    default:
        req->ret = -1;
        req->err = ENOSYS;
        IAI_LOG("UNKNOWN", "op=%u", req->op);
        break;
    }
}
