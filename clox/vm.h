#ifndef VM_H_INCLUDED
#define VM_H_INCLUDED

#include "chunk.h"
#include "uint.h"
#include "table.h"
#include "value.h"

#define STACK_MAX 16777216
#define FRAMES_MAX 64

typedef struct {
    ObjClosure *closure;
    ObjFunction *fun;
    u8 *ip;
    Value *slots;
} CallFrame;

typedef struct {
    CallFrame frames[FRAMES_MAX];
    size_t frame_size;
    u8 *ip;
    Value *stack;//[STACK_MAX];
    Value *sp;
    Table globals;
    Table strings;
    ObjUpvalue *open_upvalues;
    Obj *objects;
    const char *filename;
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
// void runtime_error(const char *fmt, ...);

#endif
