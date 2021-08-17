#include "value.h"

#include <stdio.h>
#include <stddef.h>
#include "memory.h"
#include "vector.h"

void valuearray_init(ValueArray *arr)
{
    VECTOR_INIT(arr, values);
}

void valuearray_write(ValueArray *arr, Value value)
{
    if (arr->cap < arr->size + 1) {
        size_t old = arr->cap;
        arr->cap = vector_grow_cap(old);
        arr->values = GROW_ARRAY(Value, arr->values, old, arr->cap);
    }
    arr->values[arr->size++] = value;
}

void valuearray_free(ValueArray *arr)
{
    FREE_ARRAY(Value, arr->values, arr->cap);
    valuearray_init(arr);
}

void value_print(Value value)
{
    printf("%g", value);
}
