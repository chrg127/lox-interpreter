#ifndef DISASSEMBLE_H_INCLUDED
#define DISASSEMBLE_H_INCLUDED

#include <stddef.h>
#include "chunk.h"

void disassemble(Chunk *chunk, const char *name);
size_t disassemble_opcode(Chunk *chunk, size_t offset);

#endif
