#include "disassemble.h"

#include <stdio.h>
#include "value.h"
#include "object.h"

static int simple_instr(const char *name, size_t offset)
{
    printf("%s", name);
    return offset + 1;
}

static int const_instr(const char *name, Chunk *chunk, size_t offset)
{
    u8 index = chunk->code[offset + 1];
    printf("%s %d '", name, index);
    value_print(chunk->constants.values[index]);
    printf("'");
    return offset + 2;
}

static int const_long_instr(const char *name, Chunk *chunk, size_t offset)
{
    u8 b1 = chunk->code[offset + 1];
    u8 b2 = chunk->code[offset + 2];
    u16 index = b2 << 8 | b1;
    printf("%s %d %d (%d) '", name, b1, b2, index);
    value_print(chunk->constants.values[index]);
    printf("'");
    return offset + 3;
}

static size_t byte_instr(const char *name, Chunk *chunk, size_t offset)
{
    u8 slot = chunk->code[offset+1];
    printf("%s %d", name, slot);
    return offset + 2;
}

static size_t byte2_instr(const char *name, Chunk *chunk, size_t offset)
{
    u8 b1 = chunk->code[offset+1];
    u8 b2 = chunk->code[offset+2];
    u16 slot = b2 << 8 | b1;
    printf("%s %d %d (%d)", name, b1, b2, slot);
    return offset + 3;
}

static size_t byte3_instr(const char *name, Chunk *chunk, size_t offset)
{
    u8 b1 = chunk->code[offset+1];
    u8 b2 = chunk->code[offset+2];
    u8 b3 = chunk->code[offset+3];
    u32 slot = b3 << 16 | b2 << 8 | b1;
    printf("%s %d %d %d (%d)", name, b1, b2, b3, slot);
    return offset + 4;
}

static size_t jump_instr(const char *name, int sign, Chunk *chunk, size_t offset)
{
    u8 b1 = chunk->code[offset + 1];
    u8 b2 = chunk->code[offset + 2];
    u16 branch = b2 << 8 | b1;
    printf("%s %d %d (%ld -> %ld)", name, b1, b2, offset, offset + 3 + sign * branch);
    return offset + 3;
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
    size_t line = chunk_get_line(chunk, offset);
    if (offset != 0 && line == chunk_get_line(chunk, offset-1))
        printf("   | ");
    else
        printf("%04ld ", line);

    u8 instr = chunk->code[offset];
    switch (instr) {
    case OP_CONSTANT:       return const_instr("ldc", chunk, offset);
    case OP_CONSTANT_LONG:  return const_long_instr("ldc", chunk, offset);
    case OP_NEGATE:         return simple_instr("neg", offset);
    case OP_NIL:            return simple_instr("ldn", offset);
    case OP_TRUE:           return simple_instr("ldt", offset);
    case OP_FALSE:          return simple_instr("ldf", offset);
    case OP_POP:            return simple_instr("pop", offset);
    case OP_DEFINE_GLOBAL:  return const_long_instr("dfg", chunk, offset);
    case OP_GET_GLOBAL:     return const_long_instr("ldg", chunk, offset);
    case OP_SET_GLOBAL:     return const_long_instr("stg", chunk, offset);
    case OP_GET_LOCAL:      return byte2_instr("ldl", chunk, offset);
    case OP_SET_LOCAL:      return byte2_instr("stl", chunk, offset);
    case OP_GET_UPVALUE:    return byte2_instr("ldu", chunk, offset);
    case OP_SET_UPVALUE:    return byte2_instr("stu", chunk, offset);
    case OP_EQ:             return simple_instr("cme", offset);
    case OP_GREATER:        return simple_instr("cmg", offset);
    case OP_LESS:           return simple_instr("cml", offset);
    case OP_ADD:            return simple_instr("add", offset);
    case OP_SUB:            return simple_instr("sub", offset);
    case OP_MUL:            return simple_instr("mul", offset);
    case OP_DIV:            return simple_instr("div", offset);
    case OP_NOT:            return simple_instr("not", offset);
    case OP_PRINT:          return simple_instr("prt", offset);
    case OP_BRANCH:         return jump_instr("bfw",  1, chunk, offset);
    case OP_BRANCH_FALSE:   return jump_instr("bfl",  1, chunk, offset);
    case OP_BRANCH_BACK:    return jump_instr("bbw", -1, chunk, offset);
    case OP_CALL:           return byte_instr("cal", chunk, offset);
    case OP_RETURN:         return simple_instr("ret", offset);
    case OP_CLOSURE: {
        offset++;
        u8 b1 = chunk->code[offset++];
        u8 b2 = chunk->code[offset++];
        u16 constant = TOU16(b1, b2);
        printf("%s %d '", "clo", constant);
        value_print(chunk->constants.values[constant]);
        printf("'");

        ObjFunction *fun = AS_FUNCTION(chunk->constants.values[constant]);
        for (size_t j = 0; j < fun->upvalue_count; j++) {
            int is_local = chunk->code[offset++];
            u8 b1 = chunk->code[offset++];
            u8 b2 = chunk->code[offset++];
            u16 index    = TOU16(b1, b2);
            printf("\n%04ld:        | %s %d", offset - 2, is_local ? "local" : "upvalue", index);
        }
        return offset;
    }
    case OP_CLOSE_UPVALUE: return simple_instr("clu", offset);
    default:
        printf("[unknown] [%d]", instr);
        return offset + 1;
    }
}
