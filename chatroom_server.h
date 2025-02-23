#ifndef EPOLL_SERVER_H
#define EPOLL_SERVER_H

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "context/connection.h"

#define MAX_EVENTS 64

#define DEFAULT_BUFFER_SIZE 2048
int create_and_bind(struct addrinfo *addrinfo);

int prepare_socket(const char *ip, const char *port);

struct connection_t *accept_client(int epoll_instance, int server_socket,
                                   struct connection_t *connection);

#endif /* !EPOLL_SERVER_H */
