#ifndef CHUNK_H_INCLUDED
#define CHUNK_H_INCLUDED

#include <stddef.h>
#include "uint.h"
#include "value.h"

typedef enum {
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_NEGATE,
    OP_RETURN,
} Opcode;

typedef struct {
    size_t *lines;
    size_t size;
    size_t cap;
} LineInfo;

typedef struct {
    u8 *code;
    size_t size;
    size_t cap;
    ValueArray constants;
    LineInfo info;
} Chunk;

void chunk_init(Chunk *chunk);
void chunk_write(Chunk *chunk, u8 byte, size_t line);
void chunk_free(Chunk *chunk);
size_t chunk_add_const(Chunk *chunk, Value value);
void chunk_write_const(Chunk *chunk, Value value, size_t line);
size_t chunk_get_line(Chunk *chunk, size_t offset);

#endif
