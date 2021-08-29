#ifndef COMPILER_H_INCLUDED
#define COMPILER_H_INCLUDED

#include <stdbool.h>
#include "chunk.h"
#include "object.h"

ObjFunction *compile(const char *src, const char *filename);

#endif
