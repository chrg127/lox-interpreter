#ifndef VM_H_INCLUDED
#define VM_H_INCLUDED

#include <stddef.h>
#include "chunk.h"
#include "uint.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vector.h"

#define FRAMES_MAX 64
#define STACK_MAX 256

typedef struct {
    ObjClosure *closure;
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
void vm_push(Value value);
Value vm_pop();

VECTOR_DECLARE_INIT(GrayStack, Obj *, graystack);
VECTOR_DECLARE_WRITE(GrayStack, Obj *, graystack);

#endif
