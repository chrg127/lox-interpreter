#ifndef VM_H_INCLUDED
#define VM_H_INCLUDED

#include "chunk.h"
#include "uint.h"
#include "value.h"

#define STACK_MAX 256

typedef struct {
    Chunk *chunk;
    u8 *ip;
    Value stack[STACK_MAX];
    Value *sp;
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
void vm_push(Value value);
Value vm_pop();

#endif
