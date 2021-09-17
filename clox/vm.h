#ifndef VM_H_INCLUDED
#define VM_H_INCLUDED

#include <stddef.h>
#include "chunk.h"
#include "uint.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vector.h"

#define STACK_MAX UINT16_MAX
#define FRAMES_MAX 64

typedef struct {
    ObjClosure *closure;
    ObjFunction *fun;
    u8 *ip;
    Value *slots;
} CallFrame;

typedef struct {
    Obj **stack;
    size_t size;
    size_t cap;
} GrayStack;

typedef struct {
    const char *filename;
    CallFrame frames[FRAMES_MAX];
    size_t frame_size;
    u8 *ip;
    Value stack[STACK_MAX];
    Value *sp;
    Table globals;
    Table strings;
    ObjString *init_string;
    ObjUpvalue *open_upvalues;
    size_t bytes_allocated;
    size_t next_gc;
    Obj *objects;
    GrayStack gray_stack;
} VM;

extern VM vm;

typedef enum {
    VM_OK,
    VM_COMPILE_ERROR,
    VM_RUNTIME_ERROR,
} VMResult;

void vm_init();
void vm_free();
VMResult vm_interpret(const char *src, const char *filename);
void native_runtime_error(const char *fn, const char *fmt, ...);
void vm_push(Value value);
Value vm_pop();
bool vm_call(ObjString *name, u8 argc, Value *out);
bool vm_invoke(ObjString *name, u8 argc, Value *out);

// no free, because the gray stack is allocated manually
VECTOR_DECLARE_INIT(GrayStack, Obj *, graystack);
VECTOR_DECLARE_WRITE(GrayStack, Obj *, graystack);

#endif
