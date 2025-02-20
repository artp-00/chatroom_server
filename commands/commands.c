#include <sys/epoll.h>
#include <stdlib.h>

#include "stdio.h"
#include "utils/utils.h"
#include "commands.h"

ssize_t get_next_space(char *data, ssize_t data_length)
{
    ssize_t i = 0;
    while (data[i] && i < data_length && data[i] != ' ')
        i++;
    return i;
}

void clear_client_screen(struct connection_t *target)
{
    char *clear_string = "\e[H\e[2J\e[3J";
    if (send_data(target->client_socket, clear_string, strlen(clear_string)) != 0)
        fprintf(stderr, "[CHATROOM SERVER CLIENT ACTION] Failed to broadcast message to a client\n");
}

_Bool client_command(struct connection_t *connection, char *data, ssize_t data_length, struct connection_t *sender)
{
    ssize_t next_space = get_next_space(data, data_length - 1);

    char *command = xcalloc(next_space + 1, sizeof(char));
    memcpy(command, data, next_space);
    if (!connection)
        return 0;

    if (strcmp(command, "leave") == 0)
    {
        char *old_chatroom = sender->chatroom_id;
        char *response;
        if (old_chatroom == NULL)
        {
            int size = asprintf(&response, "You are not in any chatroom.\n");
            if (send_data(sender->client_socket, response, size) != 0)
                fprintf(stderr, "[CHATROOM SERVER CLIENT ACTION] Failed to broadcast message to a client\n");
        }
        else
        {
            sender->chatroom_id = NULL;
            int size = asprintf(&response, "Successfully left room.\n");
            if (send_data(sender->client_socket, response, size) != 0)
                fprintf(stderr, "[CHATROOM SERVER CLIENT ACTION] Failed to broadcast message to a client\n");

            free(response);

            size = asprintf(&response, "[Notification] User %s just left to your room.\n", sender->pseudonyme);
        
            struct connection_t *p;
            for (p = connection; p; p = p->next)
                if (p->chatroom_id && strcmp(p->chatroom_id, old_chatroom) == 0 && send_data(p->client_socket, response, size) != 0)
                    fprintf(stderr, "[CHATROOM SERVER CLIENT ACTION] Failed to broadcast message to a client\n");
        }
        free(response);
    }
    if (strcmp(command, "quit") == 0)
    {
        // TODO: implement
        //          need epoll instance in order to cleanly remove client
        //          => implement a context
    }
    if (strcmp(command, "room") == 0)
    {
        char *new_room = xcalloc(data_length - next_space, sizeof(char));
        memcpy(new_room, data + next_space + 1, data_length - next_space - 2);

        size_t ccount = room_count(connection, new_room);

        char *old_chatroom = sender->chatroom_id;
        sender->chatroom_id = new_room;
        char *response;
        int size;

        if (old_chatroom)
        {
            size = asprintf(&response, "[Notification] User %s just left to your room.\n", sender->pseudonyme);
        
            struct connection_t *p;
            for (p = connection; p; p = p->next)
                if (p->chatroom_id && strcmp(p->chatroom_id, old_chatroom) == 0 && send_data(p->client_socket, response, size) != 0)
                    fprintf(stderr, "[CHATROOM SERVER CLIENT ACTION] Failed to broadcast message to a client\n");

            free(response);
        }

        size = asprintf(&response, "Successfully connected to %s, there are currently %ld users connected.\n", new_room, ccount);
        if (send_data(sender->client_socket, response, size) != 0)
            fprintf(stderr, "[CHATROOM SERVER CLIENT ACTION] Failed to broadcast message to a client\n");

        free(response);

        size = asprintf(&response, "[Notification] User %s just connected to your room.\n", sender->pseudonyme);
    
        struct connection_t *p;
        for (p = connection; p; p = p->next)
            if (p->chatroom_id && strcmp(p->chatroom_id, sender->chatroom_id) == 0 && send_data(p->client_socket, response, size) != 0)
                fprintf(stderr, "[CHATROOM SERVER CLIENT ACTION] Failed to broadcast message to a client\n");

        free(response);
    }
    if (strcmp(command, "help") == 0)
    {
        // TODO: implement
    }
    if (strcmp(command, "clear") == 0)
    {
        clear_client_screen(sender);
    }
    // lists
    if (strcmp(command, "users") == 0)
    {
        // TODO: implement
    }
    if (strcmp(command, "rooms") == 0)
    {
        // TODO: implement
    }

    free(command);
    return 1;
}
