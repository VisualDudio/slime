#include "msg.h"
#include <malloc.h>
#include <string.h>

int resp_size(msg_kind_t kind) {
    switch (kind) {
    case MSG_SOCKET: return sizeof(socket_resp_t);
    case MSG_BIND: return sizeof(bind_resp_t);
    case MSG_ACCEPT: return sizeof(accept_resp_t);
    case MSG_CONNECT: return sizeof(connect_resp_t);
    default: return 0;
    }
}

msg_t *new_socket_req(int domain, int type, int protocol) {
    socket_req_t socket_req = { .domain = domain, .type = type, .protocol = protocol };
    msg_t *req = (msg_t *) malloc(sizeof(msg_kind_t) + sizeof(uint32_t) + sizeof(socket_req_t));
    if (!req) return NULL;
    req->kind = MSG_SOCKET;
    req->size = sizeof(socket_req_t);
    memcpy(req->body, &socket_req, sizeof(socket_req_t));
    return req;
}

msg_t *new_bind_req(uint32_t ip, uint16_t port) {
    bind_req_t bind_req = { .ip = ip, .port = port };
    msg_t *req = (msg_t *) malloc(sizeof(msg_kind_t) + sizeof(uint32_t) + sizeof(bind_req_t));
    if (!req) return NULL;
    req->kind = MSG_BIND;
    req->size = sizeof(bind_req_t);
    memcpy(req->body, &bind_req, sizeof(bind_req_t));
    return req;
}

msg_t *new_accept_req(int32_t flags) {
    accept_req_t accept_req = { .flags = flags };
    msg_t *req = (msg_t *) malloc(sizeof(msg_kind_t) + sizeof(uint32_t) + sizeof(accept_req_t));
    if (!req) return NULL;
    req->kind = MSG_ACCEPT;
    req->size = sizeof(accept_req_t);
    memcpy(req->body, &accept_req, sizeof(accept_req_t));
    return req;
}

msg_t *new_connect_req(uint32_t virt_ip, uint16_t virt_port) {
    connect_req_t connect_req = { .virt_ip = virt_ip, .virt_port = virt_port };
    msg_t *req = (msg_t *) malloc(sizeof(msg_kind_t) + sizeof(uint32_t) + sizeof(connect_req_t));
    if (!req) return NULL;
    req->kind = MSG_CONNECT;
    req->size = sizeof(connect_req_t);
    memcpy(req->body, &connect_req, sizeof(connect_req_t));
    return req;
}
