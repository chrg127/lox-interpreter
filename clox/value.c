#include "value.h"

#include <stdio.h>
#include "memory.h"
#include "object.h"

VECTOR_DEFINE_INIT(ValueArray, Value, valuearray, values)
VECTOR_DEFINE_WRITE(ValueArray, Value, valuearray, values)
VECTOR_DEFINE_FREE(ValueArray, Value, valuearray, values)

void value_print(Value value)
{
#ifdef NAN_BOXING
         if (IS_BOOL(value)) printf(AS_BOOL(value) ? "true" : "false");
    else if (IS_NIL(value))  printf("nil");
    else if (IS_NUM(value))  printf("%g", AS_NUM(value));
    else if (IS_OBJ(value))  obj_print(value);
    else printf("ERROR");
#else
    switch (value.type) {
    case VAL_BOOL: printf(AS_BOOL(value) ? "true" : "false"); break;
    case VAL_NIL:  printf("nil");                             break;
    case VAL_NUM:  printf("%g", AS_NUM(value));               break;
    case VAL_OBJ:  obj_print(value);                          break;
    }
#endif
}

bool value_equal(Value a, Value b)
{
#ifdef NAN_BOXING
    if (IS_NUM(a) && IS_NUM(b))
        return AS_NUM(a) == AS_NUM(b);
    return a == b;
#else
    if (a.type != b.type)
        return false;
    switch (a.type) {
    case VAL_BOOL:  return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:   return true;
    case VAL_NUM:   return AS_NUM(a) == AS_NUM(b);
    case VAL_OBJ:   return AS_OBJ(a) == AS_OBJ(b);
    default:        return false; // unreachable
    }
#endif
}
