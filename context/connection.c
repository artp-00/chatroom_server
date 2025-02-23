#include "connection.h"

#include <err.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "stdio.h"
#include "utils/utils.h"

struct connection_t *add_client(struct connection_t *head, int client_socket)
{
    struct connection_t *new_connection = xmalloc(sizeof(struct connection_t));

    new_connection->client_socket = client_socket;
    new_connection->buffer = NULL;
    new_connection->nb_read = 0;
    new_connection->next = head;
    new_connection->stage = CONNECTED_UNNAMED;
    new_connection->chatroom_id = NULL;
    new_connection->pseudonyme = NULL;

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
        if (head->pseudonyme)
            free(head->pseudonyme);
        if (head->chatroom_id)
            free(head->chatroom_id);
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
            // shoudlnt matter at all
            if (client_connection->pseudonyme)
                free(client_connection->pseudonyme);
            if (client_connection->chatroom_id)
                free(client_connection->chatroom_id);

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

size_t room_count(struct connection_t *head, char *room_id)
{
    size_t cpt = 0;
    while (head != NULL)
    {
        if (head->chatroom_id != NULL && strcmp(head->chatroom_id, room_id) == 0)
            cpt++;
        head = head->next;
    }

    return cpt;
}

void free_connections(struct connection_t *head)
{
    if (head)
    {
        free_connections(head->next);
        if (close(head->client_socket) == -1)
            err(EXIT_FAILURE, "Failed to close socket");

        free(head->buffer);
        // shoudlnt matter at all
        if (head->pseudonyme)
            free(head->pseudonyme);
        if (head->chatroom_id)
            free(head->chatroom_id);
        free(head);
    }
}

