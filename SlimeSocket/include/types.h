#pragma once

#include <pthread.h>

typedef enum SOCKET_FUNCTION_CALL
{
    SOCKET_SOCKET,
    SOCKET_BIND,
    SOCKET_ACCEPT,
    SOCKET_ACCEPT4,
    SOCKET_CONNECT,
    SOCKET_GETSOCKNAME
} SOCKET_FUNCTION_CALL;

struct epoll_calls
{
    int (*epoll_ctl) (int epfd, int op, int fd, struct epoll_event *event);
};

struct socket_calls
{
    int (*socket)(int domain, int type, int protocol);
    int (*bind)(int socket, const struct sockaddr *addr, socklen_t addrlen);
    int (*listen)(int socket, int backlog);
    int (*accept)(int socket, struct sockaddr *addr, socklen_t *addrlen);
    int (*accept4)(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags);
    int (*connect)(int socket, const struct sockaddr *addr, socklen_t addrlen);
    int (*getpeername)(int socket, struct sockaddr *addr, socklen_t *addrlen);
    int (*getsockname)(int socket, struct sockaddr *addr, socklen_t *addrlen);
    int (*setsockopt)(int socket, int level, int optname,
              const void *optval, socklen_t optlen);
    int (*getsockopt)(int socket, int level, int optname,
              void *optval, socklen_t *optlen);
    int (*fcntl)(int socket, int cmd, ... /* arg */);
    int (*close)(int socket);
};

typedef struct
{
    int overlay_socket;
    int host_socket;
    bool is_normal;
} socket_info_t;
