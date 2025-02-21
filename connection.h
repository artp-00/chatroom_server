#ifndef CONNECTION_H
#define CONNECTION_H

#include <sys/types.h>

enum connection_stage
{
    CONNECTED_UNNAMED,
    CONNECTED_IDENTIFIED,
};

// Contains the information about the clients (linked list)
struct connection_t
{
    int client_socket;

    char *buffer; // buffer containing the data received by this client

    char *pseudonyme;
    char *chatroom_id;
    enum connection_stage stage;

    ssize_t nb_read; // number of bytes read (also size of the buffer)

    struct connection_t *next; // the next client
};

// linked list
struct chatroom
{
    char *chatroom_id;
    struct chatroom *next;
};

struct connection_t *add_client(struct connection_t *head, int client_socket);

struct connection_t *remove_client(struct connection_t *head, int client_socket);

// find client based on socket
struct connection_t *find_client(struct connection_t *head, int client_socket);

// count number of clients in a room
size_t room_count(struct connection_t *head, char *room_id);

#endif /* !CONNECTION_H */
