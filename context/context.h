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

#endif // CONTEXT_H
