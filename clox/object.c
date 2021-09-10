#include "object.h"

#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "table.h"
#include "vm.h"
#include "debug.h"

static const char *type_tostring(ObjType type)
{
    switch (type) {
    case OBJ_STRING:   return "ObjString";
    case OBJ_FUNCTION: return "ObjFunction";
    case OBJ_NATIVE:   return "ObjNative";
    case OBJ_CLOSURE:  return "ObjClosure";
    case OBJ_UPVALUE:  return "ObjUpvalue";
    default: return "NoType";
    }
}

static Obj *alloc_obj(size_t size, ObjType type)
{
    Obj *obj = reallocate(NULL, 0, size);
    obj->type = type;
    obj->marked = false;
    obj->next = vm.objects;
    vm.objects = obj;

#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %s\n", (void *) obj, size, type_tostring(type));
#endif

    return obj;
}

#define ALLOCATE_OBJ(type, obj_type) \
    (type *) alloc_obj(sizeof(type), obj_type)

static ObjString *alloc_str(char *data, size_t len, u32 hash)
{
    ObjString *str = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    str->len  = len;
    str->data = data;
    str->hash = hash;
    vm_push(VALUE_MKOBJ(str));
    table_install(&vm.strings, str, VALUE_MKNIL());
    vm_pop();
    return str;
}

// algorithm: FNV-1a
static u32 hash_string(const char *str, size_t len)
{
    u32 hash = 2166136261u;
    for (size_t i = 0; i < len; i++) {
        hash ^= (u8) str[i];
        hash *= 16777619;
    }
    return hash;
}

static void print_function(ObjFunction *fun)
{
    if (fun->name == NULL) {
        printf("<script>");
        return;
    }
    printf("<fn %s>", fun->name->data);
}



ObjString *obj_copy_string(const char *str, size_t len)
{
    u32 hash = hash_string(str, len);
    ObjString *interned = table_find_string(&vm.strings, str, len, hash);
    if (interned != NULL)
        return interned;
    char *newstr = ALLOCATE(char, len + 1);
    memcpy(newstr, str, len);
    newstr[len] = '\0';
    return alloc_str(newstr, len, hash);
}

ObjString *obj_take_string(char *data, size_t len)
{
    u32 hash = hash_string(data, len);
    ObjString *interned = table_find_string(&vm.strings, data, len, hash);
    if (interned != NULL) {
        FREE_ARRAY(char, data, len + 1);
        return interned;
    }
    return alloc_str(data, len, hash);
}

ObjFunction *obj_make_fun()
{
    ObjFunction *fun = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    fun->arity = 0;
    fun->upvalue_count = 0;
    fun->name = NULL;
    chunk_init(&fun->chunk);
    return fun;
}

ObjNative *obj_make_native(NativeFn fun, const char *name, u8 arity)
{
    ObjNative *native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->fun  = fun;
    native->name = name;
    native->arity = arity;
    return native;
}

ObjClosure *obj_make_closure(ObjFunction *fun)
{
    ObjUpvalue **upvalues = ALLOCATE(ObjUpvalue *, fun->upvalue_count);
    for (int i = 0; i < fun->upvalue_count; i++)
        upvalues[i] = NULL;
    ObjClosure *closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->fun           = fun;
    closure->upvalues      = upvalues;
    closure->upvalue_count = fun->upvalue_count;
    return closure;
}

ObjUpvalue *obj_make_upvalue(Value *slot)
{
    ObjUpvalue *upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->location = slot;
    upvalue->closed   = VALUE_MKNIL();
    upvalue->next     = NULL;
    return upvalue;
}

void obj_print(Value value)
{
    switch (OBJ_TYPE(value)) {
    case OBJ_STRING: printf("%.*s", (int) AS_STRING(value)->len, AS_STRING(value)->data); break;
    case OBJ_FUNCTION: print_function(AS_FUNCTION(value)); break;
    case OBJ_NATIVE: printf("<native fn '%s'>", ((ObjNative *)AS_OBJ(value))->name); break;
    case OBJ_CLOSURE: print_function(AS_CLOSURE(value)->fun); break;
    case OBJ_UPVALUE: printf("upvalue"); break;
    }
}

void obj_free(Obj *obj)
{
#ifdef DEBUG_LOC_GC
    printf("%p free type %s\n", (void *)obj, type_tostring(obj->type));
#endif

    switch (obj->type) {
    case OBJ_STRING: {
        ObjString *str = (ObjString *)obj;
        FREE_ARRAY(char, str->data, str->len+1);
        FREE(ObjString, obj);
        break;
    }
    case OBJ_FUNCTION: {
        ObjFunction *fun = (ObjFunction *)obj;
        chunk_free(&fun->chunk);
        FREE(ObjFunction, obj);
        break;
    }
    case OBJ_NATIVE:
        FREE(ObjNative, obj);
        break;
    case OBJ_CLOSURE: {
        ObjClosure *closure = (ObjClosure *)obj;
        FREE_ARRAY(ObjUpvalue *, closure->upvalues, closure->upvalue_count);
        FREE(ObjClosure, obj);
        break;
    }
    case OBJ_UPVALUE:
        FREE(ObjUpvalue, obj);
        break;
    }
}

void obj_free_arr(Obj *objects)
{
    Obj *obj = objects;
    while (obj != NULL) {
        Obj *next = obj->next;
        obj_free(obj);
        obj = next;
    }
}

u32 obj_hash(Obj *obj)
{
    switch (obj->type) {
    case OBJ_STRING: return ((ObjString *)obj)->hash;
    case OBJ_FUNCTION: return ((ObjFunction *)obj)->name->hash;
    case OBJ_NATIVE: {
        const char *str = ((ObjNative *)obj)->name;
        return hash_string(str, strlen(str));
    }
    default: return 0;
    }
}

static ObjString *fun_tostring(ObjFunction *fun)
{
    return fun->name == NULL ? obj_copy_string("<script>", 8)
                             : fun->name;
}

ObjString *obj_tostring(Value value)
{
    switch (AS_OBJ(value)->type) {
    case OBJ_STRING:   return AS_STRING(value);
    case OBJ_FUNCTION: return fun_tostring(AS_FUNCTION(value));
    case OBJ_NATIVE: {
        const char *name = ((ObjNative *)AS_OBJ(value))->name;
        return obj_copy_string(name, strlen(name));
    }
    case OBJ_CLOSURE:  return fun_tostring(AS_CLOSURE(value)->fun);
    case OBJ_UPVALUE:  return obj_copy_string("upvalue", 7);
    default: return NULL; // unreachable
    }
}
