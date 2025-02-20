#include "connection.h"

_Bool client_command(struct connection_t *connection, char *data, ssize_t data_length, struct connection_t *sender);
void clear_client_screen(struct connection_t *target);
