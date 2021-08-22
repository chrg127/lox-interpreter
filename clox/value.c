#include "value.h"

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "memory.h"
#include "vector.h"
#include "object.h"

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
    switch (value.type) {
    case VAL_BOOL: printf(AS_BOOL(value) ? "true" : "false"); break;
    case VAL_NIL:  printf("nil");                             break;
    case VAL_NUM:  printf("%g", AS_NUM(value));               break;
    case VAL_OBJ:  object_print(value);                       break;
    }
}

bool value_equal(Value a, Value b)
{
    if (a.type != b.type)
        return false;
    switch (a.type) {
    case VAL_BOOL:  return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:   return true;
    case VAL_NUM:   return AS_NUM(a) == AS_NUM(b);
    case VAL_OBJ:
        ObjString *as = AS_STRING(a);
        ObjString *bs = AS_STRING(b);
        return as->len == bs->len && memcmp(as->data, bs->data, as->len) == 0;
    default:        return false; // unreachable
    }
}
