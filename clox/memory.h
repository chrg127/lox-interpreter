#ifndef MEMORY_H_INCLUDED
#define MEMORY_H_INCLUDED

#include <stddef.h>

void *reallocate(void *ptr, size_t old, size_t new);

#define GROW_ARRAY(type, ptr, old, new) \
    (type *) reallocate(ptr, sizeof(type) * (old), sizeof(type) * (new))

#define FREE_ARRAY(type, ptr, old) \
    do { \
        reallocate(ptr, sizeof(type) * (old), 0); \
    } while (0)


#endif
