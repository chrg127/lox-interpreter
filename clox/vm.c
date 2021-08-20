#include "vm.h"

#include <stdio.h>
#include "compiler.h"
#include "disassemble.h"

#define DEBUG_TRACE_EXECUTION

VM vm;

static void print_stack()
{
    printf("            stack: ");
    for (Value *p = vm.stack; p < vm.sp; p++) {
        printf("[");
        value_print(*p);
        printf("]");
    }
    printf("\n");
}

static VMResult run()
{
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(op) \
    do {                    \
        Value b = vm_pop(); \
        Value a = vm_pop(); \
        vm_push(a op b);    \
    } while (0)

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        disassemble_opcode(vm.chunk, (size_t)(vm.ip - vm.chunk->code));
        print_stack();
#endif
        u8 instr = READ_BYTE();
        switch (instr) {
        case OP_CONSTANT: {
            Value constant = READ_CONSTANT();
            vm_push(constant);
            break;
        }
        case OP_ADD:    BINARY_OP(+); break;
        case OP_SUB:    BINARY_OP(-); break;
        case OP_MUL:    BINARY_OP(*); break;
        case OP_DIV:    BINARY_OP(/); break;
        case OP_NEGATE:
            vm.sp[-1] = -vm.sp[-1];
            break;
        case OP_RETURN:
            value_print(vm_pop());
            printf("\n");
            return VM_OK;
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

static void reset()
{
    vm.sp = vm.stack;
}

void vm_init()
{
    reset();
}

void vm_free()
{

}

VMResult vm_interpret(const char *src)
{
    Chunk chunk;
    chunk_init(&chunk);
    if (!compile(src, &chunk)) {
        chunk_free(&chunk);
        return VM_COMPILE_ERROR;
    }
    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;
    printf("=== running VM ===\n");
    VMResult result = run();
    chunk_free(&chunk);
    return result;
}

void vm_push(Value value)
{
    *vm.sp++ = value;
}

Value vm_pop()
{
    return *--vm.sp;
}

VMResult vm_run_chunk(Chunk *chunk)
{
    vm.chunk = chunk;
    vm.ip = chunk->code;
    return run();
}
