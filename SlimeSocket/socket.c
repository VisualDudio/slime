// #define SECURITY
#define _GNU_SOURCE

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
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <errno.h>

#include <map>

#include "types.h"
#include "msg.h"

#define USE_GLOBAL_LOCK 1

#define _ADD_REF(x) &##x
#define ADD_REF(x) _ADD_REF(x)

#ifdef USE_GLOBAL_LOCK
#define RET_W_UNLOCK(lock_name, ret_val) do {           \
        pthread_mutex_unlock(ADD_REF(lock_name));       \
        return ret_val;                                 \
    } while (0)                                         
#else
#define RET_W_UNLOCK(lock_name, ret_val) do {           \
        return ret_val;                                 \
    } while (0)                                         
#endif

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
        fprintf(stderr, format, ##__VA_ARGS__); \
        fflush(stderr); \
    } while (0)
#define slime_debugln(format, ...) do { \
        fprintf(stderr, format "\n", ##__VA_ARGS__); \
        fflush(stderr); \
    } while (0)
#else
#define slime_debug(format, ...)
#define slime_debugln(format, ...)
#endif

#define log_error(format, ...) do { \
        slime_debugln(format, ##__VA_ARGS__); \
        slime_debugln("  errno: %d", errno); \
    } while (0)

int fd_to_epoll_fd[65536];
struct epoll_event epoll_events[65536];

static struct epoll_calls real_epoll;
static struct socket_calls real_socket;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static const char *router_path = "/slime/router/slime_router.sock";

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

    real_socket.socket = dlsym(RTLD_NEXT, "socket");
    real_socket.bind = dlsym(RTLD_NEXT, "bind");
    real_socket.listen = dlsym(RTLD_NEXT, "listen");
    real_socket.accept = dlsym(RTLD_NEXT, "accept");
    real_socket.accept4 = dlsym(RTLD_NEXT, "accept4");
    real_socket.connect = dlsym(RTLD_NEXT, "connect");
    real_socket.getpeername = dlsym(RTLD_NEXT, "getpeername");
    real_socket.getsockname = dlsym(RTLD_NEXT, "getsockname");
    real_socket.setsockopt = dlsym(RTLD_NEXT, "setsockopt");
    real_socket.getsockopt = dlsym(RTLD_NEXT, "getsockopt");
    real_socket.fcntl = dlsym(RTLD_NEXT, "fcntl");
    real_socket.close = dlsym(RTLD_NEXT, "close");
    real_epoll.epoll_ctl = dlsym(RTLD_NEXT, "epoll_ctl");

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
    if (real_socket.connect(unix_sock, (struct sockaddr*) &saun, len) < 0) {
        log_error("Cannot connect router. try again");
        real_socket.close(unix_sock);
    }
    return 0;
}

int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event) {
    GLOBAL_LOCK;
    slime_debugln("epoll_ctl(%d, %d, %d)", epfd, op, fd);

    switch (op) {
        case EPOLL_CTL_ADD:
            fd_to_epoll_fd[fd] = epfd;
            epoll_events[fd] = *event;
            break;
        case EPOLL_CTL_DEL:
            fd_to_epoll_fd[fd] = 0;
    }
    GLOBAL_UNLOCK;
    return real_epoll.epoll_ctl(epfd, op, fd, event);
}

msg_t *xfer_msg(msg_t *req, const char *call) {
    uint32_t write_size = sizeof(msg_kind_t) + sizeof(uint32_t) + req->size;
    if (write(router_socket, req, write_size) < write_size) {
        slime_debugln("xfer_msg: %s: write fails", call);
        return NULL;
    }

    uint32_t rem_size = sizeof(msg_kind_t) + sizeof(uint32_t) + sizeof(uint32_t);
    char *buf = malloc(rem_size);
    char *_buf = buf;
    uint32_t read_size;
    while (rem_size) {
        read_size = read(unix_sock, _buf, rem_size);
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

int socket(int domain, int type, int protocol) {
    if (init_preload() < 0) {
        return -1;
    }
    GLOBAL_LOCK;

    /* struct fd_info *fdi = calloc(1, sizeof(*fdi)); */
    /* int overlay_fd = real_socket.socket(domain, type, protocol); */
    /* fd_to_epoll_fd[overlay_fd] = 0; */

    slime_debugln("socket(%d, %d, %d) --> %d", domain, type, protocol, overlay_fd);

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

        int client_fd = dup(ret_fd);
        fd_info_t info = { .normal = false, .host_fd = ((socket_resp_t *) &resp->body)->host_socket };
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

int bind(int socket, const struct sockaddr *addr, socklen_t addrlen) {
    GLOBAL_LOCK;
    slime_debugln("bind(%d, %s, %hu)", socket, inet_ntoa(((struct sockaddr_in*)addr)->sin_addr), htons(((struct sockaddr_in*)addr)->sin_port));

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

int listen(int socket, int backlog)
{
    GLOBAL_LOCK;
    slime_debugln("listen(%d, %d)", socket, backlog);

    fd_info_t info = fd_info[socket];
    if (info.normal) {
        GLOBAL_UNLOCK;
        return real_socket.listen(socket, backlog);
    }

    msg_t *listen_req = new_listen_req(socket, backlog);
    if (!listen_req) {
        slime_debugln("listen_req creation fails");
        return -1;
    }
    msg_t *resp = xfer_msg(listen_req, "listen");
    if (!resp) {
        return -1;
    }

    // TODO
    int32_t status = ((bind_resp_t *) resp->body)->status;
    free(bind_req);
    free(resp);
    GLOBAL_UNLOCK;
    return status;
}

int accept(int socket, struct sockaddr *addr, socklen_t *addrlen) {
    accept4(socket, addr, addrlen, 0);
}

int accept4(int socket, struct sockaddr *addr, socklen_t *addrlen, int flags) {
    GLOBAL_LOCK;
    slime_debugln("accept4(%d, %s, %hu, %x)", socket, inet_ntoa(((struct sockaddr_in*)addr)->sin_addr), htons(((struct sockaddr_in*)addr)->sin_port), flags);

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

    msg_t *accept_req = new_accept(flags);
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

int connect(int socket, const struct sockaddr *addr, socklen_t addrlen) {
    GLOBAL_LOCK;
    slime_debugln("connect(%d, %s, %hu)", socket, inet_ntoa(((struct sockaddr_in*)addr)->sin_addr), htons(((struct sockaddr_in*)addr)->sin_port));

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

int getpeername(int socket, struct sockaddr *addr, socklen_t *addrlen) {
    GLOBAL_LOCK;
    if (fd_get_type(socket) == fd_normal) {
        GLOBAL_UNLOCK;
        return real_socket.getpeername(socket, addr, addrlen);
    }

    int overlay_fd = fd_get_overlay_fd(socket);
    GLOBAL_UNLOCK;
    return real_socket.getpeername(overlay_fd, addr, addrlen);
}

int getsockname(int socket, struct sockaddr *addr, socklen_t *addrlen) {
    GLOBAL_LOCK;
    if (fd_get_type(socket) == fd_normal) {
        GLOBAL_UNLOCK;
        return real_socket.getsockname(socket, addr, addrlen);
    }

    int overlay_fd = fd_get_overlay_fd(socket);
    GLOBAL_UNLOCK;
    return real_socket.getsockname(overlay_fd, addr, addrlen);
}

int getsockopt(int socket, int level, int optname, void *optval, socklen_t *optlen) {
    GLOBAL_LOCK;
    if (fd_get_type(socket) == fd_normal) {
        GLOBAL_UNLOCK;
        return real_socket.getsockopt(socket, level, optname, optval, optlen);
    }
    int overlay_fd = fd_get_overlay_fd(socket);
    GLOBAL_UNLOCK;
    return real_socket.getsockopt(overlay_fd, level, optname, optval, optlen);
}

int setsockopt(int socket, int level, int optname, const void *optval, socklen_t optlen) {
    GLOBAL_LOCK;
    slime_debugln("setsockopt(%d, %d, %d)", socket, level, optname);

    if (fd_get_type(socket) == fd_normal) {
        GLOBAL_UNLOCK;
        return real_socket.setsockopt(socket, level, optname, optval, optlen);
    }

    int overlay_fd = fd_get_overlay_fd(socket);
    int ret = real_socket.setsockopt(overlay_fd, level, optname, optval, optlen);
    if (optname != SO_REUSEPORT && optname != SO_REUSEADDR && level != IPPROTO_IPV6) {
        int host_fd = fd_get_host_fd(socket);
        ret = real_socket.setsockopt(host_fd, level, optname, optval, optlen);
    }
    GLOBAL_UNLOCK;
    return ret;
}

int fcntl(int socket, int cmd, ... /* arg */) {
    GLOBAL_LOCK;
    va_list args;
    long lparam;
    void *pparam;
    int ret;
    bool normal;
    int overlay_fd = socket, host_fd = socket;

    if (init_preload() < 0) {
        return -1;
    }
    if (fd_get_type(socket) == fd_normal) {
        normal = 1;
    }
    else {
        normal = 0;
        overlay_fd = fd_get_overlay_fd(socket);
        host_fd = fd_get_host_fd(socket);
    }

    // TODO: need to check whether it's a socket here

    va_start(args, cmd);
    switch (cmd) {
    case F_GETFD:
    case F_GETFL:
    case F_GETOWN:
    case F_GETSIG:
    case F_GETLEASE:
        if (normal) {
            ret = real_socket.fcntl(socket, cmd);
        }
        else {
            ret = real_socket.fcntl(host_fd, cmd);
            if (overlay_fd != host_fd) {
                real_socket.fcntl(overlay_fd, cmd);
            }
        }
        break;
    case F_DUPFD:
    /*case F_DUPFD_CLOEXEC:*/
    case F_SETFD:
    case F_SETFL:
    case F_SETOWN:
    case F_SETSIG:
    case F_SETLEASE:
    case F_NOTIFY:
        lparam = va_arg(args, long);
        if (normal) {
            ret = real_socket.fcntl(socket, cmd, lparam);
        }
        else {
            ret = real_socket.fcntl(host_fd, cmd, lparam);
            if (overlay_fd != host_fd) {
                real_socket.fcntl(overlay_fd, cmd, lparam);
            }
        }
        break;
    default:
        pparam = va_arg(args, void *);
        if (normal) {
            ret = real_socket.fcntl(socket, cmd, pparam);
        }
        else {
            ret = real_socket.fcntl(host_fd, cmd, pparam);
            if (overlay_fd != host_fd) {
                real_socket.fcntl(overlay_fd, cmd, pparam);
            }
        }
        break;
    }
    va_end(args);
    GLOBAL_UNLOCK;
    return ret;
}

int close(int socket) {
    GLOBAL_LOCK;
    struct fd_info *fdi;
    int ret;

    if (init_preload() < 0) {
        return -1;
    }
    if (fd_get_type(socket) == fd_normal) {
        fdi = idm_lookup(&idm, socket);
        if (fdi) {
            free(fdi);
            idm_clear(&idm, socket);
        }
        GLOBAL_UNLOCK;
        return real_socket.close(socket);
    }

    int overlay_fd = fd_get_overlay_fd(socket);
    int host_fd = fd_get_host_fd(socket);

    slime_debugln("close(%d)", socket);

    fdi = idm_lookup(&idm, socket);
    idm_clear(&idm, socket);
    free(fdi);
    if (overlay_fd != host_fd) {
        real_socket.close(host_fd);
    }
    ret = real_socket.close(overlay_fd);

    fd_to_epoll_fd[host_fd] = 0;
    fd_to_epoll_fd[overlay_fd] = 0;
    GLOBAL_UNLOCK;
    return ret;
}
