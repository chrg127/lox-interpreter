#ifndef COMPILER_H_INCLUDED
#define COMPILER_H_INCLUDED

#include <stdbool.h>
#include "chunk.h"

void compile_init();
void compile_free();
bool compile(const char *src, Chunk *chunk, const char *filename);

#endif
