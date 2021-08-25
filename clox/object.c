#include "object.h"

#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "table.h"
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

// static ObjString *alloc_str(char *data, size_t len, bool owning)
static ObjString *alloc_str(char *data, size_t len, u32 hash)
{
    // using flexible array member
    // ObjString *str = (ObjString *) alloc_obj(sizeof(ObjString) + sizeof(char)*len, OBJ_STRING);
    ObjString *str = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    str->len  = len;
    str->data = data;

    // str->owning = owning;
    // memcpy(str->data, data, len);

    str->hash = hash;
    table_install(&vm.strings, VALUE_MKOBJ(str), VALUE_MKNIL());
    return str;
}

// algorithm: FNV-1a
static u32 hash_string(const char *str, size_t len)
{
    u32 hash = 2166136261u;
    for (size_t i = 0; i < len; i++) {
        hash ^= (u8) str[i];
        hash *= 16777619;
    }
    return hash;
}

ObjString *obj_copy_string(const char *str, size_t len)
{
    u32 hash = hash_string(str, len);
    ObjString *interned = table_find_string(&vm.strings, str, len, hash);
    if (interned != NULL)
        return interned;
    char *newstr = ALLOCATE(char, len + 1);
    memcpy(newstr, str, len);
    newstr[len] = '\0';
    return alloc_str(newstr, len, hash);
}

ObjString *obj_make_string_nonowning(char *str, size_t len)
{
    return NULL;
    // return alloc_str(str, len, false);
}

ObjString *obj_take_string(char *data, size_t len)
{
    // return alloc_str(data, len, true);

    u32 hash = hash_string(data, len);
    ObjString *interned = table_find_string(&vm.strings, data, len, hash);
    if (interned != NULL) {
        FREE_ARRAY(char, data, len + 1);
        return interned;
    }
    return alloc_str(data, len, hash);
}

void obj_print(Value value)
{
    switch (OBJ_TYPE(value)) {
    case OBJ_STRING: printf("\"%.*s\"", (int) AS_STRING(value)->len, AS_STRING(value)->data); break;
    }
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

void obj_free_arr(Obj *objects)
{
    Obj *obj = objects;
    while (obj != NULL) {
        Obj *next = obj->next;
        free_obj(obj);
        obj = next;
    }
}

u32 obj_hash(Obj *obj)
{
    switch (obj->type) {
    case OBJ_STRING: return ((ObjString *)obj)->hash;
    default: return 0;
    }
}
