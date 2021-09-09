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

size_t chunk_add_const(Chunk *chunk, Value value)
{
    valuearray_write(&chunk->constants, value);
    return chunk->constants.size - 1;
}
