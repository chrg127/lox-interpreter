#ifndef NATIVE_H_INCLUDED
#define NATIVE_H_INCLUDED

#include <stdbool.h>
#include "value.h"

typedef struct {
    Value value;
    bool error;
} NativeResult;

typedef NativeResult (*NativeFn)(int argc, Value *argv);

NativeResult native_clock(int argc, Value *argv);

#endif
