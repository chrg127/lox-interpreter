#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "chunk.h"
#include "disassemble.h"

int main(int argc, char *argv[])
{
    Chunk chunk;
    chunk_init(&chunk);

    int i = chunk_add_const(&chunk, 1.2, 123);
    chunk_write(&chunk, OP_CONSTANT, 123);
    chunk_write(&chunk, i, 123);

    chunk_write(&chunk, OP_RETURN, 123);

    disassemble(&chunk, "test chunk");

    chunk_free(&chunk);
    return 0;
}
