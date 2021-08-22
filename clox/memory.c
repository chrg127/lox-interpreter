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

static void free_obj(Obj *obj)
{
    switch (obj->type) {
    case OBJ_STRING: {
        ObjString *str = (ObjString *)obj;
        FREE_ARRAY(char, str->data, str->len+1);
        FREE(ObjString, obj);
        break;
    }
    }
}

void free_objects(Obj *objects)
{
    Obj *obj = objects;
    while (obj != NULL) {
        Obj *next = obj->next;
        free_obj(obj);
        obj = next;
    }
}
