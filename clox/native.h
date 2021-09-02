#ifndef NATIVE_H_INCLUDED
#define NATIVE_H_INCLUDED

#include <stdbool.h>
#include "value.h"

typedef struct {
    Value value;
    bool error;
} NativeResult;

#define NATIVE_MKRES(v) ((NativeResult) { .value = v,         .error = false })
#define NATIVE_MKERR()  ((NativeResult) { .value = VALUE_MKNIL(), .error = true })

typedef NativeResult (*NativeFn)(int argc, Value *argv);

NativeResult native_clock(int argc, Value *argv);
NativeResult native_sqrt(int argc, Value *argv);

#endif
