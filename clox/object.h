#ifndef OBJECT_H_INCLUDED
#define OBJECT_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include "uint.h"
#include "value.h"
#include "chunk.h"
#include "native.h"
#include "table.h"
#include "vector.h"

typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_UPVALUE,
    OBJ_CLOSURE,
    OBJ_CLASS,
    OBJ_INSTANCE,
    OBJ_BOUND_METHOD,
    OBJ_ARRAY,
} ObjType;

struct Obj {
    ObjType type;
    bool marked;
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
    const char *file;
} ObjFunction;

typedef struct {
    Obj obj;
    NativeFn fun;
    u8 arity;
    const char *name;
} ObjNative;

typedef struct ObjUpvalue {
    Obj obj;
    Value *location;            // pointer to the value it closes over
    Value closed;               // location of the closed variable after the upvalue becomes closed
    struct ObjUpvalue *next;
} ObjUpvalue;

typedef struct {
    Obj obj;
    ObjFunction *fun;
    ObjUpvalue **upvalues;
    int upvalue_count;
} ObjClosure;

typedef struct {
    Obj obj;
    ObjString *name;
    Value ctor;
    Table methods;
    Table statics;
} ObjClass;

typedef struct {
    Obj obj;
    ObjClass *klass;
    Table fields;
} ObjInstance;

typedef struct {
    Obj obj;
    Value receiver;
    Value method;
} ObjBoundMethod;

typedef struct {
    Obj obj;
    Value *data;
    size_t len;
} ObjArray;

#define OBJ_TYPE(value)     (AS_OBJ(value)->type)

static inline bool obj_is_type(Value value, ObjType type)
{
    return IS_OBJ(value) && OBJ_TYPE(value) == type;
}

#define IS_STRING(value)        obj_is_type((value), OBJ_STRING)
#define IS_FUNCTION(value)      obj_is_type((value), OBJ_FUNCTION)
#define IS_NATIVE(value)        obj_is_type((value), OBJ_NATIVE)
#define IS_CLOSURE(value)       obj_is_type((value), OBJ_CLOSURE)
#define IS_CLASS(value)         obj_is_type((value), OBJ_CLASS)
#define IS_INSTANCE(value)      obj_is_type((value), OBJ_INSTANCE)
#define IS_BOUND_METHOD(value)  obj_is_type((value), OBJ_BOUND_METHOD)
#define IS_ARRAY(value)         obj_is_type((value), OBJ_ARRAY)

#define AS_STRING(value)        ((ObjString *)      AS_OBJ(value))
#define AS_CSTRING(value)       (((ObjString *)     AS_OBJ(value))->data)
#define AS_FUNCTION(value)      ((ObjFunction *)    AS_OBJ(value))
#define AS_NATIVE(value)        (((ObjNative *)     AS_OBJ(value))->fun)
#define AS_NATIVE_OBJ(value)    ((ObjNative *)      AS_OBJ(value))
#define AS_CLOSURE(value)       ((ObjClosure *)     AS_OBJ(value))
#define AS_CLASS(value)         ((ObjClass *)       AS_OBJ(value))
#define AS_INSTANCE(value)      ((ObjInstance *)    AS_OBJ(value))
#define AS_BOUND_METHOD(value)  ((ObjBoundMethod *) AS_OBJ(value))
#define AS_ARRAY(value)         ((ObjArray *)       AS_OBJ(value))

ObjString *obj_copy_string(const char *str, size_t len);
ObjString *obj_take_string(char *data, size_t len);
ObjFunction *obj_make_fun(ObjString *name, const char *filename);
ObjNative *obj_make_native(NativeFn fun, const char *name, u8 arity);
ObjUpvalue *obj_make_upvalue(Value *slot);
ObjClosure *obj_make_closure(ObjFunction *fun);
ObjClass *obj_make_class(ObjString *name);
ObjInstance *obj_make_instance(ObjClass *klass);
ObjBoundMethod *obj_make_bound_method(Value receiver, Value method);
ObjArray *obj_make_array(size_t len, Value *elems);
void obj_print(Value value, bool debug);
void obj_free(Obj *obj);
void obj_free_arr(Obj *objects);
u32 obj_hash(Obj *obj);
bool obj_strcmp(Value a, Value b);
Value obj_tostring(Value value);
Value obj_concat(Value a, Value b);

static inline Value obj_make_ssostring(const char *str, size_t len)
{
    return len < VALUE_SSO_SIZE ? value_mksstr(str, len)
                                : VALUE_MKOBJ(obj_copy_string(str, len));
}

#endif
