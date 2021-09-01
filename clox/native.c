#include "native.h"

#include <time.h>
#include "vm.h"

NativeResult native_clock(int argc, Value *argv)
{
    NativeResult res;
    res.value = VALUE_MKNUM((double)clock() / CLOCKS_PER_SEC);
    res.error = false;
    return res;
}
