#include "value.h"

#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "object.h"

VECTOR_DEFINE_INIT(ValueArray, Value, valuearray, values)
VECTOR_DEFINE_WRITE(ValueArray, Value, valuearray, values)
VECTOR_DEFINE_FREE(ValueArray, Value, valuearray, values)

void value_print(Value value, bool debug)
{
         if (IS_BOOL(value)) printf(AS_BOOL(value) ? "true" : "false");
    else if (IS_NIL(value))  printf("nil");
    else if (IS_NUM(value))  printf("%g", AS_NUM(value));
    else if (IS_OBJ(value))  obj_print(value, debug);
    else if (IS_SSTR(value)) printf(debug ? "\"%s\"" : "%s", AS_SSTR(value));
    else printf("ERROR");
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
    case VAL_SSTR:  return memcmp(AS_SSTR(a), AS_SSTR(b), VALUE_SSO_SIZE) == 0;
    default:        return false; // unreachable
    }
#endif
}

static Value num_tostring(double num)
{
    int len = snprintf(NULL, 0, "%g", num) + 1;
    if ((size_t)len < VALUE_SSO_SIZE) {
        char output[VALUE_SSO_SIZE];
        snprintf(output, len, "%g", num);
        return value_mksstr(output, len-1);
    }
    char *output = ALLOCATE(char, len);
    snprintf(output, len, "%g", num);
    return VALUE_MKOBJ(obj_take_string(output, len-1));
}

Value value_tostring(Value value)
{
         if (IS_NUM(value))  return num_tostring(AS_NUM(value));
    else if (IS_BOOL(value)) return AS_BOOL(value) ? obj_make_ssostring("true",  4)
                                                   : obj_make_ssostring("false", 5);
    else if (IS_NIL(value))  return obj_make_ssostring("nil", 3);
    else if (IS_OBJ(value))  return obj_tostring(value);
    else if (IS_SSTR(value)) return value;
    else                     return obj_make_ssostring("ERROR", 5);
}

u32 value_hash(Value value)
{
         if (IS_BOOL(value)) return AS_BOOL(value);
    else if (IS_NUM(value))  return AS_NUM(value);
    else if (IS_NIL(value))  return 0;
    else if (IS_OBJ(value))  return obj_hash(AS_OBJ(value));
    else if (IS_SSTR(value)) return hash_string(AS_SSTR(value), VALUE_SSO_SIZE);
    else return 0;
}
