#include "chunk.h"

#include "memory.h"
#include "vector.h"
#include "value.h"

static void lineinfo_init(LineInfo *info)
{
    VECTOR_INIT(info, lines);
}

static void lineinfo_free(LineInfo *info)
{
    FREE_ARRAY(size_t, info->lines, info->cap);
    lineinfo_init(info);
}

void chunk_init(Chunk *chunk)
{
    VECTOR_INIT(chunk, code);
    valuearray_init(&chunk->constants);
    lineinfo_init(&chunk->info);
}

static void write_line(LineInfo *info, size_t line)
{
    if (info->cap != 0 && info->lines[info->size - 2] == line) {
        info->lines[info->size - 1]++;
        return;
    }
    // grow info array
    if (info->cap < info->size + 2) {
        size_t old = info->cap;
        info->cap = vector_grow_cap(old);
        info->lines = GROW_ARRAY(size_t, info->lines, old, info->cap);
    }
    info->lines[info->size++] = line;
    info->lines[info->size++] = 1;
}

void chunk_write(Chunk *chunk, u8 byte, size_t line)
{
    if (chunk->cap < chunk->size + 1) {
        size_t old = chunk->cap;
        chunk->cap = vector_grow_cap(old);
        chunk->code = GROW_ARRAY(u8, chunk->code, old, chunk->cap);
    }
    chunk->code[chunk->size++] = byte;
    write_line(&chunk->info, line);
}

void chunk_free(Chunk *chunk)
{
    FREE_ARRAY(u8, chunk->code, chunk->cap);
    valuearray_free(&chunk->constants);
    lineinfo_free(&chunk->info);
    chunk_init(chunk);
}

size_t chunk_add_const(Chunk *chunk, Value value)
{
    for (size_t i = 0; i < chunk->constants.size; i++) {
        if (value_equal(chunk->constants.values[i], value))
            return i;
    }
    valuearray_write(&chunk->constants, value);
    return chunk->constants.size - 1;
}

void chunk_write_const(Chunk *chunk, Value value, size_t line)
{
    size_t i = chunk_add_const(chunk, value);
    chunk_write(chunk, i >= 256 ? OP_CONSTANT_LONG : OP_CONSTANT, line);
    chunk_write(chunk, i & 0xFF, line);
    if (i >= 256) {
        chunk_write(chunk, (i >>  8) & 0xFF, line);
        chunk_write(chunk, (i >> 16) & 0xFF, line);
    }
}

size_t chunk_get_line(Chunk *chunk, size_t offset)
{
    if (chunk->info.size == 0)
        return 0;
    size_t j = 1, i = chunk->info.lines[j];
    while (i <= offset) {
        j += 2;
        i += chunk->info.lines[j];
    }
    return chunk->info.lines[j-1];
}
