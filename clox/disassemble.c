#include "disassemble.h"

#include <stdio.h>
#include "value.h"

static int simple_instr(const char *name, size_t offset)
{
    printf("%s\n", name);
    return offset + 1;
}

static int const_instr(const char *name, Chunk *chunk, size_t offset)
{
    u8 index = chunk->code[offset + 1];
    printf("%-16s %4d '", name, index);
    value_print(chunk->constants.values[index]);
    printf("'\n");
    return offset + 2;
}

void disassemble(Chunk *chunk, const char *name)
{
    printf("== %s ==\n", name);

    for (size_t i = 0; i < chunk->size; )
        i = disassemble_opcode(chunk, i);
}

size_t disassemble_opcode(Chunk *chunk, size_t offset)
{
    printf("%04ld ", offset);
    if (offset != 0 && chunk->lines[offset] == chunk->lines[offset - 1])
        printf("    | ");
    else
        printf("%5d ", chunk->lines[offset]);

    u8 instr = chunk->code[offset];
    switch (instr) {
    case OP_CONSTANT: return const_instr("OP_CONSTANT", chunk, offset);
    case OP_NEGATE:   return simple_instr("OP_NEGATE", offset);
    case OP_ADD:      return simple_instr("OP_ADD", offset);
    case OP_SUB:      return simple_instr("OP_SUB", offset);
    case OP_MUL:      return simple_instr("OP_MUL", offset);
    case OP_DIV:      return simple_instr("OP_DIV", offset);
    case OP_RETURN:   return simple_instr("OP_RETURN", offset);
    default:
        printf("unknown opcode %d\n", instr);
        return offset + 1;
    }
}
