#include "xalloc.h"

#include <err.h>
#include <stdlib.h>

void *xmalloc(size_t size)
{
    void *res = malloc(size);
    if (!res)
        err(EXIT_FAILURE, "Impossible to malloc");

    return res;
}

void *xcalloc(size_t nmemb, size_t size)
{
    void *res = calloc(nmemb, size);
    if (!res)
        err(EXIT_FAILURE, "Impossible to calloc");

    return res;
}

void *xrealloc(void *ptr, size_t size)
{
    void *res = realloc(ptr, size);
    if (!res)
        err(EXIT_FAILURE, "Impossible to realloc");

    return res;
}
