#include "native.h"

#include <stdio.h>
#include <time.h>
#include <math.h>
#include "vm.h"
#include "object.h"

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
    return NATIVE_MKRES(value_tostring(argv[0]));
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

NativeResult native_del_field(int argc, Value *argv)
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
