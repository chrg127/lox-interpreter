#include "value.h"

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "memory.h"
#include "object.h"

VECTOR_DEFINE_INIT(ValueArray, Value, valuearray, values)
VECTOR_DEFINE_WRITE(ValueArray, Value, valuearray, values)
VECTOR_DEFINE_FREE(ValueArray, Value, valuearray, values)

void value_print(Value value)
{
    switch (value.type) {
    case VAL_BOOL: printf(AS_BOOL(value) ? "true" : "false"); break;
    case VAL_NIL:  printf("nil");                             break;
    case VAL_NUM:  printf("%g", AS_NUM(value));               break;
    case VAL_OBJ:  obj_print(value);                          break;
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
    case VAL_OBJ:   return AS_OBJ(a) == AS_OBJ(b);
    default:        return false; // unreachable
    }
}

u32 hash_num(double n)
{
    return n;
}

u32 hash_bool(bool b)
{
    return b;
}
