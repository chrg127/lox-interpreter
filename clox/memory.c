#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "value.h"
#include "object.h"
#include "table.h"
#include "vm.h"
#include "list.h"
#include "compiler.h"
#include "debug.h"

#define GC_HEAP_GROW_FACTOR 2

static void mark_roots()
{
    for (Value *slot = vm.stack; slot < vm.sp; slot++)
        gc_mark_value(*slot);
    for (size_t i = 0; i < vm.frame_size; i++)
        gc_mark_obj((Obj *) vm.frames[i].closure);
    LIST_FOR_EACH(ObjUpvalue, vm.open_upvalues, upvalue)
        gc_mark_obj((Obj *) upvalue);
    gc_mark_table(&vm.globals);
    compiler_mark_roots();
}

static void gc_mark_arr(ValueArray *arr)
{
    for (size_t i = 0; i < arr->size; i++)
        gc_mark_value(arr->values[i]);
}

static void mark_black(Obj *obj)
{
#ifdef DEBUG_LOC_GC
    printf("%p mark black ", (void *)obj);
    value_print(VALUE_MKOBJ(obj));
    printf("\n");
#endif

    switch (obj->type) {
    case OBJ_NATIVE:
    case OBJ_STRING:
        break;
    case OBJ_UPVALUE:
        gc_mark_value(((ObjUpvalue *)obj)->closed);
        break;
    case OBJ_FUNCTION: {
        ObjFunction *fun = (ObjFunction *)obj;
        gc_mark_obj((Obj *)fun->name);
        gc_mark_arr(&fun->chunk.constants);
        break;
    }
    case OBJ_CLOSURE: {
        ObjClosure *closure = (ObjClosure *)obj;
        gc_mark_obj((Obj *)closure->fun);
        for (int i = 0; i < closure->upvalue_count; i++)
            gc_mark_obj((Obj *)closure->upvalues[i]);
        break;
    }
    }
}

static void trace_refs()
{
    while (vm.gray_stack.size > 0) {
        Obj *obj = vm.gray_stack.stack[--vm.gray_stack.size];
        mark_black(obj);
    }
}

static void remove_whites(Table *tab)
{
    // TABLE_FOR_EACH(tab, entry) {
    //     if (entry->key != NULL && !entry->key->obj.marked)
    //         table_delete(tab, entry->key);
    // }
    for (size_t i = 0; i < tab->cap; i++) {
        Entry *entry = &tab->entries[i];
        if (!IS_STRING(entry->key))
            continue;
        ObjString *key = AS_STRING(entry->key);
        if (!key->obj.marked)
            table_delete(tab, key);
    }
}

static void sweep()
{
    Obj *prev = NULL;
    Obj *obj  = vm.objects;
    while (obj != NULL) {
        if (obj->marked) {
            obj->marked = false;
            prev = obj;
            obj  = obj->next;
        } else {
            Obj *unreached = obj;
            obj = obj->next;
            if (prev != NULL)
                prev->next = obj;
            else
                vm.objects = obj;
            obj_free(unreached);
        }
    }
}

void *reallocate(void *ptr, size_t old, size_t new)
{
    vm.bytes_allocated += new - old;

    if (new > old) {
#ifdef DEBUG_STRESS_GC
        gc_collect();
#endif
    }

    if (vm.bytes_allocated > vm.next_gc)
        gc_collect();

    if (new == 0) {
        free(ptr);
        return NULL;
    }
    void *res = realloc(ptr, new);
    if (!res)
        abort();
    return res;
}

void gc_collect()
{
#ifdef DEBUG_LOC_GC
    printf("-- gc begin\n");
    size_t before = vm.bytes_allocated;
#endif

    mark_roots();
    trace_refs();
    remove_whites(&vm.strings);
    sweep();
    vm.next_gc = vm.bytes_allocated = GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOC_GC
    printf("-- gc end\n");
    printf("   collected %zu bytes (%zu -> %zu), next at %zu\n",
        before - vm.bytes_allocated, before, vm.bytes_allocated, vm.next_gc);
#endif
}

void gc_mark_table(Table *tab)
{
    // TABLE_FOR_EACH(tab, entry) {
    //     gc_mark_obj((Obj *)entry->key);
    //     gc_mark_value(entry->value);
    // }
    for (size_t i = 0; i < tab->cap; i++) {
        Entry *entry = &tab->entries[i];
        gc_mark_value(entry->key);
        gc_mark_value(entry->value);
    }
}

void gc_mark_value(Value value)
{
    if (IS_OBJ(value))
        gc_mark_obj(AS_OBJ(value));
}

void gc_mark_obj(Obj *obj)
{
    if (obj == NULL || obj->marked)
        return;

#ifdef DEBUG_LOC_GC
    printf("%p mark ", (void *)obj);
    value_print(VALUE_MKOBJ(obj));
    printf("\n");
#endif

    obj->marked = true;
    graystack_write(&vm.gray_stack, obj);
}
