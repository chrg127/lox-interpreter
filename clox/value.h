#ifndef VALUE_H_INCLUDED
#define VALUE_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUM,
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
    } as;
} Value;

#define VALUE_MKBOOL(value) ((Value) { VAL_BOOL, { .boolean = value } })
#define VALUE_MKNIL()       ((Value) { VAL_NIL,  { .number  = 0     } })
#define VALUE_MKNUM(value)  ((Value) { VAL_NUM,  { .number  = value } })

#define AS_BOOL(value)      ((value).as.boolean)
#define AS_NUM(value)       ((value).as.number)

#define IS_BOOL(value)      ((value).type == VAL_BOOL)
#define IS_NIL(value)       ((value).type == VAL_NIL)
#define IS_NUM(value)       ((value).type == VAL_NUM)

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
