#include "object.h"

#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "vm.h"

static Obj *alloc_obj(size_t size, ObjType type)
{
    Obj *obj = reallocate(NULL, 0, size);
    obj->type = type;
    obj->next = vm.objects;
    vm.objects = obj;
    return obj;
}

#define ALLOCATE_OBJ(type, obj_type) \
    (type *) alloc_obj(sizeof(type), obj_type)

static ObjString *alloc_str(char *data, size_t len)
{
    ObjString *str = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    str->len = len;
    str->data = data;
    return str;
}

ObjString *obj_copy_string(const char *str, size_t len)
{
    char *newstr = ALLOCATE(char, len + 1);
    memcpy(newstr, str, len);
    newstr[len] = '\0';
    return alloc_str(newstr, len);
}

ObjString *obj_take_string(char *data, size_t len)
{
    return alloc_str(data, len);
}

void obj_print(Value value)
{
    switch (OBJ_TYPE(value)) {
    case OBJ_STRING: printf("\"%.*s\"", (int) AS_STRING(value)->len, AS_STRING(value)->data); break;
    }
}
