#include "vm.h"

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include "compiler.h"
#include "disassemble.h"
#include "object.h"
#include "memory.h"

VM vm;

void push(Value value)
{
    *vm.sp++ = value;
}

Value pop()
{
    return *--vm.sp;
}

static void print_stack()
{
    printf("stack: ");
    if (vm.stack == vm.sp)
        printf("(empty)\n");
    else {
        for (Value *p = vm.stack; p < vm.sp; p++) {
            printf("[");
            value_print(*p);
            printf("]");
        }
        printf("\n");
    }
}

static Value peek(size_t dist)
{
    return vm.sp[-1 - dist];
}

static bool is_falsey(Value value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concat()
{
    ObjString *b = AS_STRING(pop());
    ObjString *a = AS_STRING(pop());
    size_t len = a->len + b->len;
    char *data = ALLOCATE(char, len+1);
    memcpy(data,          a->data, a->len);
    memcpy(data + a->len, b->data, b->len);
    data[len] = '\0';
    ObjString *result = obj_take_string(data, len);
    push(VALUE_MKOBJ(result));
}

static void reset_stack()
{
    if (vm.stack != NULL)
        FREE_ARRAY(Value, vm.stack, STACK_MAX);
    vm.stack = ALLOCATE(Value, STACK_MAX);
    vm.sp = vm.stack;
    vm.frame_size = 0;
}

static void runtime_error(const char *fmt, ...)
{
    CallFrame *frame = &vm.frames[vm.frame_size - 1];
    size_t offset = frame->ip - frame->fun->chunk.code - 1;
    int line = chunk_get_line(&frame->fun->chunk, offset);
    fprintf(stderr, "%s:%d: runtime error: ", vm.filename, line);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputs("\n", stderr);

    fprintf(stderr, "traceback:\n");
    for (int i = vm.frame_size - 1; i >= 0; i--) {
        CallFrame *frame = &vm.frames[i];
        ObjFunction *fun = frame->fun;
        size_t offset = frame->ip - fun->chunk.code - 1;
        fprintf(stderr, "[line %ld] in ", chunk_get_line(&fun->chunk, offset));
        if (fun->name == NULL)
            fprintf(stderr, "script\n");
        else
            fprintf(stderr, "%s()\n", fun->name->data);
    }

    reset_stack();
}

static bool call(ObjFunction *fun, u8 argc)
{
    if (argc != fun->arity) {
        runtime_error("expected %d arguments, got %d", fun->arity, argc);
        return false;
    }
    if (vm.frame_size == FRAMES_MAX) {
        runtime_error("stack overflow");
        return false;
    }
    CallFrame *frame = &vm.frames[vm.frame_size++];
    frame->fun   = fun;
    frame->ip    = fun->chunk.code;
    frame->slots = vm.sp - argc - 1;
    return true;
}

static bool call_value(Value callee, u8 argc)
{
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
        case OBJ_FUNCTION:
            return call(AS_FUNCTION(callee), argc);
        case OBJ_NATIVE: {
            NativeFn native = AS_NATIVE(callee);
            Value result = native(argc, vm.sp - argc);
            vm.sp -= argc + 1;
            push(result);
            return true;
        }
        default:
            break;
        }
    }
    runtime_error("attempt to call non-callable object");
    return false;
}

static void define_native(const char *name, NativeFn fun)
{
    push(VALUE_MKOBJ(obj_copy_string(name, strlen(name))));
    push(VALUE_MKOBJ(obj_make_native(fun, name)));
    table_install(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

static Value clock_native(int argc, Value *argv)
{
    return VALUE_MKNUM((double)clock() / CLOCKS_PER_SEC);
}

static VMResult run()
{
    CallFrame *frame = &vm.frames[vm.frame_size - 1];
#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() (frame->ip += 2, (u16)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->fun->chunk.constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(value_type, op) \
    do {                    \
        if (!IS_NUM(peek(0)) || !IS_NUM(peek(1))) { \
            runtime_error("operands must be numbers"); \
            return VM_RUNTIME_ERROR; \
        } \
        double b = AS_NUM(pop()); \
        double a = AS_NUM(pop()); \
        push(value_type(a op b));    \
    } while (0)

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        print_stack();
        disassemble_opcode(&frame->fun->chunk, (size_t)(frame->ip - frame->fun->chunk.code));
        printf("\n");
#endif
        u8 instr = READ_BYTE();
        switch (instr) {
        case OP_CONSTANT: {
            Value constant = READ_CONSTANT();
            push(constant);
            break;
        }
        case OP_NIL:    push(VALUE_MKNIL());       break;
        case OP_TRUE:   push(VALUE_MKBOOL(true));  break;
        case OP_FALSE:  push(VALUE_MKBOOL(false));     break;
        // case OP_PUSH:
        case OP_POP:    pop(); break;
        case OP_DEFINE_GLOBAL: {
            ObjString *name = READ_STRING();
            table_install(&vm.globals, name, peek(0));
            pop();
            break;
        }
        case OP_GET_GLOBAL: {
            ObjString *name = READ_STRING();
            Value value;
            if (!table_lookup(&vm.globals, name, &value)) {
                runtime_error("undefined variable '%s'", name->data);
                return VM_RUNTIME_ERROR;
            }
            push(value);
            break;
        }
        case OP_SET_GLOBAL: {
            ObjString *name = READ_STRING();
            if (table_install(&vm.globals, name, peek(0))) {
                table_delete(&vm.globals, name);
                runtime_error("undefined variable '%s'", name->data);
                return VM_RUNTIME_ERROR;
            }
            break;
        }
        case OP_GET_LOCAL: {
            u8 b1 = READ_BYTE();
            u8 b2 = READ_BYTE();
            u8 b3 = READ_BYTE();
            u32 slot = b3 << 16 | b2 << 8 | b1;
            push(frame->slots[slot]);
            break;
        }
        case OP_SET_LOCAL: {
            u8 b1 = READ_BYTE();
            u8 b2 = READ_BYTE();
            u8 b3 = READ_BYTE();
            u32 slot = b3 << 16 | b2 << 8 | b1;
            frame->slots[slot] = peek(0);
            break;
        }
        case OP_EQ: {
            Value b = pop();
            Value a = pop();
            push(VALUE_MKBOOL(value_equal(a, b)));
            break;
        }
        case OP_GREATER: BINARY_OP(VALUE_MKBOOL, >); break;
        case OP_LESS:    BINARY_OP(VALUE_MKBOOL, <); break;
        case OP_ADD:
            if (IS_STRING(peek(0)) && IS_STRING(peek(1)))
                concat();
            else if (IS_NUM(peek(0)) && IS_NUM(peek(1))) {
                double b = AS_NUM(pop());
                double a = AS_NUM(pop());
                push(VALUE_MKNUM(a + b));
            } else {
                runtime_error("operands must be two numbers or two strings");
                return VM_RUNTIME_ERROR;
            }
            break;
        case OP_SUB:    BINARY_OP(VALUE_MKNUM, -); break;
        case OP_MUL:    BINARY_OP(VALUE_MKNUM, *); break;
        case OP_DIV:    BINARY_OP(VALUE_MKNUM, /); break;
        case OP_NOT:
            push(VALUE_MKBOOL(is_falsey(pop())));
            break;
        case OP_NEGATE:
            if (!IS_NUM(peek(0))) {
                runtime_error("operand must be a number");
                return VM_RUNTIME_ERROR;
            }
            // vm.sp[-1] = VALUE_MKNUM(-AS_NUM(vm.sp[-1]));
            push(VALUE_MKNUM(-AS_NUM(pop())));
            break;
        case OP_PRINT:
            value_print(pop());
            printf("\n");
            break;
        case OP_BRANCH: {
            u16 offset = READ_SHORT();
            frame->ip += offset;
            break;
        }
        case OP_BRANCH_FALSE: {
            u16 offset = READ_SHORT();
            if (is_falsey(peek(0)))
                frame->ip += offset;
            break;
        }
        case OP_BRANCH_BACK: {
            u16 offset = READ_SHORT();
            frame->ip -= offset;
            break;
        }
        case OP_CALL: {
            u8 argc = READ_BYTE();
            if (!call_value(peek(argc), argc))
                return VM_RUNTIME_ERROR;
            frame = &vm.frames[vm.frame_size - 1];
            break;
        }
        case OP_RETURN: {
            Value result = pop();
            vm.frame_size--;
            if (vm.frame_size == 0) {
                pop();
                return VM_OK;
            }
            vm.sp = frame->slots;
            push(result);
            frame = &vm.frames[vm.frame_size-1];
            break;
        }
        default:
            runtime_error("unknown opcode: %d", instr);
            return VM_RUNTIME_ERROR;
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

void vm_init()
{
    reset_stack();
    vm.objects = NULL;
    table_init(&vm.globals);
    table_init(&vm.strings);
    define_native("clock", clock_native);
}

void vm_free()
{
    table_free(&vm.globals);
    table_free(&vm.strings);
    obj_free_arr(vm.objects);
}

VMResult vm_interpret(const char *src, const char *filename)
{
    ObjFunction *fun = compile(src, filename);
    if (!fun)
        return VM_COMPILE_ERROR;
    push(VALUE_MKOBJ(fun));
    call(fun, 0);
    vm.filename = filename;

#ifdef DEBUG_TRACE_EXECUTION
    printf("=== running VM ===\n");
#endif

    return run();
}
