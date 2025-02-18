#ifndef CONNECTION_H
#define CONNECTION_H

#include <sys/types.h>

/**
 * \brief Contains the information about the clients (linked list)
 */
struct connection_t
{
    int client_socket; /**< socket fd of the client */

    char *buffer; /**< buffer containing the data received by this client */

    ssize_t nb_read; /**< number of bytes read (also size of the buffer) */

    struct connection_t *next; /**< the next client */
};

/**
 * \brief Adds a new client connection_t to the linked list
 *
 * \param connection: the connection_t linked list with all the clients
 *
 * \param client_socket: the client socket fd to add
 *
 * \return The connection_t linked list with the element added
 */
struct connection_t *add_client(struct connection_t *head, int client_socket);

/**
 * \brief Removes the client connection_t from the linked list connection
 *
 * \param connection: the connection_t linked list with all the clients
 *
 * \param client_socket: the client socket fd to remove
 *
 * \return The connection_t linked list with element removed
 */
struct connection_t *remove_client(struct connection_t *head,
                                   int client_socket);

/**
 * \brief Find the connection_t element where the socket is equal to client sock
 *
 * \param connection: the connection_t linked list with all the clients
 *
 * \param client_socket: the client socket to find
 *
 * \return The connection_t element of the specific client
 */
struct connection_t *find_client(struct connection_t *head, int client_socket);

#endif /* !CONNECTION_H */
