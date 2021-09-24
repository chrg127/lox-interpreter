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
    OP_POP,
    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_GET_PROPERTY,
    OP_SET_PROPERTY,
    OP_GET_SUPER,
    OP_EQ,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_NOT,
    OP_NEGATE,
    OP_CREATE_ARRAY,    // with length
    OP_BUILD_ARRAY,     // with init-list
    OP_LOAD_SUBSCRIPT,
    OP_STORE_SUBSCRIPT,
    OP_PRINT,
    OP_BRANCH,
    OP_BRANCH_FALSE,
    OP_BRANCH_BACK,
    OP_CALL,
    OP_INVOKE,
    OP_SUPER_INVOKE,
    OP_RETURN,
    OP_CLOSURE,
    OP_CLOSE_UPVALUE,
    OP_CLASS,
    OP_METHOD,
    OP_STATIC,
    OP_INHERIT,
} Opcode;

typedef struct {
    VecU8 code;
    ValueArray constants;
    VecSizet lineinfo;
} Chunk;

void chunk_init(Chunk *chunk);
void chunk_write(Chunk *chunk, u8 byte, size_t line);
void chunk_free(Chunk *chunk);
size_t chunk_add_const(Chunk *chunk, Value value);
size_t chunk_get_line(Chunk *chunk, size_t offset);

#endif
