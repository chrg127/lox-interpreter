#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "chunk.h"
#include "disassemble.h"
#include "vm.h"

int main(int argc, char *argv[])
{
    vm_init();

    Chunk chunk;
    chunk_init(&chunk);

    int i;
    i = chunk_add_const(&chunk, 1.2);
    chunk_write(&chunk, OP_CONSTANT, 123);
    chunk_write(&chunk, i, 123);
    i = chunk_add_const(&chunk, 3.4);
    chunk_write(&chunk, OP_CONSTANT, 123);
    chunk_write(&chunk, i, 123);

    chunk_write(&chunk, OP_ADD, 123);

    i = chunk_add_const(&chunk, 5.6);
    chunk_write(&chunk, OP_CONSTANT, 123);
    chunk_write(&chunk, i, 123);

    chunk_write(&chunk, OP_DIV, 123);

    chunk_write(&chunk, OP_NEGATE, 123);
    chunk_write(&chunk, OP_RETURN, 123);

    disassemble(&chunk, "test chunk");
    vm_interpret(&chunk);

    chunk_free(&chunk);
    vm_free();

    return 0;
}
