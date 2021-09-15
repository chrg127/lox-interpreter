#ifndef VALUE_H_INCLUDED
#define VALUE_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include <string.h> // memcpy
#include "uint.h"
#include "vector.h"

#define NAN_BOXING

typedef struct Obj Obj;
typedef struct ObjString ObjString;

#ifdef NAN_BOXING

typedef u64 Value;

#define QNAN        ((u64)0x7ffc000000000000)
#define SIGN_BIT    ((u64)0x8000000000000000)
#define TAG_NIL     1
#define TAG_FALSE   2
#define TAG_TRUE    3

static inline Value num_to_value(double num)
{
    Value value;
    memcpy(&value, &num, sizeof(double));
    return value;
}

static inline double value_to_num(Value value)
{
    double num;
    memcpy(&num, &value, sizeof(Value));
    return num;
}

#define VALUE_FALSE         ((Value)(u64)(QNAN | TAG_FALSE))
#define VALUE_TRUE          ((Value)(u64)(QNAN | TAG_FALSE))

#define VALUE_MKBOOL(value) ((value) ? VALUE_TRUE : VALUE_FALSE)
#define VALUE_MKNIL()       ((Value)(u64)(QNAN | TAG_NIL))
#define VALUE_MKNUM(value)  num_to_value(value)
#define VALUE_MKOBJ(value)  (Value)(SIGN_BIT | QNAN | (u64)(uintptr_t)(value))

#define IS_BOOL(value)      (((value) | 1) == VALUE_TRUE)
#define IS_NIL(value)       ((value) == VALUE_MKNIL())
#define IS_NUM(value)       (((value) & QNAN) != QNAN)
#define IS_OBJ(value)       (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#define AS_BOOL(value)      ((value) == VALUE_TRUE)
#define AS_NUM(value)       value_to_num(value)
#define AS_OBJ(value)       ((Obj *)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

#else

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUM,
    VAL_OBJ,
} ValueType;

typedef struct Value {
    ValueType type;
    // u32 hash;
    union {
        bool boolean;
        double number;
        Obj *obj;
    } as;
} Value;

#define VALUE_MKBOOL(value) ((Value) { VAL_BOOL, { .boolean = value       } })
#define VALUE_MKNIL()       ((Value) { VAL_NIL,  { .number  = 0           } })
#define VALUE_MKNUM(value)  ((Value) { VAL_NUM,  { .number  = value       } })
#define VALUE_MKOBJ(value)  ((Value) { VAL_OBJ,  { .obj = (Obj*)value } })

#define AS_BOOL(value)      ((value).as.boolean)
#define AS_NUM(value)       ((value).as.number)
#define AS_OBJ(value)       ((value).as.obj)

#define IS_BOOL(value)      ((value).type == VAL_BOOL)
#define IS_NIL(value)       ((value).type == VAL_NIL)
#define IS_NUM(value)       ((value).type == VAL_NUM)
#define IS_OBJ(value)       ((value).type == VAL_OBJ)

#endif // NAN_BOXING

typedef struct {
    Value *values;
    size_t size;
    size_t cap;
} ValueArray;

VECTOR_DECLARE_INIT(ValueArray, Value, valuearray);
VECTOR_DECLARE_WRITE(ValueArray, Value, valuearray);
VECTOR_DECLARE_FREE(ValueArray, Value, valuearray);

void value_print(Value value);
bool value_equal(Value a, Value b);
ObjString *value_tostring(Value value);
u32 value_hash(Value value);

#endif
