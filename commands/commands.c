#include <string.h>
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

char *concat(char *first, char *second)
{
    if (second == NULL)
        return first;
    if (first == NULL)
    {
        size_t len2 = strlen(second);
        char *res = xcalloc(len2 + 1, sizeof(char));
        void *err = memcpy(res, second, len2);
        if (!err)
            return NULL;
        return res;
    }
    size_t len1 = strlen(first);
    size_t len2 = strlen(second);
    char *res = xrealloc(first, sizeof(char) * (len1 + len2 + 1));
    void *err = memcpy(res + len1, second, len2);
    if (!err)
        return NULL;
    res[len1 + len2] = 0;
    return res;
}

void clear_client_screen(struct connection_t *target)
{
    char *clear_string = "\e[H\e[2JP\e[3J";
    if (send_data(target->client_socket, clear_string, strlen(clear_string)) != 0)
        fprintf(stderr, "[CHATROOM SERVER CLIENT ACTION] Failed to broadcast message to a client\n");
}

_Bool client_command(struct connection_t *connection, char *data, ssize_t data_length, struct connection_t *sender)
{
    // very long function
    // should probably be separated into individual functions for commands but need context first
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
    else if (strcmp(command, "quit") == 0)
    {
        // TODO: implement
        //          need epoll instance in order to cleanly remove client
        //          => implement a context
    }
    else if (strcmp(command, "room") == 0)
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

        // new_room leaks currently but it shall be added to a rooms linked list in order to keep track of all the rooms
        // it cant be freed here
        free(response);
    }
    else if (strcmp(command, "help") == 0)
    {
        char *help_string =  "To chat with someone you need to be in a room with them,\nyou can use commands to find out where they are and connect to their room.\nCommand list:\n\t/help: Prints this message. \n\t/room [room_id]: Connects you to a room. \n\t/leave: Exits the room you are currently in. \n\t/clear: Clears the screen. \n\t/users: Lists the current users and where they are.\n";
        if (send_data(sender->client_socket, help_string, strlen(help_string)) != 0)
            fprintf(stderr, "[CHATROOM SERVER USERS COMMAND] Failed to broadcast message to a client\n");

    }
    else if (strcmp(command, "clear") == 0)
    {
        clear_client_screen(sender);
    }
    else if (strcmp(command, "users") == 0)
    {
        char *init_string = "Connected user:\n";
        char *res = concat(NULL, init_string);
        char *tmp;
        int nbytes;
        char *buffer;
        struct connection_t *p;
        for (p = connection; p; p = p->next)
        {
            if (p->pseudonyme && p == sender)
            {
                if (p->chatroom_id)
                    nbytes = asprintf(&buffer, "\tUser %s(you) in room %s.\n", p->pseudonyme, p->chatroom_id);
                else
                    nbytes = asprintf(&buffer, "\tUser %s(you) not in a room.\n", p->pseudonyme);
                if (nbytes != -1)
                {
                    tmp = concat(res, buffer);
                    if (tmp)
                        res = tmp;
                    free(buffer);
                }
            }
            else if (p->pseudonyme)
            {
                if (p->chatroom_id)
                    nbytes = asprintf(&buffer, "\tUser %s in room %s.\n", p->pseudonyme, p->chatroom_id);
                else
                    nbytes = asprintf(&buffer, "\tUser %s not in a room.\n", p->pseudonyme);
                if (nbytes != -1)
                {
                    tmp = concat(res, buffer);
                    if (tmp)
                        res = tmp;
                    free(buffer);
                }
            }
        }
        if (send_data(sender->client_socket, res, strlen(res)) != 0)
            fprintf(stderr, "[CHATROOM SERVER USERS COMMAND] Failed to broadcast message to a client\n");
        free(res);
    }
    else if (strcmp(command, "rooms") == 0)
    {
        // TODO: implement
        // requires context

        // char *res = NULL;
        // struct connection_t *p;
        // for (p = connection; p; p = p->next)
        // {
        //     if (p->chatroom_id && )
        // }
    }
    else
    {
        // unknown command
        char *unknown_message;
        int nbytes = asprintf(&unknown_message, "Unknown command %s.\n", command);
        if (nbytes != -1 && send_data(sender->client_socket, unknown_message, nbytes) != 0)
        {
            fprintf(stderr, "[CHATROOM SERVER USERS COMMAND] Failed to broadcast message to a client\n");
            free(unknown_message);
        }
    }

    free(command);
    return 1;
}
