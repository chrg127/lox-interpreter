#ifndef COMPILER_H_INCLUDED
#define COMPILER_H_INCLUDED

#include <stdbool.h>
#include "chunk.h"

bool compile(const char *src, Chunk *chunk, const char *filename);

#endif
