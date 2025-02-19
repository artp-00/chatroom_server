#include <err.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <stdlib.h>

#include "chatroom_server.h"
#include "connection.h"
#include "utils/xalloc.h"

struct connection_t *remove_wrapper(struct connection_t *connection, int client_socket, int epoll_instance);
int send_data(int client_socket, char *data, ssize_t data_length);

int create_and_bind(struct addrinfo *addrinfo)
{
    struct addrinfo *p;
    int optval = 1;
    int s;
    for (p = addrinfo; p; p = p->ai_next)
    {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s == -1)
            continue;
        // configure the socket
        if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1)
            continue;

        if (bind(s, p->ai_addr, p->ai_addrlen) == 0)
            break;
        else
            close(s);
    }
    freeaddrinfo(addrinfo);
    if (!p)
    {
        fprintf(stderr, "[CHATROOM SERVER CREATE AND BIND] Failed to connect\n");
        return -1;
    }
    return s;
}

int prepare_socket(const char *ip, const char *port)
{
    struct addrinfo *hints = xmalloc(sizeof(struct addrinfo));
    // address criteria on which address gets chosen
    hints->ai_family = AF_INET; // IPv4
    hints->ai_socktype = 0;
    hints->ai_protocol = IPPROTO_TCP; // TCP
    hints->ai_flags = 0; // no flag
    // more info on the getaddrinfo man page

    struct addrinfo *res; // result struct

    if (getaddrinfo(ip, port, hints, &res) != 0)
    {
        fprintf(stderr, "[CHATROOM SERVER PREPARE SOCKET] FAILED TO GET ADDRESS INFO | ip: %s, port: %s\n", ip, port);
        free(hints);
        return -1;
    }

    free(hints);
    int s = create_and_bind(res); // res is freed

    if (listen(s, SOMAXCONN) == -1) // second param = maximum connections
         errx(1, "[CHATROOM SERVER PREPARE SOCKET] FAILED TO LISTEN TO SOCKET\n");

    return s;
}

struct connection_t *accept_client(int epoll_instance, int server_socket,
                                   struct connection_t *connection)
{
    // on accepte la premiere connection vers le serveur et creer un socket permettant de communiquer
    // pas sur des params null
    int new_client_socket = accept(server_socket, NULL, NULL);
    if (new_client_socket == -1)
    {
        fprintf(stderr, "[CHATROOM SERVER ACCEPT CLIENT] FAILED TO FETCH NEW CLIENT SOCKET\n");
        return NULL;
    }
    struct connection_t *nc = add_client(connection, new_client_socket);
    // client event setup
    struct epoll_event client_event;
    client_event.events = EPOLLIN;
    client_event.data.fd = new_client_socket;

    // add client to epoll instance
    if (epoll_ctl(epoll_instance, EPOLL_CTL_ADD, new_client_socket, &client_event))
    {
        fprintf(stderr, "[CHATROOM SERVER ACCEPT CLIENT] FAILED TO ADD CLIENT TO CHATROOM INSTANCE\n");
        close(new_client_socket);
        return NULL;
    }
    printf("[CHATROOM SERVER ACCEPT] Received connection request on socket: %d\n", new_client_socket);
    char *pseudonyme_question = "Enter your pseudonyme: ";
    if (send_data(new_client_socket, pseudonyme_question, strlen(pseudonyme_question)) != 0)
        fprintf(stderr, "[CHATROOM SERVER CLIENT ACTION] FAILED TO BROADCAST MESSAGE TO A CLIENT\n");

    return nc;
}

int concatenate_nlc(char **data, char *buffer, ssize_t buff_len, size_t start_index) // nlc = newline check
{
    // cet fonction concatene deux chaines de characteres en place et renvoie true si un charactere newline est detecter
    // pourrait changer pour differentes utilisation du serveur
    int res = 0;
    //printf("[DEBUG]: concatenate_nld | start_index: %ld | reallocating %ld bytes\n", start_index, start_index + buff_len);
    *data = xrealloc(*data, start_index + buff_len); // taille du strings actuel + des chars a ajouter
    for (ssize_t i = 0; i < buff_len; i++)
    {
        if (buffer[i] == '\n')
            res = 1;
        (*data)[start_index + i] = buffer[i];
    }
    return res;
}

// struct connection_t *client_action(int epoll_instance, int client_socket, struct connection_t *connection, char *data, ssize_t data_length)
int send_data(int client_socket, char *data, ssize_t data_length)
{
    ssize_t cpt = 0;
    ssize_t tmp;
    while (cpt < data_length)
    {
        // on envoie le msg au client et on update cpt
        tmp = send(client_socket, data + cpt, data_length, 0); // 0 = NOFLAG
        if (tmp == -1)
            return -1;
        cpt += tmp;
    }
    return 0;
}

char *get_message_value(char *data, ssize_t data_length, struct connection_t *sender)
{
    // "pseudonyme: "
    size_t new_size = strlen(sender->pseudonyme) + 2;
    // null byte
    new_size += data_length + 1;

    char *res = xcalloc(new_size, sizeof(char));
    res = memcpy(res, sender->pseudonyme, strlen(sender->pseudonyme));
    if (!res)
        errx(2, "[CHATROOM SERVER CLIENT ACTION] Failed to build message value\n");
    void *err = memcpy(res + strlen(sender->pseudonyme), ": ", 2);
    if (!err)
        errx(2, "[CHATROOM SERVER CLIENT ACTION] Failed to build message value\n");
    err = memcpy(res + strlen(sender->pseudonyme) + 2, data, data_length);
    if (!err)
        errx(2, "[CHATROOM SERVER CLIENT ACTION] Failed to build message value\n");
    return res;
}

// struct connection_t *client_action(struct connection_t *connection, int client_socket, int server_socket, char *data, ssize_t data_length)
struct connection_t *client_action(struct connection_t *connection, char *data, ssize_t data_length, struct connection_t *sender)
{
    // fonctions changeant en fonction de l'utilisation du serveur
    
    char *message_value = get_message_value(data, data_length, sender);
    size_t true_size = strlen(message_value);

    struct connection_t *p;
    for (p = connection; p; p = p->next)
        if (p != sender && send_data(p->client_socket, message_value, true_size) != 0)
        // if (send_data(p->client_socket, message_value, true_size) != 0)
            fprintf(stderr, "[CHATROOM SERVER CLIENT ACTION] Failed to broadcast message to a client\n");
    return connection;
}

struct connection_t *client_get_name(struct connection_t *connection, char *data, ssize_t data_length)
{
    // fonctions changeant en fonction de l'utilisation du serveur

    // null bad already initialized
    char *pseudonyme = xcalloc(data_length, sizeof(char));
    data[data_length - 1] = 0;
    pseudonyme = memcpy(pseudonyme, data, data_length);

    // printf("[DEBUG] Got pseudonyme: %s of length: %ld\n", pseudonyme, data_length);
    // update pseudonyme
    connection->pseudonyme = pseudonyme;
    printf("[CHATROOM SERVER CLIENT] Got pseudonyme %s from client in socket %d\n", pseudonyme, connection->client_socket);

    // update client state
    connection->stage = CONNECTED_IDENTIFIED;
    return connection;
}

struct connection_t *remove_wrapper(struct connection_t *connection, int client_socket, int epoll_instance)
{
    if (epoll_ctl(epoll_instance, EPOLL_CTL_DEL, client_socket, NULL) == -1)
        errx(1, "[CHATROOM SERVER HANDLE CLIENT EVENT] FAILED TO REMOVE SOCKET FROM CHATROOM INSTANCE\n");

    connection = remove_client(connection, client_socket);
    return connection;
}

struct connection_t *handle_client_event(int epoll_instance, int client_socket, struct connection_t *connection)
{
    // fonction mystique a commenter
    char buffer[DEFAULT_BUFFER_SIZE];
    // recv ne recoit pas forcement tt d'un coup
    ssize_t errc = recv(client_socket, buffer, DEFAULT_BUFFER_SIZE, 0); // 0 = flags / aucun flag

    //printf("[DEBUG]: recv of size %ld\n", errc);
    if (errc == -1)
    {
        fprintf(stderr, "[CHATROOM SERVER HANDLE CLIENT EVENT] FAILED TO RECEIVE CLIENT MESSAGE\n");
        //fprintf(stderr, "errno: %s\n", strerror(errno));
        return NULL;
    }
    if (errc == 0) // client socket closed
        return remove_wrapper(connection, client_socket, epoll_instance);
    //struct connection_t *current = find_client(connection, client_socket);
    char *data = NULL;
    size_t size_cpt = 0;
    // tant qu'il n'y a pas de newline on concatene et on receive
    while (!concatenate_nlc(&data, buffer, errc, size_cpt))
    {
        size_cpt += errc;
        errc = recv(client_socket, buffer, DEFAULT_BUFFER_SIZE, 0); // 0 = flags / aucun flag
        if (errc == -1)
        {
            fprintf(stderr, "[CHATROOM SERVER HANDLE CLIENT EVENT] FAILED TO RECEIVE CLIENT MESSAGE\n");
            return NULL;
        }
        if (errc == 0) // client socket closed
            return remove_wrapper(connection, client_socket, epoll_instance);
    }
    size_cpt += errc;

    // printf("messaged received: \n");
    // write(STDOUT_FILENO, data, size_cpt);
    //
    // int err = client_action(epoll_instance, client_socket, server_socket, connection, data, sizeof(data));
    // on reagit a le requete

    struct connection_t *sender = find_client(connection, client_socket);

    struct connection_t *tmp;
    switch (connection->stage)
    {
        case CONNECTED_UNLINKED:
        case CONNECTED_UNNAMED:
            tmp = client_get_name(sender, data, size_cpt);
            break;
        case CONNECTED_IDENTIFIED:
            tmp = client_action(connection, data, size_cpt, sender);
            break;
    }
    if (!tmp)
    {
        close(client_socket);
        return remove_wrapper(connection, client_socket, epoll_instance);
    }
    return connection;
}


int setup_epoll(int ep_instance, int server_socket)
{
    // creation de l'event du server socket
    struct epoll_event server_event;
    server_event.events = EPOLLIN;
    server_event.data.fd = -1;

    // epoll:
    //          event = action detecter
    //          epoll_event.events = config des events
    //          CHATROOMIN = detecter une action quand le socket recoit qq chose
    //          epoll_event.data = data stocker dans chaque event
    //                  ici un socket client

    // ajout du socket a l'environnement epoll
    // option CHATROOM_CTL_ADD = action d'ajout
    if (epoll_ctl(ep_instance, EPOLL_CTL_ADD, server_socket, &server_event))
        errx(1, "[CHATROOM SERVER MAIN] FAILED TO ADD SERVER SOCKET TO CHATROOM LIST OF INTEREST\n");

    return ep_instance;
}

int main(int argc, char *argv[]) {
    if (argc != 3)
        errx(1, "[CHATROOM SERVER MAIN] WRONG ARGUMENTS\n");

    char *host = argv[1];
    char *port = argv[2];

    // creation d'une instance epoll
    int ep_instance = epoll_create1(0);
    if (ep_instance == -1)
        errx(1, "[CHATROOM SERVER MAIN] FAILED TO CREATE CHATROOM INSTANCE\n");

    // creation du socket server
    int server_socket = prepare_socket(host, port);
    ep_instance = setup_epoll(ep_instance, server_socket);

    struct connection_t *clients = NULL;
    // clients = structure contenant des informations sur tout les clients
    //           event contient un pointeur vers cet structure
    // struct connection_t *tmp;
    // sert a check si la valeur de retour des fcts auxilières est null auquel cas une erreur s'est produit
    // et clients ne doit pas etre modifier
    
    printf("Server started.\n\tConnect to server with: nc (ip) (port)\n\n");
    for (;;)
    {
        struct epoll_event sevents[MAX_EVENTS]; // server events

        // stocks events in sevents and event count in ecount
        int ecount = epoll_wait(ep_instance, sevents, MAX_EVENTS, -1);
        if (ecount == -1)
            errx(1, "[CHATROOM SERVER MAIN] FAILED TO WAIT FOR CLIENT SOCKET\n");

        // on parcours tout les events non traité
        for (int i = 0; i < ecount; i++)
        {
            if (sevents[i].data.fd == -1) // server socket event
            {
                struct connection_t *tmp = accept_client(ep_instance, server_socket, clients);
                if (tmp)
                    clients = tmp;
                // if tmp == NULL: error
            }
            else // client socket event
            {
                // il reste la edge case de message trop grand mais jamais eu de probleme ?
                struct connection_t *tmp = handle_client_event(ep_instance, sevents[i].data.fd, clients);
                if (tmp)
                    clients = tmp;
                // if tmp == NULL: error
            }
        }
    }
    close(server_socket);
    return 0;
}
