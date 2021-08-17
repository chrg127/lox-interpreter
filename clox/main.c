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

    chunk_write_const(&chunk, 1.2, 123);
    chunk_write_const(&chunk, 1.6, 123);
    chunk_write_const(&chunk, 1.11, 123);

    chunk_write(&chunk, OP_RETURN, 123);

    disassemble(&chunk, "test chunk");

    chunk_free(&chunk);
    return 0;
}
