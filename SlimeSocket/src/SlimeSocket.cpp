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
#include <ifaddrs.h>
#include <errno.h>
#include <time.h>
#include <unordered_map>

#include "SlimeSocket.h"
#include "Message.h"
#include "types.h"
#include "NetworkUtils.h"

static const char *router_path = "/slime/SlimeRouter.sock";
static struct socket_calls socket_library;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static uint32_t prefix_ip = 0;
static uint32_t prefix_mask = 0;
static int32_t router_socket = 0;
static std::unordered_map<int, socket_info_t> socket_lookup;

__attribute__((constructor))
int main(void) { 
    LOG("main called\n");
    
    ERROR_CODE ec = S_OK;
    
    pthread_mutex_lock(&mutex);

    TRACE_IF_FAILED(load_socket_library(),
                    Cleanup,
                    "Failed to load socket library! 0x%x\n", ec);

    getenv_options();
    
    TRACE_IF_FAILED(connect_to_router(),
                    Cleanup,
                    "Failed to connect to router! 0x%x", ec);
    
Cleanup:
    pthread_mutex_unlock(&mutex);
    return 0;
}

__attribute__((destructor))
void exit(void) { 
    LOG("exit called\n");
    
    unload_socket_library();
}

static inline bool is_on_overlay(const struct sockaddr_in* addr) {
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
        LOG("WARNING: VNET_PREFIX is not set. Using 0.0.0.0/0.");
        LOG("All connections are treated as virtual network connections.");
    }
}

ERROR_CODE load_socket_library(void) {
    ERROR_CODE ec = S_OK;
    
    socket_library.socket = (int (*)(int, int, int)) dlsym(RTLD_NEXT, "socket");
    socket_library.bind = (int (*)(int, const sockaddr *, socklen_t)) dlsym(RTLD_NEXT, "bind");
    socket_library.listen = (int (*)(int, int)) dlsym(RTLD_NEXT, "listen");
    socket_library.accept = (int (*)(int, sockaddr *, socklen_t *)) dlsym(RTLD_NEXT, "accept");
    socket_library.accept4 = (int (*)(int, sockaddr *, socklen_t *, int)) dlsym(RTLD_NEXT, "accept4");
    socket_library.connect = (int (*)(int, const sockaddr *, socklen_t)) dlsym(RTLD_NEXT, "connect");
    socket_library.getpeername = (int (*)(int, sockaddr *, socklen_t *)) dlsym(RTLD_NEXT, "getpeername");
    socket_library.getsockname = (int (*)(int, sockaddr *, socklen_t *)) dlsym(RTLD_NEXT, "getsockname");
    socket_library.setsockopt = (int (*)(int, int, int, const void *, socklen_t)) dlsym(RTLD_NEXT, "setsockopt");
    socket_library.getsockopt = (int (*)(int, int, int, void *, socklen_t *)) dlsym(RTLD_NEXT, "getsockopt");
    socket_library.fcntl = (int (*)(int, int, ...)) dlsym(RTLD_NEXT, "fcntl");
    socket_library.close = (int (*)(int)) dlsym(RTLD_NEXT, "close");

    return ec;
}

void unload_socket_library(void) {
    // TODO: unload socket library
}

ERROR_CODE connect_to_router() {
    ERROR_CODE ec = S_OK;
    int address_length = 0;
    struct sockaddr_un server_address;
    
    router_socket = socket_library.socket(AF_UNIX, SOCK_STREAM, 0);
    TRACE_IF_FAILED(router_socket,
                    Cleanup,
                    "Failed to create unix socket! 0x%x\n", errno);
                    
    memset(&server_address, 0, sizeof(server_address));
    server_address.sun_family = AF_UNIX;
    strcpy(server_address.sun_path, router_path);
    address_length = sizeof(server_address.sun_family) + strlen(server_address.sun_path);
    
    TRACE_IF_FAILED(socket_library.connect(router_socket,
                                           (struct sockaddr*)&server_address,
                                           address_length),
                    Cleanup,
                    "Failed to connect to router! 0x%x\n", errno);
                    
Cleanup:
    return ec;
}

ERROR_CODE _socket(int domain, int type, int protocol, int* overlay_socket, int* host_socket) {
    ERROR_CODE ec = S_OK;
    SocketRequest socket_request;
    SocketResponse socket_response;
    MessageHeader message_header;
    
    *overlay_socket = socket_library.socket(domain, type, protocol);

    message_header.Id = REQUEST_TYPE_SOCKET;
    message_header.Size = sizeof(socket_request);
    socket_request.Domain = domain;
    socket_request.Type = type;
    socket_request.Protocol = protocol;
    
    TRACE_IF_FAILED(_send_message(message_header, &socket_request, sizeof(socket_request)),
                    Cleanup,
                    "Failed to send socket request! 0x%x\n", errno);
    
    TRACE_IF_FAILED(NetworkUtils::ReadFileDescriptorFromUnixSocket(router_socket, host_socket),
                    Cleanup,
                    "Failed to read file descriptor from socket! 0x%x\n", ec);
    
    TRACE_IF_FAILED(NetworkUtils::ReadAllFromSocket(router_socket,
                                                    &socket_response,
                                                    sizeof(socket_response)),
                    Cleanup,
                    "Failed to read from socket! 0x%x\n", errno);
    
    TRACE_IF_FAILED(socket_response.Status, Cleanup, "Router failed to create socket! 0x%x\n", ec);

Cleanup:
    return ec;
}

int socket(int domain, int type, int protocol) {
    LOG("socket called\n");
    int overlay_socket = 0;
    int host_socket = 0;
    
    if (FAILED(_socket(domain, type, protocol, &overlay_socket, &host_socket))) {
        socket_lookup.insert({overlay_socket, {overlay_socket, -1, true}});
        return socket_library.socket(domain, type, protocol);
    }

    socket_lookup.insert({overlay_socket, {overlay_socket, host_socket, false}});
    return overlay_socket;
}

ERROR_CODE _bind(int socket, struct sockaddr_in *addr, socklen_t addrlen) {
    ERROR_CODE ec = S_OK;
    BindRequest bind_request;
    BindResponse bind_response;
    MessageHeader message_header;
    const socket_info_t& socket_info = socket_lookup.at(socket);
    
    TRACE_IF_FAILED(socket_library.bind(socket_info.overlay_socket, (struct sockaddr*)addr, addrlen),
                    Cleanup,
                    "Failed to bind to socket! 0x%x\n", errno);
    
    TRACE_IF_FAILED(getsockname(socket_info.overlay_socket,
                                (struct sockaddr*)addr,
                                &addrlen),
                    Cleanup,
                    "Failed to get socket information! 0x%x\n", errno);

    if (addr->sin_addr.s_addr == INADDR_ANY) {
        TRACE_IF_FAILED(NetworkUtils::GetIpAddress(&addr->sin_addr.s_addr),
                        Cleanup,
                        "Failed to get IP address! 0x%x", ec);
    }
    
    message_header.Id = REQUEST_TYPE_BIND;
    message_header.Size = sizeof(bind_request);
    bind_request.VirtualIpAddress = addr->sin_addr.s_addr;
    bind_request.VirtualPort = addr->sin_port;
    TRACE_IF_FAILED(_send_message(message_header, &bind_request, sizeof(bind_request)),
                    Cleanup,
                    "Failed to send bind reqeuest! 0x%x\n", errno);
    
    TRACE_IF_FAILED(NetworkUtils::WriteFileDescriptorToUnixSocket(router_socket,
                                                                  socket_info.host_socket),
                    Cleanup,
                    "Failed to send file descriptor! 0x%x\n", ec);
    
    TRACE_IF_FAILED(NetworkUtils::ReadAllFromSocket(router_socket,
                                                    &bind_response,
                                                    sizeof(bind_response)),
                    Cleanup,
                    "Failed to read from socket! 0x%x\n", errno);
    
    TRACE_IF_FAILED(bind_response.Status, Cleanup, "Router failed to bind socket! 0x%x\n", ec);

Cleanup:
    return ec;
}

int bind(int socket, const struct sockaddr *addr, socklen_t addrlen) {
    LOG("bind called\n");
    int ec = S_OK;
    
    // TODO: send request for each interface if INADDR_ANY
    ec = _bind(socket, (struct sockaddr_in*)addr, addrlen);
    if (FAILED(ec)) {
        ec = socket_library.bind(socket, addr, addrlen);
    }

    return ec;
}

int listen(int socket, int backlog) {
    LOG("listen called\n");
    const socket_info_t& socket_info = socket_lookup.at(socket);

    if (socket_info.is_normal) {
        return socket_library.listen(socket, backlog);
    }

    return socket_library.listen(socket_info.host_socket, backlog);
}

int accept(int socket, struct sockaddr *addr, socklen_t *addrlen) {
    return accept4(socket, addr, addrlen, 0);
}

ERROR_CODE _send_message(const MessageHeader& message_header, void* body, size_t body_size) {
    ERROR_CODE ec = S_OK;

    TRACE_IF_FAILED(NetworkUtils::WriteAllToSocket(router_socket,
                                                   &message_header,
                                                   sizeof(message_header)),
                    Cleanup,
                    "Failed to write request header! 0x%x\n", errno);
                                   
    TRACE_IF_FAILED(NetworkUtils::WriteAllToSocket(router_socket,
                                                   body,
                                                   body_size),
                    Cleanup,
                    "Failed to write request body! 0x%x\n", errno);
Cleanup:
    return ec;
}

ERROR_CODE
_accept4(int socket, struct sockaddr *addr, socklen_t *addrlen, int flags, int* client_socket) {
    UNREFERENCED_PARAMETER(addr);
    UNREFERENCED_PARAMETER(addrlen);
    
    ERROR_CODE ec = S_OK;
    MessageHeader message_header;
    AcceptRequest accept_request;
    AcceptResponse accept_response;
    const socket_info_t& socket_info = socket_lookup.at(socket);

    message_header.Id = REQUEST_TYPE_ACCEPT;
    message_header.Size = sizeof(accept_request);
    accept_request.Flags = flags;
    TRACE_IF_FAILED(_send_message(message_header, &accept_request, sizeof(accept_request)),
                    Cleanup,
                    "Failed to send accept request! 0x%x\n", errno);
    
    TRACE_IF_FAILED(NetworkUtils::WriteFileDescriptorToUnixSocket(router_socket,
                                                                 socket_info.host_socket),
                    Cleanup,
                    "Failed to send file descriptor! 0x%x\n", ec);

    TRACE_IF_FAILED(NetworkUtils::ReadFileDescriptorFromUnixSocket(router_socket,
                                                                   client_socket),
                    Cleanup,
                    "Failed to receive file descriptor! 0x%x\n", ec);
    
    TRACE_IF_FAILED(NetworkUtils::ReadAllFromSocket(router_socket,
                                                    &accept_response,
                                                    sizeof(accept_response)),
                    Cleanup,
                    "Failed to read from socket! 0x%x\n", errno);
    
    TRACE_IF_FAILED(accept_response.Status, Cleanup, "Router failed to accept socket! 0x%x\n", ec);
    
Cleanup:
    return ec;
}

int accept4(int socket, struct sockaddr *addr, socklen_t *addrlen, int flags) {
    LOG("accept called\n");

    int client_socket = 0;
    
    if (FAILED(_accept4(socket, addr, addrlen, flags, &client_socket))) {
        // TODO: update socket_lookup
        return socket_library.accept4(socket, addr, addrlen, flags);
    }
    
    // TODO: update socket_lookup    
    return client_socket;
}

int _connect(int socket, const struct sockaddr_in *addr, socklen_t addrlen) {
    UNREFERENCED_PARAMETER(addrlen);
    
    ERROR_CODE ec = S_OK;
    ConnectRequest connect_request;
    ConnectResponse connect_response;
    MessageHeader message_header;
    socket_info_t& socket_info = socket_lookup.at(socket);
    
    message_header.Id = REQUEST_TYPE_CONNECT;
    message_header.Size = sizeof(connect_request);
    connect_request.VirtualIpAddress = addr->sin_addr.s_addr;
    connect_request.VirtualPort = addr->sin_port;
    
    TRACE_IF_FAILED(_send_message(message_header, &connect_request, sizeof(connect_request)),
                    Cleanup,
                    "Failed to send connect request! 0x%x\n", errno);
    
    TRACE_IF_FAILED(NetworkUtils::WriteFileDescriptorToUnixSocket(router_socket, socket_info.host_socket),
                    Cleanup,
                    "Failed to send file descriptor! 0x%x\n", ec);
    
    TRACE_IF_FAILED(NetworkUtils::ReadAllFromSocket(router_socket,
                                                    &connect_response,
                                                    sizeof(connect_response)),
                    Cleanup,
                    "Failed to read from socket! 0x%x\n", errno);
    
    TRACE_IF_FAILED(connect_response.Status, Cleanup, "Router failed to connect socket! 0x%x\n", ec);

    if (socket != socket_info.host_socket) {
        socket_info.overlay_socket = dup(socket_info.overlay_socket);
        dup2(socket_info.host_socket, socket);
        socket_library.close(socket_info.host_socket);
        socket_info.host_socket = socket;
    }
Cleanup:
    return ec;
}

int connect(int socket, const struct sockaddr *addr, socklen_t addrlen) {
    LOG("connect called\n");
    struct timespec start;
	struct timespec end;
    double elapsed_time = 0;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
#ifdef MEASURE
    socket_library.connect(socket, addr, addrlen);
#else
    if (FAILED(_connect(socket, (const struct sockaddr_in*)addr, addrlen))) {
        return socket_library.connect(socket, addr, addrlen);
    }
#endif
    clock_gettime(CLOCK_MONOTONIC, &end);    
    elapsed_time = (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec -	start.tv_nsec)/1.0e9;
    LOG("Connection setup time: %lf seconds\n", elapsed_time);
    return 0;
}

/* int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event) { } */
/* int getpeername(int socket, struct sockaddr *addr, socklen_t *addrlen) { } */
/* int getsockname(int socket, struct sockaddr *addr, socklen_t *addrlen) { } */
/* int getsockopt(int socket, int level, int optname, void *optval, socklen_t *optlen) { } */
/* int setsockopt(int socket, int level, int optname, const void *optval, socklen_t optlen) { } */
/* int fcntl(int socket, int cmd, ... /1* arg *1/) { } */

int close(int socket) {
    LOG("close called\n");
    socket_info_t socket_info = socket_lookup.at(socket);
    if (socket_info.is_normal) {
        return socket_library.close(socket);
    }

    return socket_library.close(socket_info.host_socket);
}
