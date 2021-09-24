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
    u8 *ip;                         // instruction pointer
    Value *sp;                      // stack pointer
    size_t frame_count;
    Table globals;
    Table strings;                  // interned strings; see alloc_str()
    Obj *objects;                   // linked list of all allocated object; see alloc_obj()
    ObjUpvalue *open_upvalues;      // sorted linked list of upvalues that still point to a variable on the stack

    const char *filename;           // name of file passed by argv, or "script" if none
    ObjString *init_string;         // interned string "init" for fast ctor access

    size_t bytes_allocated;
    size_t next_gc;                 // how many bytes must be allocated to trigger the garbage collector
    GrayStack gray_stack;           // used by gc to hold traced objects; see gc_mark_obj()

    Value stack[STACK_MAX];
    CallFrame frames[FRAMES_MAX];
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

// no free, because the gray stack is allocated manually
VECTOR_DECLARE_INIT(GrayStack, Obj *, graystack);
VECTOR_DECLARE_WRITE(GrayStack, Obj *, graystack);

#endif
