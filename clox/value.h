#ifndef VALUE_H_INCLUDED
#define VALUE_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include "uint.h"

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUM,
    VAL_OBJ,
} ValueType;

typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef struct {
    ValueType type;
    u32 hash;
    union {
        bool boolean;
        double number;
        Obj *obj;
    } as;
} Value;

u32 hash_num(double n);
u32 hash_bool(bool b);

#define VALUE_MKBOOL(value) ((Value) { VAL_BOOL, hash_bool(value),  { .boolean = value       } })
#define VALUE_MKNIL()       ((Value) { VAL_NIL,  0,                { .number  = 0           } })
#define VALUE_MKNUM(value)  ((Value) { VAL_NUM,  hash_num(value), { .number  = value       } })
#define VALUE_MKOBJ(value)  ((Value) { VAL_OBJ,  obj_hash((Obj*)value),  { .obj     = (Obj*)value } })

#define AS_BOOL(value)      ((value).as.boolean)
#define AS_NUM(value)       ((value).as.number)
#define AS_OBJ(value)       ((value).as.obj)

#define IS_BOOL(value)      ((value).type == VAL_BOOL)
#define IS_NIL(value)       ((value).type == VAL_NIL)
#define IS_NUM(value)       ((value).type == VAL_NUM)
#define IS_OBJ(value)       ((value).type == VAL_OBJ)

typedef struct {
    Value *values;
    size_t size;
    size_t cap;
} ValueArray;

void valuearray_init(ValueArray *arr);
void valuearray_write(ValueArray *arr, Value value);
void valuearray_free(ValueArray *arr);

void value_print(Value value);
bool value_equal(Value a, Value b);

#endif
