#include <sys/epoll.h>
#include <stdlib.h>

#include "utils/utils.h"
#include "commands.h"

ssize_t get_next_space(char *data, ssize_t data_length)
{
    ssize_t i = 0;
    while (data[i] && i < data_length && data[i] != ' ')
        i++;
    // if (i == data_length || !data[i])
    //     return -1;
    return i;
}

_Bool client_command(struct connection_t *connection, char *data, ssize_t data_length, struct connection_t *sender)
{
    ssize_t next_space = get_next_space(data, data_length - 1);

    char *command = xcalloc(next_space + 1, sizeof(char));
    memcpy(command, data, next_space);
    if (!connection)
        return 0;

    // printf("[DEBUG] Got command: %s$\n", command);
    if (strcmp(command, "leave") == 0)
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

        sender->chatroom_id = new_room;

        char *response;
        int size = asprintf(&response, "Successfully connected to %s, there are currently %ld users connected.\n", new_room, ccount);
        if (send_data(sender->client_socket, response, size) != 0)
            fprintf(stderr, "[CHATROOM SERVER CLIENT ACTION] Failed to broadcast message to a client\n");
    }
    if (strcmp(command, "help") == 0)
    {
        // TODO: implement
    }
    if (strcmp(command, "clear") == 0)
    {
        char *clear_string = "\e[H\e[2J\e[3J";
        if (send_data(sender->client_socket, clear_string, strlen(clear_string)) != 0)
            fprintf(stderr, "[CHATROOM SERVER CLIENT ACTION] Failed to broadcast message to a client\n");
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
