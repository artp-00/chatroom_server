#include "connection.h"

#include <err.h>
#include <stdlib.h>
#include <unistd.h>

#include "utils/xalloc.h"

struct connection_t *add_client(struct connection_t *head, int client_socket)
{
    struct connection_t *new_connection = xmalloc(sizeof(struct connection_t));

    new_connection->client_socket = client_socket;
    new_connection->buffer = NULL;
    new_connection->nb_read = 0;
    new_connection->next = head;

    return new_connection;
}

struct connection_t *remove_client(struct connection_t *head, int client_socket)
{
    if (head && head->client_socket == client_socket)
    {
        struct connection_t *client_connection = head->next;
        if (close(head->client_socket) == -1)
            err(EXIT_FAILURE, "Failed to close socket");
        free(head->buffer);
        free(head);
        return client_connection;
    }

    struct connection_t *tmp = head;
    while (tmp->next)
    {
        if (tmp->next->client_socket == client_socket)
        {
            struct connection_t *client_connection = tmp->next;
            tmp->next = client_connection->next;
            if (close(client_connection->client_socket) == -1)
                err(EXIT_FAILURE, "Failed to close socket");
            free(client_connection->buffer);
            free(client_connection);
            break;
        }
        tmp = tmp->next;
    }

    return head;
}

struct connection_t *find_client(struct connection_t *head, int client_socket)
{
    while (head != NULL && head->client_socket != client_socket)
        head = head->next;

    return head;
}
