#ifndef VALUE_H_INCLUDED
#define VALUE_H_INCLUDED

#include <stddef.h>

typedef double Value;

typedef struct {
    Value *values;
    size_t size;
    size_t cap;
} ValueArray;

void valuearray_init(ValueArray *arr);
void valuearray_write(ValueArray *arr, Value value);
void valuearray_free(ValueArray *arr);

void value_print(Value value);

#endif
