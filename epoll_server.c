#include <err.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <errno.h>

#include "epoll_server.h"
#include "utils/xalloc.h"


int create_and_bind(struct addrinfo *addrinfo)
{
    struct addrinfo *p;
    int optval = 1;
    // en gros true ou false
    // pour chaque options mises a jour par setsockopt
    int s;
    // addrinfo renvoie une liste de socket/parametrage possiblement utilisable
    // on itere sur cet liste pour trouver un bindable
    for (p = addrinfo; p; p = p->ai_next)
    {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        // ai->family | PF_INET
        // ai->protocol | 0 = tcp protocol
        if (s == -1)
            continue;
        // configure le socket
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
        fprintf(stderr, "[EPOLL SERVER CREATE AND BIND] FAILED TO CONNECT\n");
        return -1;
    }
    return s;
}

int prepare_socket(const char *ip, const char *port)
{
    struct addrinfo *hints = xmalloc(sizeof(struct addrinfo)); // criteres de selection de l'addresse renvoyer par getaddrinfo()
    hints->ai_family = AF_INET; // IPv4
    hints->ai_socktype = 0; // nimporte quel type de socket | a changer ?
    //hints->ai_socktype = SOCK_STREAM;
    hints->ai_protocol = IPPROTO_TCP; // TCP
    hints->ai_flags = 0; // aucun flag
    //hints->ai_flags = AI_V4MAPPED | AI_ADDRCONFIG;
    // plus de detail sur man page getaddrinfo

    struct addrinfo *res; // addrinfo result struct

    if (getaddrinfo(ip, port, hints, &res) != 0)
    {
        fprintf(stderr, "[EPOLL SERVER PREPARE SOCKET] FAILED TO GET ADDRESS INFO | ip: %s, port: %s\n", ip, port);
        free(hints);
        return -1;
    }

    free(hints);
    int s = create_and_bind(res); // res est free

    if (listen(s, SOMAXCONN) == -1) // deuxieme param = maximum de connections
         errx(1, "[EPOLL SERVER PREPARE SOCKET] FAILED TO LISTEN TO SOCKET\n");
        // jsp si errx close le socket

    return s;
}

struct connection_t *accept_client(int epoll_instance, int server_socket,
                                   struct connection_t *connection)
{
    // on accepte la premiere connection vers le serveur et creer un socket permettant de communiquer
    // pas sur des params null
    int new_client_socket = accept(server_socket, NULL, NULL);
    // erreur ici peut etre
    if (new_client_socket == -1)
    {
        fprintf(stderr, "[EPOLL SERVER ACCEPT CLIENT] FAILED TO FETCH NEW CLIENT SOCKET\n");
        return NULL;
    }
    // ajout client dans le structure
    struct connection_t *nc = add_client(connection, new_client_socket); // nc = new connection | pas sur si besoin de faire une nouvelle variable
    // setup de l'event client
    struct epoll_event client_event;
    client_event.events = EPOLLIN;
    client_event.data.fd = new_client_socket;

    // ajout du client a l'instance epoll
    if (epoll_ctl(epoll_instance, EPOLL_CTL_ADD, new_client_socket, &client_event))
    {
        fprintf(stderr, "[EPOLL SERVER ACCEPT CLIENT] FAILED TO ADD CLIENT TO EPOLL INSTANCE\n");
        close(new_client_socket);
        return NULL;
    }
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

// struct connection_t *client_action(struct connection_t *connection, int client_socket, int server_socket, char *data, ssize_t data_length)
struct connection_t *client_action(struct connection_t *connection, char *data, ssize_t data_length)
{
    // fonctions changeant en fonction de l'utilisation du serveur
    // actuellement echo
    // devrait etre un broadcast

    struct connection_t *p;
    for (p = connection; p; p = p->next)
        if (send_data(p->client_socket, data, data_length) != 0)
            fprintf(stderr, "[EPOLL SERVER CLIENT ACTION] FAILED TO BROADCAST MESSAGE TO A CLIENT\n");


    return connection;
}

struct connection_t *remove_wrapper(struct connection_t *connection, int client_socket, int epoll_instance)
{
    if (epoll_ctl(epoll_instance, EPOLL_CTL_DEL, client_socket, NULL) == -1)
        errx(1, "[EPOLL SERVER HANDLE CLIENT EVENT] FAILED TO REMOVE SOCKET FROM EPOLL INSTANCE\n");

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
        fprintf(stderr, "[EPOLL SERVER HANDLE CLIENT EVENT] FAILED TO RECEIVE CLIENT MESSAGE\n");
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
            fprintf(stderr, "[EPOLL SERVER HANDLE CLIENT EVENT] FAILED TO RECEIVE CLIENT MESSAGE\n");
            //fprintf(stderr, "errno: %s\n", strerror(errno));
            return NULL;
        }
        if (errc == 0) // client socket closed
            return remove_wrapper(connection, client_socket, epoll_instance);
    }
    size_cpt += errc;

    //printf("messaged received: \n");
    //write(STDOUT_FILENO, data, size_cpt);

    //int err = client_action(epoll_instance, client_socket, server_socket, connection, data, sizeof(data));
    //on reagit a le requete
    struct connection_t *tmp = client_action(connection, data, size_cpt);
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
    //          EPOLLIN = detecter une action quand le socket recoit qq chose
    //          epoll_event.data = data stocker dans chaque event
    //                  ici un socket client

    // ajout du socket a l'environnement epoll
    // option EPOLL_CTL_ADD = action d'ajout
    if (epoll_ctl(ep_instance, EPOLL_CTL_ADD, server_socket, &server_event))
        errx(1, "[EPOLL SERVER MAIN] FAILED TO ADD SERVER SOCKET TO EPOLL LIST OF INTEREST\n");

    return ep_instance;
}

int main(int argc, char *argv[]) {
    if (argc != 3)
        errx(1, "[EPOLL SERVER MAIN] WRONG ARGUMENTS\n");

    char *host = argv[1];
    char *port = argv[2];

    // creation d'une instance epoll
    int ep_instance = epoll_create1(0);
    if (ep_instance == -1)
        errx(1, "[EPOLL SERVER MAIN] FAILED TO CREATE EPOLL INSTANCE\n");

    // creation du socket server
    int server_socket = prepare_socket(host, port);
    ep_instance = setup_epoll(ep_instance, server_socket);

    struct connection_t *clients = NULL;
    // clients = structure contenant des informations sur tout les clients
    //           event contient un pointeur vers cet structure
    //struct connection_t *tmp;
    // sert a check si la valeur de retour des fcts auxilières est null auquel cas une erreur s'est produit
    // et clients ne doit pas etre modifier
    for (;;)
    {
        struct epoll_event sevents[MAX_EVENTS]; // server events

        //printf("Waiting for connections...\n");
        int ecount = epoll_wait(ep_instance, sevents, MAX_EVENTS, -1);
        // stock tout les events dans sevents et le nombre devent dans ecount
        // // fonction bloquante
        if (ecount == -1)
            errx(1, "[EPOLL SERVER MAIN] FAILED TO WAIT FOR CLIENT SOCKET\n");

        // on parcours tout les events non traité
        for (int i = 0; i < ecount; i++)
        {
            //printf("[EPOLL SERVER DEBUG] INLOOP: processing event %d of ecount: %d\n", i, ecount);
            if (sevents[i].data.fd == -1) // server socket event
            {
                struct connection_t *tmp = accept_client(ep_instance, server_socket, clients);
                if (tmp)
                    clients = tmp;
                // si tmp == NULL: erreur
            }
            else // client socket event
            {
                // il reste la edge case de message trop grand mais jamais eu de probleme ?
                struct connection_t *tmp = handle_client_event(ep_instance, sevents[i].data.fd, clients);
                if (tmp)
                    clients = tmp;
                // si tmp == NULL: erreur
            }

        }
    }

    close(server_socket);
    return 0;
}
