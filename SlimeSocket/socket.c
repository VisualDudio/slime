#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <errno.h>

#include <map>

#include "types.h"
#include "msg.h"

#define USE_GLOBAL_LOCK
#define SLIME_DEBUG

#ifdef USE_GLOBAL_LOCK
static pthread_mutex_t giant_lock = PTHREAD_MUTEX_INITIALIZER;
#define GLOBAL_LOCK do {                        \
        pthread_mutex_lock(&giant_lock);        \
    } while (0)
#define GLOBAL_UNLOCK do {                      \
        pthread_mutex_unlock(&giant_lock);      \
    } while (0)
#else
#define GLOBAL_LOCK
#define GLOBAL_UNLOCK
#endif

#ifdef SLIME_DEBUG
#define slime_debug(format, ...) do { \
        fprintf(stderr, "slime: " format, ##__VA_ARGS__); \
        fflush(stderr); \
    } while (0)
#define slime_debugln(format, ...) do { \
        fprintf(stderr, "slime: " format "\n", ##__VA_ARGS__); \
        fflush(stderr); \
    } while (0)
#else
#define slime_debug(format, ...)
#define slime_debugln(format, ...)
#endif

#define log_error(format, ...) do { \
        slime_debugln(format, ##__VA_ARGS__); \
        perror("slime"); \
        slime_debugln("  errno: %d", errno); \
    } while (0)


static const char *router_path = "/slime/SlimeRouter.sock";

static struct socket_calls real_socket;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static uint32_t prefix_ip = 0;
static uint32_t prefix_mask = 0;
static int32_t router_socket = 0;

static std::map<int, fd_info_t> fd_info;

static inline bool is_on_overlay(const struct sockaddr_in* addr) {
    struct in_addr addr_tmp;
    addr_tmp.s_addr = prefix_ip;
    struct in_addr mask_tmp;
    mask_tmp.s_addr = prefix_mask;
    slime_debug("is %s ", inet_ntoa(addr->sin_addr));
    slime_debug("on overlay %s", inet_ntoa(addr_tmp));
    slime_debugln("/%s?", inet_ntoa(mask_tmp));
    if (addr->sin_family == AF_INET6) {
        const uint8_t *bytes = ((const struct sockaddr_in6 *)addr)->sin6_addr.s6_addr;
        bytes += 12;
        struct in_addr addr4 = { *(const in_addr_t *)bytes };
        return ((addr4.s_addr & prefix_mask) == (prefix_ip & prefix_mask));
    }
    return ((addr->sin_addr.s_addr & prefix_mask) == (prefix_ip & prefix_mask));
}

void getenv_options(void) {
    const char* prefix = getenv("VNET_PREFIX");
    if (prefix) {
        uint8_t a, b, c, d, bits;
        if (sscanf(prefix, "%hhu.%hhu.%hhu.%hhu/%hhu", &a, &b, &c, &d, &bits) == 5) {
            if (bits <= 32) {
                prefix_ip = htonl((a << 24UL) | (b << 16UL) | (c << 8UL) | (d));
                prefix_mask = htonl((0xFFFFFFFFUL << (32 - bits)) & 0xFFFFFFFFUL);
            }
        }
    }
    if (prefix_ip == 0 && prefix_mask == 0) {
        slime_debugln("WARNING: VNET_PREFIX is not set. Using 0.0.0.0/0.");
        slime_debugln("All connections are treated as virtual network connections.");
    }
}

static int init_preload(void) {
    static int init;
    // quick check without lock
    if (init) {
        return 0;
    }

    pthread_mutex_lock(&mutex);
    // XXX unneeded?
    if (init) {
        goto out;
    }

    real_socket.socket = (int (*)(int, int, int)) dlsym(RTLD_NEXT, "socket");
    real_socket.bind = (int (*)(int, const sockaddr *, socklen_t)) dlsym(RTLD_NEXT, "bind");
    real_socket.listen = (int (*)(int, int)) dlsym(RTLD_NEXT, "listen");
    real_socket.accept = (int (*)(int, sockaddr *, socklen_t *)) dlsym(RTLD_NEXT, "accept");
    real_socket.accept4 = (int (*)(int, sockaddr *, socklen_t *, int)) dlsym(RTLD_NEXT, "accept4");
    real_socket.connect = (int (*)(int, const sockaddr *, socklen_t)) dlsym(RTLD_NEXT, "connect");
    real_socket.getpeername = (int (*)(int, sockaddr *, socklen_t *)) dlsym(RTLD_NEXT, "getpeername");
    real_socket.getsockname = (int (*)(int, sockaddr *, socklen_t *)) dlsym(RTLD_NEXT, "getsockname");
    real_socket.setsockopt = (int (*)(int, int, int, const void *, socklen_t)) dlsym(RTLD_NEXT, "setsockopt");
    real_socket.getsockopt = (int (*)(int, int, int, void *, socklen_t *)) dlsym(RTLD_NEXT, "getsockopt");
    real_socket.fcntl = (int (*)(int, int, ...)) dlsym(RTLD_NEXT, "fcntl");
    real_socket.close = (int (*)(int)) dlsym(RTLD_NEXT, "close");

    getenv_options();
    if (connect_router() < 0) {
        return -1;
    }
    init = 1;
out:
    pthread_mutex_unlock(&mutex);
    return 0;
}

// Returns error only
int connect_router() {
    slime_debugln("connect router...");
    router_socket = real_socket.socket(AF_UNIX, SOCK_STREAM, 0);
    if (router_socket < 0) {
        log_error("Cannot create router socket.\n");
        return -1;
    }
    struct sockaddr_un saun;
    memset(&saun, 0, sizeof(saun));
    saun.sun_family = AF_UNIX;
    strcpy(saun.sun_path, router_path);
    int len = sizeof(saun.sun_family) + strlen(saun.sun_path);
    if (real_socket.connect(router_socket, (struct sockaddr*) &saun, len) < 0) {
        log_error("Cannot connect router. try again");
        real_socket.close(router_socket);
    }
    return 0;
}

msg_t *xfer_msg(msg_t *req, const char *call) {
    uint32_t write_size = sizeof(msg_kind_t) + sizeof(uint32_t) + req->size;
    if (write(router_socket, req, write_size) < write_size) {
        slime_debugln("xfer_msg: %s: write fails", call);
        return NULL;
    }

    uint32_t rem_size = sizeof(msg_kind_t) + sizeof(uint32_t) + sizeof(uint32_t);
    char *buf = (char *) malloc(rem_size);
    char *_buf = buf;
    uint32_t read_size;
    while (rem_size) {
        read_size = read(router_socket, _buf, rem_size);
        if (read_size < 0) {
            log_error("xfer_msg: %s: read fails", call);
            return NULL;
        }
        rem_size -= read_size;
        _buf += read_size;
    }
    msg_t *resp = (msg_t *) buf;
    if (resp->kind != req->kind) {
        slime_debugln("xfer_msg: %s: message kind mismatch: received %x", call, resp->kind);
        return NULL;
    }
    return resp;
}

int _socket(int domain, int type, int protocol) {
    if (init_preload() < 0) {
        return -1;
    }
    GLOBAL_LOCK;

    if ((domain == AF_INET || domain == AF_INET6) && (type & SOCK_STREAM) && (!protocol || protocol == IPPROTO_TCP)) {
        msg_t *socket_req = new_socket_req(domain, type, protocol);
        if (!socket_req) {
            slime_debugln("socket_req creation fails");
            return -1;
        }
        msg_t *resp = xfer_msg(socket_req, "socket");
        if (!resp) {
            return -1;
        }

        int host_socket = ((socket_resp_t *) &resp->body)->host_socket;
        int client_fd = dup(host_socket);
        fd_info_t info = { .normal = false, .host_fd = host_socket };
        fd_info[client_fd] = info;
        free(socket_req);
        free(resp);
        GLOBAL_UNLOCK;
        return client_fd;
    } else {
        int client_fd = real_socket.socket(domain, type, protocol);
        if (client_fd < 0) {
            GLOBAL_UNLOCK;
            return -1;
        }
        fd_info_t info = { .normal = true, .host_fd = 0 };
        fd_info[client_fd] = info;
        GLOBAL_UNLOCK;
        return client_fd;
    }
}

int socket(int domain, int type, int protocol) {
    slime_debugln("socket(%x, %x, %x)", domain, type, protocol);
    int ret = _socket(domain, type, protocol);
    slime_debugln("socket(%x, %x, %x) -> %d", domain, type, protocol, ret);
    return ret;
}

int _bind(int socket, const struct sockaddr *addr, socklen_t addrlen) {
    GLOBAL_LOCK;

    fd_info_t info = fd_info[socket];
    if (info.normal || addr->sa_family != AF_INET) {
        GLOBAL_UNLOCK;
        return real_socket.bind(socket, addr, addrlen);
    }

    struct sockaddr_in *addr_in = (sockaddr_in *) addr;
    uint32_t ip = addr_in->sin_addr.s_addr;
    uint16_t port = addr_in->sin_port;

    if (!is_on_overlay(addr_in) && addr_in->sin_addr.s_addr != INADDR_ANY) {
        // bind on non-overlay address, no need to contact router
        GLOBAL_UNLOCK;
        return real_socket.bind(socket, addr, addrlen);
    }

    msg_t *bind_req = new_bind_req(ip, port);
    if (!bind_req) {
        slime_debugln("bind_req creation fails");
        return -1;
    }
    msg_t *resp = xfer_msg(bind_req, "bind");
    if (!resp) {
        return -1;
    }

    int32_t status = ((bind_resp_t *) resp->body)->status;
    free(bind_req);
    free(resp);
    GLOBAL_UNLOCK;
    return status;
}

int bind(int socket, const struct sockaddr *addr, socklen_t addrlen) {
    slime_debugln("bind(%d, %s, %hu)", socket, inet_ntoa(((struct sockaddr_in*)addr)->sin_addr), htons(((struct sockaddr_in*)addr)->sin_port));
    int ret = _bind(socket, addr, addrlen);
    slime_debugln("bind(%d, %s, %hu) -> %x", socket, inet_ntoa(((struct sockaddr_in*)addr)->sin_addr), htons(((struct sockaddr_in*)addr)->sin_port), ret);
    return ret;
}

int listen(int socket, int backlog) {
    GLOBAL_LOCK;
    slime_debugln("listen(%d, %d)", socket, backlog);

    fd_info_t info = fd_info[socket];
    if (info.normal) {
        GLOBAL_UNLOCK;
        return real_socket.listen(socket, backlog);
    }

    return real_socket.listen(info.host_fd, backlog);
}

int accept(int socket, struct sockaddr *addr, socklen_t *addrlen) {
    return accept4(socket, addr, addrlen, 0);
}

int _accept4(int socket, struct sockaddr *addr, socklen_t *addrlen, int flags) {
    GLOBAL_LOCK;

    fd_info_t info = fd_info[socket];
    if (info.normal || addr->sa_family != AF_INET) {
        GLOBAL_UNLOCK;
        return real_socket.accept4(socket, addr, addrlen, flags);
    }

    struct sockaddr_in *addr_in = (sockaddr_in *) addr;
    uint32_t ip = addr_in->sin_addr.s_addr;
    uint16_t port = addr_in->sin_port;

    if (!is_on_overlay(addr_in) && addr_in->sin_addr.s_addr != INADDR_ANY) {
        GLOBAL_UNLOCK;
        return real_socket.accept4(socket, addr, addrlen, flags);
    }

    msg_t *accept_req = new_accept_req(flags);
    if (!accept_req) {
        slime_debugln("accept_req creation fails");
        return -1;
    }
    msg_t *resp = xfer_msg(accept_req, "accept");
    if (!resp) {
        return -1;
    }

    int32_t csock = ((accept_resp_t *) resp->body)->client_sock;
    free(accept_req);
    free(resp);
    GLOBAL_UNLOCK;
    return csock;
}

int accept4(int socket, struct sockaddr *addr, socklen_t *addrlen, int flags) {
    slime_debugln("accept4(%d, %s, %hu, %x)", socket, inet_ntoa(((struct sockaddr_in*)addr)->sin_addr), htons(((struct sockaddr_in*)addr)->sin_port), flags);
    int ret = _accept4(socket, addr, addrlen, flags);
    slime_debugln("accept4(%d, %s, %hu, %x) -> %x", socket, inet_ntoa(((struct sockaddr_in*)addr)->sin_addr), htons(((struct sockaddr_in*)addr)->sin_port), flags, ret);
    return ret;
}

int _connect(int socket, const struct sockaddr *addr, socklen_t addrlen) {
    GLOBAL_LOCK;

    fd_info_t info = fd_info[socket];
    if (info.normal || addr->sa_family != AF_INET) {
        GLOBAL_UNLOCK;
        return real_socket.connect(socket, addr, addrlen);
    }

    struct sockaddr_in *addr_in = (sockaddr_in *) addr;
    uint32_t ip = addr_in->sin_addr.s_addr;
    uint16_t port = addr_in->sin_port;

    if (!is_on_overlay(addr_in) && addr_in->sin_addr.s_addr != INADDR_ANY) {
        GLOBAL_UNLOCK;
        return real_socket.connect(socket, addr, addrlen);
    }

    msg_t *connect_req = new_connect_req(ip, port);
    if (!connect_req) {
        slime_debugln("connect_req creation fails");
        return -1;
    }
    msg_t *resp = xfer_msg(connect_req, "connect");
    if (!resp) {
        return -1;
    }

    int32_t status = ((connect_resp_t *) resp->body)->status;
    free(connect_req);
    free(resp);
    GLOBAL_UNLOCK;
    return status;
}

int connect(int socket, const struct sockaddr *addr, socklen_t addrlen) {
    slime_debugln("connect(%d, %s, %hu)", socket, inet_ntoa(((struct sockaddr_in*)addr)->sin_addr), htons(((struct sockaddr_in*)addr)->sin_port));
    int ret = _connect(socket, addr, addrlen);
    slime_debugln("connect(%d, %s, %hu) -> %x", socket, inet_ntoa(((struct sockaddr_in*)addr)->sin_addr), htons(((struct sockaddr_in*)addr)->sin_port), ret);
    return ret;
}

/* int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event) { } */
/* int getpeername(int socket, struct sockaddr *addr, socklen_t *addrlen) { } */
/* int getsockname(int socket, struct sockaddr *addr, socklen_t *addrlen) { } */
/* int getsockopt(int socket, int level, int optname, void *optval, socklen_t *optlen) { } */
/* int setsockopt(int socket, int level, int optname, const void *optval, socklen_t optlen) { } */
/* int fcntl(int socket, int cmd, ... /1* arg *1/) { } */

int close(int socket) {
    GLOBAL_LOCK;
    slime_debugln("close(%d)", socket);

    fd_info_t info = fd_info[socket];
    if (info.normal) {
        GLOBAL_UNLOCK;
        return real_socket.close(socket);
    }

    return real_socket.close(info.host_fd);
}
