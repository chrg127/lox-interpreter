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
        native_runtime_error("sqrt", "invalid parameter");
        return NATIVE_MKERR();
    }
    double param = AS_NUM(argv[0]);
    double res = sqrt(param);
    return NATIVE_MKRES(VALUE_MKNUM(res));
}

NativeResult native_tostr(int argc, Value *argv)
{
    return NATIVE_MKRES(VALUE_MKOBJ(value_tostring(argv[0])));
}
