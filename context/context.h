#ifndef CONTEXT_H
#define CONTEXT_H
#include "connection.h"

struct context
{
    struct chatroom *chatrooms;
    struct connection_t *clients;
    int ep_instance;
    int server_socket;
};

struct context *create_context(int ep_instance, int srv_sckt);
void rm_ep_clients(int ep_instance, struct connection_t *head);
void free_context(struct context *ctx);

#endif // CONTEXT_H
