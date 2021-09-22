#include "chunk.h"

#include "memory.h"
#include "vector.h"
#include "value.h"
#include "uint.h"
#include "vm.h"

void chunk_init(Chunk *chunk)
{
    vec_u8_init(&chunk->code);
    valuearray_init(&chunk->constants);
    vec_size_t_init(&chunk->lineinfo);
}

static void write_line(VecSizet *info, size_t line)
{
    if (info->cap != 0 && info->data[info->size - 2] == line) {
        info->data[info->size - 1]++;
        return;
    }
    vec_size_t_write(info, line);
    vec_size_t_write(info, 1);
}

void chunk_write(Chunk *chunk, u8 byte, size_t line)
{
    vec_u8_write(&chunk->code, byte);
    write_line(&chunk->lineinfo, line);
}

void chunk_free(Chunk *chunk)
{
    vec_u8_free(&chunk->code);
    valuearray_free(&chunk->constants);
    vec_size_t_free(&chunk->lineinfo);
    chunk_init(chunk);
}

size_t chunk_add_const(Chunk *chunk, Value value)
{
    for (size_t i = 0; i < chunk->constants.size; i++) {
        if (value_equal(chunk->constants.values[i], value))
            return i;
    }
    vm_push(value);
    valuearray_write(&chunk->constants, value);
    vm_pop();
    return chunk->constants.size - 1;
}

size_t chunk_get_line(Chunk *chunk, size_t offset)
{
    if (chunk->lineinfo.size == 0)
        return 0;
    size_t j = 1, i = chunk->lineinfo.data[j];
    while (i <= offset) {
        j += 2;
        i += chunk->lineinfo.data[j];
    }
    return chunk->lineinfo.data[j-1];
}
