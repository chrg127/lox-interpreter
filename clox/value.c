#include "value.h"

#include <stdio.h>
#include <string.h>
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
    case VAL_SSTR: printf("\"%s\"", AS_SSTR(value)); break;
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
    case VAL_SSTR:  return memcmp(AS_SSTR(a), AS_SSTR(b), VALUE_SSO_SIZE) == 0;
    default:        return false; // unreachable
    }
#endif
}

Value num_tostring(double num)
{
    int len = snprintf(NULL, 0, "%f", num);
    char *output = ALLOCATE(char, len);
    snprintf(output, len, "%f", num);
    return VALUE_MKOBJ(obj_take_string(output, len-1));
}

Value value_tostring(Value value)
{
// #ifdef NAN_BOXING
    if (IS_NUM(value))       return num_tostring(AS_NUM(value));
    else if (IS_BOOL(value)) return AS_BOOL(value) ? obj_make_ssostring("true", 4) : obj_make_ssostring("false", 5);
    else if (IS_NIL(value))  return obj_make_ssostring("nil", 3);
    else if (IS_OBJ(value))  return obj_tostring(value);
    else return VALUE_MKNIL();
// #else
//     switch (value.type) {
//     case VAL_NUM:  return num_tostring(AS_NUM(value));
//     case VAL_BOOL: return AS_BOOL(value) ? obj_copy_string("true", 4) : obj_copy_string("false", 5);
//     case VAL_NIL:  return obj_copy_string("nil", 3);
//     case VAL_OBJ:  return obj_tostring(value);
//     case VAL_SSTR: return obj_copy_string(AS_SSTR(value), VALUE_SSO_SIZE);
//     default: return NULL; // unreachable
//     }
// #endif
}

u32 value_hash(Value value)
{
#ifdef NAN_BOXING
         if (IS_BOOL(value)) return AS_BOOL(value);
    else if (IS_NUM(value))  return AS_NUM(value);
    else if (IS_NIL(value))  return 0;
    else if (IS_OBJ(value))  return obj_hash(AS_OBJ(value));
    else return 0;
#else
    switch (value.type) {
    case VAL_BOOL: return AS_BOOL(value);
    case VAL_NUM:  return AS_NUM(value);
    case VAL_NIL:  return 0;
    case VAL_OBJ:  return obj_hash(AS_OBJ(value));
    case VAL_SSTR: return hash_string(AS_SSTR(value), VALUE_SSO_SIZE);
    default: return 0;
    }
#endif
}
