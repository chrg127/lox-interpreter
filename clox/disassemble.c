#include "disassemble.h"

#include <stdio.h>
#include "value.h"
#include "object.h"

static size_t simple_instr(const char *name, size_t offset)
{
    printf("%s", name);
    return offset + 1;
}

static size_t const_instr(const char *name, Chunk *chunk, size_t offset)
{
    u8 index = chunk->code[offset + 1];
    printf("%s %03d '", name, index);
    value_print(chunk->constants.values[index]);
    printf("'");
    return offset + 2;
}

static size_t byte_instr(const char *name, Chunk *chunk, size_t offset)
{
    u8 slot = chunk->code[offset+1];
    printf("%s %03d", name, slot);
    return offset + 2;
}

static size_t jump_instr(const char *name, int sign, Chunk *chunk, size_t offset)
{
    u16 branch = TOU16(chunk->code[offset + 1], chunk->code[offset + 2]);
    printf("%s %ld -> %ld", name, offset, offset + 3 + sign * branch);
    return offset + 3;
}

static size_t closure_instr(const char *name, Chunk *chunk, size_t offset)
{
    offset++;
    u8 constant = chunk->code[offset++];
    printf("%s %03d '", "clo", constant);
    value_print(chunk->constants.values[constant]);
    printf("'");

    ObjFunction *fun = AS_FUNCTION(chunk->constants.values[constant]);
    for (int j = 0; j < fun->upvalue_count; j++) {
        int is_local = chunk->code[offset++];
        int index    = chunk->code[offset++];
        printf("\n%04ld:       | %s %03d", offset - 2, is_local ? "local" : "upvalue", index);
    }

    return offset;
}

static size_t invoke_instr(const char *name, Chunk *chunk, size_t offset)
{
    u8 constant = chunk->code[offset + 1];
    u8 argc     = chunk->code[offset + 2];
    printf("%s (%03d args) %03d '", name, argc, constant);
    value_print(chunk->constants.values[constant]);
    printf("'");
    return offset + 3;
}

void disassemble(Chunk *chunk, const char *name)
{
    printf("=== %s ===\n", name);
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
    case OP_CONSTANT:       return const_instr("ldc", chunk, offset);
    case OP_NEGATE:         return simple_instr("neg", offset);
    case OP_NIL:            return simple_instr("ldn", offset);
    case OP_TRUE:           return simple_instr("ldt", offset);
    case OP_FALSE:          return simple_instr("ldf", offset);
    case OP_POP:            return simple_instr("pop", offset);
    case OP_DEFINE_GLOBAL:  return const_instr("dfg", chunk, offset);
    case OP_GET_GLOBAL:     return const_instr("ldg", chunk, offset);
    case OP_SET_GLOBAL:     return const_instr("stg", chunk, offset);
    case OP_GET_LOCAL:      return byte_instr("ldl", chunk, offset);
    case OP_SET_LOCAL:      return byte_instr("stl", chunk, offset);
    case OP_GET_UPVALUE:    return byte_instr("ldu", chunk, offset);
    case OP_SET_UPVALUE:    return byte_instr("stu", chunk, offset);
    case OP_GET_PROPERTY:   return const_instr("ldp", chunk, offset);
    case OP_SET_PROPERTY:   return const_instr("stp", chunk, offset);
    case OP_GET_SUPER:      return const_instr("lds", chunk, offset);
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
    case OP_INVOKE:         return invoke_instr("ivk", chunk, offset);
    case OP_SUPER_INVOKE:   return invoke_instr("svk", chunk, offset);
    case OP_RETURN:         return simple_instr("ret", offset);
    case OP_CLOSURE:        return closure_instr("clo", chunk, offset);
    case OP_CLOSE_UPVALUE:  return simple_instr("clu", offset);
    case OP_CLASS:          return const_instr("dfc", chunk, offset);
    case OP_METHOD:         return const_instr("dfm", chunk, offset);
    default:
        printf("[unknown] [%d]", instr);
        return offset + 1;
    }
}
