#include <sys/socket.h>
#include "utils.h"

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
