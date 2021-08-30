#ifndef COMPILER_H_INCLUDED
#define COMPILER_H_INCLUDED

#include <stdbool.h>
#include "chunk.h"
#include "object.h"

void compile_init();
void compile_free();
ObjFunction *compile(const char *src, const char *filename);

#endif
