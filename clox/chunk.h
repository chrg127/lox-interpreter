#ifndef CHUNK_H_INCLUDED
#define CHUNK_H_INCLUDED

#include <stddef.h>
#include "uint.h"
#include "value.h"

typedef enum {
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_RETURN,
} Opcode;

typedef struct {
    u8 *code;
    size_t size;
    size_t cap;
    ValueArray constants;
    int *lines;
} Chunk;

void chunk_init(Chunk *chunk);
void chunk_write(Chunk *chunk, u8 byte, int line);
void chunk_free(Chunk *chunk);
int chunk_add_const(Chunk *chunk, Value value);

#endif
