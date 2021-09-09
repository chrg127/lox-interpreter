#ifndef OBJECT_H_INCLUDED
#define OBJECT_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include "uint.h"
#include "value.h"
#include "chunk.h"
#include "native.h"

typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_CLOSURE,
    OBJ_UPVALUE,
} ObjType;

struct Obj {
    ObjType type;
    struct Obj *next;
};

struct ObjString {
    Obj obj;
    size_t len;
    char *data;
    u32 hash;
};

typedef struct {
    Obj obj;
    int arity;
    int upvalue_count;
    Chunk chunk;
    ObjString *name;
} ObjFunction;

typedef struct {
    Obj obj;
    NativeFn fun;
    u8 arity;
    const char *name;
} ObjNative;

typedef struct ObjUpvalue {
    Obj obj;
    Value *location;
    Value closed;
    struct ObjUpvalue *next;
} ObjUpvalue;

typedef struct {
    Obj obj;
    ObjFunction *fun;
    ObjUpvalue **upvalues;
    int upvalue_count;
} ObjClosure;

#define OBJ_TYPE(value)     (AS_OBJ(value)->type)

static inline bool obj_is_type(Value value, ObjType type)
{
    return IS_OBJ(value) && OBJ_TYPE(value) == type;
}

#define IS_STRING(value)    obj_is_type((value), OBJ_STRING)
#define IS_FUNCTION(value)  obj_is_type((value), OBJ_FUNCTION)
#define IS_NATIVE(value)    obj_is_type((value), OBJ_NATIVE)
#define IS_CLOSURE(value)   obj_is_type((value), OBJ_CLOSURE)

#define AS_STRING(value)    ((ObjString *) AS_OBJ(value))
#define AS_CSTRING(value)   (((ObjString *) AS_OBJ(value))->data)
#define AS_FUNCTION(value)  ((ObjFunction *) AS_OBJ(value))
#define AS_NATIVE(value)    (((ObjNative *) AS_OBJ(value))->fun)
#define AS_NATIVE_OBJ(value) ((ObjNative *) AS_OBJ(value))
#define AS_CLOSURE(value)   ((ObjClosure *) AS_OBJ(value))

ObjString *obj_copy_string(const char *str, size_t len);
ObjString *obj_take_string(char *data, size_t len);
ObjFunction *obj_make_fun();
ObjNative *obj_make_native(NativeFn fun, const char *name, u8 arity);
ObjClosure *obj_make_closure(ObjFunction *fun);
ObjUpvalue *obj_make_upvalue(Value *slot);
void obj_print(Value value);
void obj_free_arr(Obj *objects);
u32 obj_hash(Obj *obj);
bool obj_strcmp(Value a, Value b);
ObjString *obj_tostring(Value value);

#endif
