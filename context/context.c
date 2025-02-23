#include <context/context.h>
#include "connection.h"
#include <stdlib.h>
#include "utils/utils.h"
#include <sys/epoll.h>
#include <err.h>
#include <stdio.h>
#include <unistd.h>

// TODO: wrap add and remove clients here
//       implement in main

struct context *create_context(int ep_instance, int srv_sckt)
{
    struct context *res = xmalloc(sizeof(struct context));

    res->ep_instance = ep_instance;
    res->chatrooms = NULL;
    res->clients = NULL;
    res->server_socket = srv_sckt;
    return res;
}

void rm_ep_clients(int ep_instance, struct connection_t *head)
{
    if (!head)
        return;

    if (epoll_ctl(ep_instance, EPOLL_CTL_DEL, head->client_socket, NULL) == -1)
        errx(1, "[CHATROOM SERVER HANDLE CLIENT EVENT] FAILED TO REMOVE SOCKET FROM CHATROOM INSTANCE\n");

    rm_ep_clients(ep_instance, head->next);
}

void free_context(struct context *ctx)
{
    rm_ep_clients(ctx->ep_instance, ctx->clients);
    free_connections(ctx->clients);
    free_chatrooms(ctx->chatrooms);
    // just dont close epoll_instance
    close(ctx->server_socket);
    
}
