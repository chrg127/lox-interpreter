#include "chunk.h"

#include "memory.h"
#include "vector.h"
#include "value.h"

void chunk_init(Chunk *chunk)
{
    VECTOR_INIT(chunk, code);
    valuearray_init(&chunk->constants);
    chunk->lines = NULL;
}

void chunk_write(Chunk *chunk, u8 byte, int line)
{
    if (chunk->cap < chunk->size + 1) {
        size_t old = chunk->cap;
        chunk->cap = vector_grow_cap(old);
        chunk->code = GROW_ARRAY(u8, chunk->code, old, chunk->cap);
        chunk->lines = GROW_ARRAY(int, chunk->lines, old, chunk->cap);
    }
    chunk->code[chunk->size] = byte;
    chunk->lines[chunk->size] = line;
    chunk->size++;
}

void chunk_free(Chunk *chunk)
{
    FREE_ARRAY(u8, chunk->code, chunk->cap);
    FREE_ARRAY(int, chunk->lines, chunk->cap);
    valuearray_free(&chunk->constants);
    chunk_init(chunk);
}

int chunk_add_const(Chunk *chunk, Value value)
{
    valuearray_write(&chunk->constants, value);
    return chunk->constants.size - 1;
}

void chunk_write_const(Chunk *chunk, Value value, int line)
{
    int i = chunk_add_const(chunk, value);
    chunk_write(chunk, i >= 0 ? OP_CONSTANT_LONG : OP_CONSTANT, line);
    chunk_write(chunk, i & 0xFF, line);
    if (i >= 0) {
        chunk_write(chunk, (i >>  8) & 0xFF, line);
        chunk_write(chunk, (i >> 16) & 0xFF, line);
    }
}
