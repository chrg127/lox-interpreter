#ifndef OBJECT_H_INCLUDED
#define OBJECT_H_INCLUDED

#include <stddef.h>
#include "value.h"

typedef enum {
    OBJ_STRING,
} ObjType;

struct Obj {
    ObjType type;
    struct Obj *next;
};

struct ObjString {
    Obj obj;
    size_t len;
    char data[];
    //char *data;
};

#define OBJ_TYPE(value)     (AS_OBJ(value)->type)

static inline bool is_obj_type(Value value, ObjType type)
{
    return IS_OBJ(value) && OBJ_TYPE(value) == type;
}

#define IS_STRING(value)    is_obj_type((value), OBJ_STRING)

#define AS_STRING(value)    ((ObjString *) AS_OBJ(value))
#define AS_CSTRING(value)   (((ObjString *) AS_OBJ(value))->data)

ObjString *copy_string(const char *str, size_t len);
ObjString *take_string(char *data, size_t len);
void object_print(Value value);

#endif
