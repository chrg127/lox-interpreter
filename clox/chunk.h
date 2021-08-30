#ifndef CHUNK_H_INCLUDED
#define CHUNK_H_INCLUDED

#include <stddef.h>
#include "uint.h"
#include "value.h"

typedef enum {
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    // OP_PUSH,
    OP_POP,
    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_EQ,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_NOT,
    OP_NEGATE,
    OP_PRINT,
    OP_BRANCH,
    OP_BRANCH_FALSE,
    OP_BRANCH_BACK,
    OP_CALL,
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
