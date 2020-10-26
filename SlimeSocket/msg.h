#ifndef _MSG_H_
#define _MSG_H_

#include <stdint.h>

typedef enum {
    MSG_SOCKET = 0,
    MSG_BIND = 1,
    MSG_ACCEPT = 2,
    MSG_CONNECT = 3,
    MSG_MAX
} msg_kind_t;

typedef struct {
    msg_kind_t kind;
    uint32_t size;
    uint8_t body[0];
} msg_t;

typedef struct {
    int32_t domain;
    int32_t type;
    int32_t protocol;
} socket_req_t;

typedef struct {
    int32_t host_socket;
} socket_resp_t;

typedef struct {
    uint32_t ip;
    uint16_t port;
} bind_req_t;

typedef struct {
    int32_t status;
} bind_resp_t;

typedef struct {
    int32_t flags;
} accept_req_t;

typedef struct {
    int32_t client_sock;
} accept_resp_t;

typedef struct {
    uint32_t virt_ip;
    uint16_t virt_port;
} connect_req_t;

typedef struct {
    int32_t status;
} connect_resp_t;

msg_t *new_socket_req(int domain, int type, int protocol);
msg_t *new_bind_req(uint32_t ip, uint16_t port);
msg_t *new_accept_req(int32_t flags);
msg_t *new_connect_req(uint32_t virt_ip, uint16_t virt_port);

#endif
