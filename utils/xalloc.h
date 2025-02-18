#ifndef XALLOC_H
#define XALLOC_H

#include <stddef.h>

// wrappers arround dynamic allocation functions
// in order to avoid code duplication
void *xmalloc(size_t size);
void *xcalloc(size_t nmemb, size_t size);
void *xrealloc(void *ptr, size_t size);

#endif /* !XALLOC_H */
