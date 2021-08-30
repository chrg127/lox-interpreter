#ifndef OBJECT_H_INCLUDED
#define OBJECT_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include "uint.h"
#include "value.h"
#include "chunk.h"

typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_NATIVE,
} ObjType;

struct Obj {
    ObjType type;
    struct Obj *next;
};

struct ObjString {
    Obj obj;
    size_t len;
    // bool owning;
    //char data[];
    char *data;
    u32 hash;
};

typedef struct {
    Obj obj;
    int arity;
    Chunk chunk;
    ObjString *name;
} ObjFunction;

typedef Value (*NativeFn)(int argc, Value *argv);

typedef struct {
    Obj obj;
    NativeFn fun;
    const char *name;
} ObjNative;

#define OBJ_TYPE(value)     (AS_OBJ(value)->type)

static inline bool obj_is_type(Value value, ObjType type)
{
    return IS_OBJ(value) && OBJ_TYPE(value) == type;
}

#define IS_STRING(value)    obj_is_type((value), OBJ_STRING)
#define IS_FUNCTION(value)  obj_is_type((value), OBJ_FUNCTION)
#define IS_NATIVE(value)    obj_is_type((value), OBJ_NATIVE)

#define AS_STRING(value)    ((ObjString *) AS_OBJ(value))
#define AS_CSTRING(value)   (((ObjString *) AS_OBJ(value))->data)
#define AS_FUNCTION(value)  ((ObjFunction *) AS_OBJ(value))
#define AS_NATIVE(value)    (((ObjNative *) AS_OBJ(value))->fun)

ObjString *obj_copy_string(const char *str, size_t len);
ObjString *obj_take_string(char *data, size_t len);
ObjFunction *obj_make_fun();
ObjNative *obj_make_native(NativeFn fun, const char *name);
void obj_print(Value value);
void obj_free_arr(Obj *objects);
ObjString *obj_make_string_nonowning(char *str, size_t len);
u32 obj_hash(Obj *obj);
bool obj_strcmp(Value a, Value b);

#endif
