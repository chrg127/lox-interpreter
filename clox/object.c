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

static ObjString *alloc_str(char *data, size_t len, u32 hash)
{
    ObjString *str = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    str->len  = len;
    str->data = data;
    str->hash = hash;
    table_install(&vm.strings, str, VALUE_MKNIL());
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

ObjString *obj_take_string(char *data, size_t len)
{
    u32 hash = hash_string(data, len);
    ObjString *interned = table_find_string(&vm.strings, data, len, hash);
    if (interned != NULL) {
        FREE_ARRAY(char, data, len + 1);
        return interned;
    }
    return alloc_str(data, len, hash);
}

ObjFunction *obj_make_fun()
{
    ObjFunction *fun = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    fun->arity = 0;
    fun->name = NULL;
    chunk_init(&fun->chunk);
    return fun;
}

ObjNative *obj_make_native(NativeFn fun, const char *name, u8 arity)
{
    ObjNative *native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->fun  = fun;
    native->name = name;
    native->arity = arity;
    return native;
}

static void print_function(ObjFunction *fun)
{
    if (fun->name == NULL) {
        printf("<script>");
        return;
    }
    printf("<fn %s>", fun->name->data);
}

void obj_print(Value value)
{
    switch (OBJ_TYPE(value)) {
    case OBJ_STRING: printf("\"%.*s\"", (int) AS_STRING(value)->len, AS_STRING(value)->data); break;
    case OBJ_FUNCTION: print_function(AS_FUNCTION(value)); break;
    case OBJ_NATIVE: printf("<native fn '%s'>", ((ObjNative *)AS_OBJ(value))->name); break;
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
    case OBJ_FUNCTION: {
        ObjFunction *fun = (ObjFunction *)obj;
        chunk_free(&fun->chunk);
        FREE(ObjFunction, obj);
        break;
    }
    case OBJ_NATIVE:
        FREE(ObjNative, obj);
        break;
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
    case OBJ_FUNCTION: return ((ObjFunction *)obj)->name->hash;
    case OBJ_NATIVE: {
        const char *str = ((ObjNative *)obj)->name;
        return hash_string(str, strlen(str));
    }
    default: return 0;
    }
}
