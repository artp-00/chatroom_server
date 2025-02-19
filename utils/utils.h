#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// xalloc
// wrappers arround dynamic allocation functions
// in order to avoid code duplication
void *xmalloc(size_t size);
void *xcalloc(size_t nmemb, size_t size);
void *xrealloc(void *ptr, size_t size);

// net_utils
int send_data(int client_socket, char *data, ssize_t data_length);

#endif /* !UTILS_H */
