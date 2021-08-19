#include "vm.h"

#include <stdio.h>
#include "compiler.h"
#include "disassemble.h"

#define DEBUG_TRACE_EXECUTION

VM vm;

static void print_stack()
{
    printf("stack: ");
    for (Value *p = vm.stack; p < vm.sp; p++) {
        printf("[ ");
        value_print(*p);
        printf(" ]");
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
        print_stack();
        disassemble_opcode(vm.chunk, (size_t)(vm.ip - vm.chunk->code));
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
        case OP_NEGATE: vm_push(-vm_pop()); break;
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
    compile(src);
    return VM_OK;
}

void vm_push(Value value)
{
    *vm.sp++ = value;
}

Value vm_pop()
{
    return *--vm.sp;
}

