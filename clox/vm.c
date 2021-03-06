#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include "compiler.h"
#include "disassemble.h"
#include "object.h"
#include "memory.h"
#include "debug.h"

VM vm;

void vm_push(Value value)
{
    *vm.sp++ = value;
}

Value vm_pop()
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
    ObjString *b = AS_STRING(peek(0));
    ObjString *a = AS_STRING(peek(1));
    size_t len = a->len + b->len;
    char *data = ALLOCATE(char, len+1);
    memcpy(data,          a->data, a->len);
    memcpy(data + a->len, b->data, b->len);
    data[len] = '\0';
    ObjString *result = obj_take_string(data, len);
    vm_pop();
    vm_pop();
    vm_push(VALUE_MKOBJ(result));
}

static void reset_stack()
{
    vm.sp = vm.stack;
    vm.frame_size = 0;
    vm.open_upvalues = NULL;
}

static void runtime_error(const char *fmt, ...)
{
    CallFrame *frame = &vm.frames[vm.frame_size - 1];
    size_t offset = frame->ip - frame->closure->fun->chunk.code - 1;
    int line = frame->closure->fun->chunk.lines[offset];
    fprintf(stderr, "%s:%d: runtime error: ", vm.filename, line);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputs("\n", stderr);

    fprintf(stderr, "traceback:\n");
    for (int i = vm.frame_size - 1; i >= 0; i--) {
        CallFrame *frame = &vm.frames[i];
        ObjFunction *fun = frame->closure->fun;
        size_t offset = frame->ip - fun->chunk.code - 1;
        fprintf(stderr, "[line %d] in ", fun->chunk.lines[offset]);
        if (fun->name == NULL)
            fprintf(stderr, "script\n");
        else
            fprintf(stderr, "%s()\n", fun->name->data);
    }

    reset_stack();
}

static bool call(ObjClosure *closure, u8 argc)
{
    if (argc != closure->fun->arity) {
        runtime_error("expected %d arguments, got %d",
            closure->fun->arity, argc);
        return false;
    }
    if (vm.frame_size == FRAMES_MAX) {
        runtime_error("stack overflow");
        return false;
    }
    CallFrame *frame = &vm.frames[vm.frame_size++];
    frame->closure = closure;
    frame->ip    = closure->fun->chunk.code;
    frame->slots = vm.sp - argc - 1;
    return true;
}

static bool call_value(Value callee, u8 argc)
{
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
        // case OBJ_FUNCTION:
        //     return call(AS_FUNCTION(callee), argc);
        case OBJ_NATIVE: {
            NativeFn native = AS_NATIVE(callee);
            Value result = native(argc, vm.sp - argc);
            vm.sp -= argc + 1;
            vm_push(result);
            return true;
        case OBJ_CLOSURE:
            return call(AS_CLOSURE(callee), argc);
        case OBJ_CLASS: {
            ObjClass *klass = AS_CLASS(callee);
            vm.sp[-argc-1] = VALUE_MKOBJ(obj_make_instance(klass));
            Value ctor;
            if (table_lookup(&klass->methods, vm.init_string, &ctor))
                return call(AS_CLOSURE(ctor), argc);
            else if (argc != 0) {
                runtime_error("expected 0 arguments, got %d", argc);
                return false;
            }
            return true;
        }
        case OBJ_BOUND_METHOD: {
            ObjBoundMethod *bound = AS_BOUND_METHOD(callee);
            vm.sp[-argc-1] = bound->receiver;
            return call(bound->method, argc);
        }
        }
        default:
            break;
        }
    }
    runtime_error("attempt to call non-callable object");
    return false;
}

static bool invoke_from_class(ObjClass *klass, ObjString *name, u8 argc)
{
    Value method;
    if (!table_lookup(&klass->methods, name, &method)) {
        runtime_error("undefined property '%s'", name->data);
        return false;
    }
    return call(AS_CLOSURE(method), argc);
}

static bool invoke(ObjString *name, u8 argc)
{
    Value receiver = peek(argc);
    if (!IS_INSTANCE(receiver)) {
        runtime_error("can't call a method on a non-instance value");
        return false;
    }

    ObjInstance *inst = AS_INSTANCE(receiver);
    Value value;
    if (table_lookup(&inst->fields, name, &value)) {
        vm.sp[-argc-1] = value;
        return call_value(value, argc);
    }
    return invoke_from_class(inst->klass, name, argc);
}

static ObjUpvalue *capture_upvalue(Value *local)
{
    ObjUpvalue *prev = NULL;
    ObjUpvalue *entry = vm.open_upvalues;
    while (entry != NULL && entry->location > local) {
        prev = entry;
        entry = entry->next;
    }
    if (entry != NULL && entry->location == local)
        return entry;
    ObjUpvalue *created = obj_make_upvalue(local);
    created->next = entry;
    if (prev == NULL)
        vm.open_upvalues = created;
    else
        prev->next = created;
    return created;
}

static void close_upvalues(Value *last)
{
    while (vm.open_upvalues != NULL && vm.open_upvalues->location >= last) {
        ObjUpvalue *upvalue = vm.open_upvalues;
        upvalue->closed   = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.open_upvalues  = upvalue->next;
    }
}

static void define_method(ObjString *name)
{
    Value method = peek(0);
    ObjClass *klass = AS_CLASS(peek(1));
    table_install(&klass->methods, name, method);
    vm_pop();
}

static bool bind_method(ObjClass *klass, ObjString *name)
{
    Value method;
    if (!table_lookup(&klass->methods, name, &method))
        return false;
    ObjBoundMethod *bound = obj_make_bound_method(peek(0), AS_CLOSURE(method));
    vm_pop();
    vm_push(VALUE_MKOBJ(bound));
    return true;
}

static void define_native(const char *name, NativeFn fun)
{
    vm_push(VALUE_MKOBJ(obj_copy_string(name, strlen(name))));
    vm_push(VALUE_MKOBJ(obj_make_native(fun, name)));
    table_install(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    vm_pop();
    vm_pop();
}

static Value clock_native(int argc, Value *argv)
{
    return VALUE_MKNUM((double)clock() / CLOCKS_PER_SEC);
}

static VMResult run()
{
    CallFrame *frame = &vm.frames[vm.frame_size - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() \
    (frame->ip += 2, (u16)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() \
    (frame->closure->fun->chunk.constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())

#define BINARY_OP(value_type, op)                       \
    do {                                                \
        if (!IS_NUM(peek(0)) || !IS_NUM(peek(1))) {     \
            runtime_error("operands must be numbers");  \
            return VM_RUNTIME_ERROR;                    \
        }                                               \
        double b = AS_NUM(vm_pop());                       \
        double a = AS_NUM(vm_pop());                       \
        vm_push(value_type(a op b));                       \
    } while (0)

    for (;;) {

#ifdef DEBUG_TRACE_EXECUTION
        print_stack();
        disassemble_opcode(
            &frame->closure->fun->chunk,
            (size_t)(frame->ip - frame->closure->fun->chunk.code)
        );
        printf("\n");
#endif

        u8 instr = READ_BYTE();
        switch (instr) {
        case OP_CONSTANT: {
            Value constant = READ_CONSTANT();
            vm_push(constant);
            break;
        }
        case OP_NIL:    vm_push(VALUE_MKNIL());       break;
        case OP_TRUE:   vm_push(VALUE_MKBOOL(true));  break;
        case OP_FALSE:  vm_push(VALUE_MKBOOL(false)); break;
        case OP_POP:    vm_pop();                     break;
        case OP_DEFINE_GLOBAL: {
            ObjString *name = READ_STRING();
            table_install(&vm.globals, name, peek(0));
            vm_pop();
            break;
        }
        case OP_GET_GLOBAL: {
            ObjString *name = READ_STRING();
            Value value;
            if (!table_lookup(&vm.globals, name, &value)) {
                runtime_error("undefined variable '%s'", name->data);
                return VM_RUNTIME_ERROR;
            }
            vm_push(value);
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
            u8 slot = READ_BYTE();
            vm_push(frame->slots[slot]);
            break;
        }
        case OP_SET_LOCAL: {
            u8 slot = READ_BYTE();
            frame->slots[slot] = peek(0);
            break;
        }
        case OP_GET_UPVALUE: {
            u8 slot = READ_BYTE();
            vm_push(*frame->closure->upvalues[slot]->location);
            break;
        }
        case OP_SET_UPVALUE: {
            u8 slot = READ_BYTE();
            *frame->closure->upvalues[slot]->location = peek(0);
            break;
        }
        case OP_GET_PROPERTY: {
            if (!IS_INSTANCE(peek(0))) {
                runtime_error("attempt to get a property from a non-instance value");
                return VM_RUNTIME_ERROR;
            }

            ObjInstance *inst = AS_INSTANCE(peek(0));
            ObjString *name = READ_STRING();
            Value value;
            // field?
            if (table_lookup(&inst->fields, name, &value)) {
                vm_pop();
                vm_push(value);
                break;
            }
            // method?
            if (bind_method(inst->klass, name))
                break;

            runtime_error("undefined property '%s'", name->data);
            return VM_RUNTIME_ERROR;
        }
        case OP_SET_PROPERTY: {
            if (!IS_INSTANCE(peek(1))) {
                runtime_error("attempt to get a property from a non-instance value");
                return VM_RUNTIME_ERROR;
            }
            ObjInstance *inst = AS_INSTANCE(peek(1));
            table_install(&inst->fields, READ_STRING(), peek(0));
            Value value = vm_pop();
            vm_pop();
            vm_push(value);
            break;
        }
        case OP_GET_SUPER: {
            ObjString *name = READ_STRING();
            ObjClass *superclass = AS_CLASS(vm_pop());
            if (!bind_method(superclass, name))
                return VM_RUNTIME_ERROR;
            break;
        }
        case OP_EQ: {
            Value b = vm_pop();
            Value a = vm_pop();
            vm_push(VALUE_MKBOOL(value_equal(a, b)));
            break;
        }
        case OP_GREATER: BINARY_OP(VALUE_MKBOOL, >); break;
        case OP_LESS:    BINARY_OP(VALUE_MKBOOL, <); break;
        case OP_ADD:
            if (IS_STRING(peek(0)) && IS_STRING(peek(1)))
                concat();
            else if (IS_NUM(peek(0)) && IS_NUM(peek(1))) {
                double b = AS_NUM(vm_pop());
                double a = AS_NUM(vm_pop());
                vm_push(VALUE_MKNUM(a + b));
            } else {
                runtime_error("operands must be two numbers or two strings");
                return VM_RUNTIME_ERROR;
            }
            break;
        case OP_SUB:    BINARY_OP(VALUE_MKNUM, -); break;
        case OP_MUL:    BINARY_OP(VALUE_MKNUM, *); break;
        case OP_DIV:    BINARY_OP(VALUE_MKNUM, /); break;
        case OP_NOT:
            vm_push(VALUE_MKBOOL(is_falsey(vm_pop())));
            break;
        case OP_NEGATE:
            if (!IS_NUM(peek(0))) {
                runtime_error("operand must be a number");
                return VM_RUNTIME_ERROR;
            }
            vm_push(VALUE_MKNUM(-AS_NUM(vm_pop())));
            break;
        case OP_PRINT:
            value_print(vm_pop());
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
        case OP_INVOKE: {
            ObjString *method = READ_STRING();
            u8 argc = READ_BYTE();
            if (!invoke(method, argc))
                return VM_RUNTIME_ERROR;
            frame = &vm.frames[vm.frame_size-1];
            break;
        }
        case OP_SUPER_INVOKE: {
            ObjString *method = READ_STRING();
            u8 argc = READ_BYTE();
            ObjClass *superclass = AS_CLASS(vm_pop());
            if (!invoke_from_class(superclass, method, argc))
                return VM_RUNTIME_ERROR;
            frame = &vm.frames[vm.frame_size-1];
            break;
        }
        case OP_RETURN: {
            Value result = vm_pop();
            close_upvalues(frame->slots);
            vm.frame_size--;
            if (vm.frame_size == 0) {
                vm_pop();
                return VM_OK;
            }
            vm.sp = frame->slots;
            vm_push(result);
            frame = &vm.frames[vm.frame_size-1];
            break;
        }
        case OP_CLOSURE: {
            ObjFunction *fun = AS_FUNCTION(READ_CONSTANT());
            ObjClosure *closure = obj_make_closure(fun);
            vm_push(VALUE_MKOBJ(closure));
            for (int i = 0; i < closure->upvalue_count; i++) {
                u8 is_local = READ_BYTE();
                u8 index    = READ_BYTE();
                if (is_local)
                    closure->upvalues[i] = capture_upvalue(frame->slots + index);
                else
                    closure->upvalues[i] = frame->closure->upvalues[index];
            }
            break;
        }
        case OP_CLOSE_UPVALUE:
            close_upvalues(vm.sp - 1);
            vm_pop();
            break;
        case OP_CLASS:
            vm_push(VALUE_MKOBJ(obj_make_class(READ_STRING())));
            break;
        case OP_METHOD:
            define_method(READ_STRING());
            break;
        case OP_INHERIT: {
            Value superclass = peek(1);
            if (!IS_CLASS(superclass)) {
                runtime_error("superclass must be a class");
                return VM_RUNTIME_ERROR;
            }
            ObjClass *subclass = AS_CLASS(peek(0));
            table_add_all(&AS_CLASS(superclass)->methods, &subclass->methods);
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
    vm.bytes_allocated = 0;
    vm.next_gc = 1024 * 1024;
    table_init(&vm.globals);
    table_init(&vm.strings);
    vm.init_string = NULL;
    vm.init_string = obj_copy_string("init", 4);
    graystack_init(&vm.gray_stack);
    define_native("clock", clock_native);
}

void vm_free()
{
    table_free(&vm.globals);
    table_free(&vm.strings);
    obj_free_arr(vm.objects);
    vm.init_string = NULL;
    free(vm.gray_stack.stack);
}

VMResult vm_interpret(const char *src, const char *filename)
{
    ObjFunction *fun = compile(src, filename);
    if (!fun)
        return VM_COMPILE_ERROR;
    vm_push(VALUE_MKOBJ(fun));
    ObjClosure *closure = obj_make_closure(fun);
    vm_pop();
    vm_push(VALUE_MKOBJ(closure));
    call(closure, 0);
    vm.filename = filename;

#ifdef DEBUG_TRACE_EXECUTION
    printf("=== running VM ===\n");
#endif

    return run();
}

VECTOR_DEFINE_INIT(GrayStack, Obj *, graystack, stack)

void graystack_write(GrayStack *arr, Obj *obj)
{
    if (arr->cap < arr->size + 1) {
        arr->cap = vector_grow_cap(arr->cap);
        arr->stack = realloc(arr->stack, sizeof(Obj *) * arr->cap);
        if (!arr->stack) {
            fprintf(stderr, "panic: gray stack is NULL\n");
            exit(1);
        }
    }
    arr->stack[arr->size++] = obj;
}
