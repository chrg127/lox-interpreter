#ifndef MEMORY_H_INCLUDED
#define MEMORY_H_INCLUDED

#include <stddef.h>
// #include "value.h"

typedef struct Value Value;
typedef struct Obj Obj;
typedef struct Table Table;

void *reallocate(void *ptr, size_t old, size_t new);
void gc_collect();
void gc_mark_value(Value value);
void gc_mark_obj(Obj *obj);
void gc_mark_table(Table *tab);

#define ALLOCATE(type, count) \
    (type *) reallocate(NULL, 0, sizeof(type) * (count))

#define GROW_ARRAY(type, ptr, old, new) \
    (type *) reallocate(ptr, sizeof(type) * (old), sizeof(type) * (new))

#define FREE_ARRAY(type, ptr, old) \
    do { \
        reallocate(ptr, sizeof(type) * (old), 0); \
    } while (0)

#define FREE(type, ptr) reallocate(ptr, sizeof(type), 0)

#endif
