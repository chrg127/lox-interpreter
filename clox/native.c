#include "native.h"

#include <stdio.h>
#include <time.h>
#include <math.h>
#include "vm.h"
#include "object.h"
#include "memory.h"

NativeResult native_clock(int argc, Value *argv)
{
    return NATIVE_MKRES(VALUE_MKNUM((double)clock() / CLOCKS_PER_SEC));
}

NativeResult native_sqrt(int argc, Value *argv)
{
    if (!IS_NUM(argv[0])) {
        native_runtime_error("sqrt", "invalid parameter: not a number value");
        return NATIVE_MKERR();
    }
    double param = AS_NUM(argv[0]);
    double res = sqrt(param);
    return NATIVE_MKRES(VALUE_MKNUM(res));
}

NativeResult native_tostr(int argc, Value *argv)
{
    Value arg = argv[0];
    return NATIVE_MKRES(value_tostring(arg));
}

NativeResult native_typeof(int argc, Value *argv)
{
    return (IS_INSTANCE(argv[0])) ? NATIVE_MKRES(VALUE_MKOBJ(AS_INSTANCE(argv[0])->klass))
                                  : NATIVE_MKRES(VALUE_MKNIL());
}

NativeResult native_has_field(int argc, Value *argv)
{
    if (!IS_STRING(argv[1])) {
        native_runtime_error("has_field", "invalid parameter: not a string value");
        return NATIVE_MKERR();
    }
    if (!IS_INSTANCE(argv[0]))
        return NATIVE_MKRES(VALUE_MKBOOL(true));
    ObjInstance *inst = AS_INSTANCE(argv[0]);
    ObjString *name = AS_STRING(argv[1]);
    Value value;
    bool res = table_lookup(&inst->fields, name, &value);
    return NATIVE_MKRES(VALUE_MKBOOL(res));
}

NativeResult native_delete_field(int argc, Value *argv)
{
    if (!IS_INSTANCE(argv[0])) {
        native_runtime_error("del_field", "invalid parameter: not an instance value");
        return NATIVE_MKERR();
    }
    if (!IS_STRING(argv[1])) {
        native_runtime_error("del_field", "invalid parameter: not a string value");
        return NATIVE_MKERR();
    }
    ObjInstance *inst = AS_INSTANCE(argv[0]);
    ObjString *name = AS_STRING(argv[1]);
    table_delete(&inst->fields, name);
    return NATIVE_MKRES(VALUE_MKNIL());
}

NativeResult native_len(int argc, Value *argv)
{
    Value array = argv[0];
    if (IS_STRING(array)) return NATIVE_MKRES(VALUE_MKNUM((double)AS_STRING(array)->len));
    if (IS_SSTR(array))   return NATIVE_MKRES(VALUE_MKNUM((double)strlen(AS_SSTR(array))));
    if (IS_ARRAY(array))  return NATIVE_MKRES(VALUE_MKNUM((double)AS_ARRAY(array)->len));
    native_runtime_error("len", "invalid parameter: not an array or string value");
    return NATIVE_MKERR();
}

NativeResult native_strtoarr(int argc, Value *argv)
{
    if (!IS_STRING(argv[0]) || !IS_SSTR(argv[0])) {
        native_runtime_error("strtoarr", "invalid parameter: not a string value");
        return NATIVE_MKERR();
    }
    Value str = argv[0];
    size_t len = IS_STRING(str) ? AS_STRING(str)->len  : strlen(AS_SSTR(str));
    char *data = IS_STRING(str) ? AS_STRING(str)->data : AS_SSTR(str);
    Value *values = ALLOCATE(Value, len);
    for (size_t i = 0; i < len; i++)
        values[i] = VALUE_MKNUM((double)data[i]);
    return NATIVE_MKRES(VALUE_MKOBJ(obj_make_array(len, values)));
}

NativeResult native_arrtostr(int argc, Value *argv)
{
    if (!IS_ARRAY(argv[0])) {
        native_runtime_error("arrtostr", "invalid parameter: not an array value");
        return NATIVE_MKERR();
    }

    ObjArray *arr = AS_ARRAY(argv[0]);
    char *str = ALLOCATE(char, arr->len);
    for (size_t i = 0; i < arr->len; i++) {
        if (!IS_NUM(arr->data[i])) {
            native_runtime_error("arrtostr", "array value contains non-numeric values");
            FREE_ARRAY(char, str, arr->len);
        }
        str[i] = (char) AS_NUM(arr->data[i]);
    }

    if (arr->len < VALUE_SSO_SIZE) {
        Value strval = value_mksstr(str, arr->len);
        FREE_ARRAY(char, str, arr->len);
        return NATIVE_MKRES(strval);
    }
    return NATIVE_MKRES(VALUE_MKOBJ(obj_take_string(str, arr->len)));
}
