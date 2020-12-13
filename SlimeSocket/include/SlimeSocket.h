/*++

Module Name:

    SlimeSocket.h

Abstract:

    Header file for the SlimeSocket library.

--*/

#pragma once

//
// ---------------------------------------------------------------------- Includes
//

#include "Error.h"
#include "Message.h"

//
// ---------------------------------------------------------------------- Definitions
//



//
// ---------------------------------------------------------------------- Functions
//

ERROR_CODE load_socket_library(void);

void unload_socket_library(void);

void getenv_options(void);

ERROR_CODE connect_to_router();

ERROR_CODE _send_message(const MessageHeader& message_header, char* body, size_t body_size);

ERROR_CODE _socket(int domain, int type, int protocol, int* overlay_socket, int* host_socket);

int socket(int domain, int type, int protocol);

ERROR_CODE _bind(int socket, struct sockaddr_in *addr, socklen_t addrlen);

int bind(int socket, const struct sockaddr *addr, socklen_t addrlen);

int listen(int socket, int backlog);

int accept(int socket, struct sockaddr *addr, socklen_t *addrlen);

ERROR_CODE _send_message(const MessageHeader& message_header, void* body, size_t body_size);

ERROR_CODE _accept4(int socket, struct sockaddr *addr, socklen_t *addrlen, int flags, int* client_socket);

int accept4(int socket, struct sockaddr *addr, socklen_t *addrlen, int flags);

int _connect(int socket, const struct sockaddr_in *addr, socklen_t addrlen);

int connect(int socket, const struct sockaddr *addr, socklen_t addrlen);

int close(int socket);

ssize_t send(int fd, const void *buf, size_t len, int flags);
ssize_t recv(int fd, void *buf, size_t len, int flags);
ssize_t recvfrom(int socket, void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);
ssize_t sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);
ssize_t write(int fd, const void *buf, size_t len);
ssize_t read(int fd, void *buf, size_t len);
