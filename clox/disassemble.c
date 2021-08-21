#include "disassemble.h"

#include <stdio.h>
#include "value.h"

static int simple_instr(const char *name, size_t offset)
{
    printf("%s", name);
    return offset + 1;
}

static int const_instr(const char *name, Chunk *chunk, size_t offset)
{
    u8 index = chunk->code[offset + 1];
    printf("%s #%d '", name, index);
    value_print(chunk->constants.values[index]);
    printf("'");
    return offset + 2;
}

void disassemble(Chunk *chunk, const char *name)
{
    printf("== %s ==\n", name);

    for (size_t i = 0; i < chunk->size; ) {
        i = disassemble_opcode(chunk, i);
        printf("\n");
    }
}

size_t disassemble_opcode(Chunk *chunk, size_t offset)
{
    printf("%04ld: ", offset);
    if (offset != 0 && chunk->lines[offset] == chunk->lines[offset - 1])
        printf("   | ");
    else
        printf("%04d ", chunk->lines[offset]);

    u8 instr = chunk->code[offset];
    switch (instr) {
    case OP_CONSTANT: return const_instr("load", chunk, offset);
    case OP_NEGATE:   return simple_instr("neg", offset);
    case OP_NIL:      return simple_instr("ldnil", offset);
    case OP_TRUE:     return simple_instr("ldtrue", offset);
    case OP_FALSE:    return simple_instr("ldfalse", offset);
    case OP_EQ:       return simple_instr("equal", offset);
    case OP_GREATER:  return simple_instr("greater", offset);
    case OP_LESS:     return simple_instr("less", offset);
    case OP_ADD:      return simple_instr("add", offset);
    case OP_SUB:      return simple_instr("sub", offset);
    case OP_MUL:      return simple_instr("mul", offset);
    case OP_DIV:      return simple_instr("div", offset);
    case OP_NOT:      return simple_instr("not", offset);
    case OP_RETURN:   return simple_instr("ret", offset);
    default:
        printf("[unknown] [%d]", instr);
        return offset + 1;
    }
}
