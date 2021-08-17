#include "memory.h"

#include <stdlib.h>

void *reallocate(void *ptr, size_t old, size_t new)
{
    if (new == 0) {
        free(ptr);
        return NULL;
    }
    void *res = realloc(ptr, new);
    if (!res)
        abort();
    return res;
}
