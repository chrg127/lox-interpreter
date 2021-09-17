#include "object.h"

#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "table.h"
#include "vm.h"
#include "debug.h"

#ifdef DEBUG_LOG_GC
static const char *type_tostring(ObjType type)
{
    switch (type) {
    case OBJ_STRING:   return "ObjString";
    case OBJ_FUNCTION: return "ObjFunction";
    case OBJ_NATIVE:   return "ObjNative";
    case OBJ_CLOSURE:  return "ObjClosure";
    case OBJ_UPVALUE:  return "ObjUpvalue";
    default:           return "ERROR";
    }
}
#endif

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
    fun->arity         = 0;
    fun->upvalue_count = 0;
    fun->name          = NULL;
    chunk_init(&fun->chunk);
    return fun;
}

ObjNative *obj_make_native(NativeFn fun, const char *name, u8 arity)
{
    ObjNative *native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->fun   = fun;
    native->name  = name;
    native->arity = arity;
    return native;
}

ObjUpvalue *obj_make_upvalue(Value *slot)
{
    ObjUpvalue *upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->location = slot;
    upvalue->closed   = VALUE_MKNIL();
    upvalue->next     = NULL;
    return upvalue;
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

ObjClass *obj_make_class(ObjString *name)
{
    ObjClass *klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    klass->name = name;
    klass->ctor = VALUE_MKNIL();
    table_init(&klass->methods);
    return klass;
}

ObjInstance *obj_make_instance(ObjClass *klass)
{
    ObjInstance *inst = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    inst->klass = klass;
    table_init(&inst->fields);
    return inst;
}

ObjBoundMethod *obj_make_bound_method(Value receiver, ObjClosure *method)
{
    ObjBoundMethod *bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
    bound->receiver = receiver;
    bound->method   = method;
    return bound;
}

void obj_print(Value value, bool debug)
{
    switch (OBJ_TYPE(value)) {
    case OBJ_STRING:    printf(debug ? "\"%s\"" : "%s", AS_STRING(value)->data); break;
    case OBJ_FUNCTION:  print_function(AS_FUNCTION(value)); break;
    case OBJ_NATIVE:    printf("<native fn '%s'>", ((ObjNative *)AS_OBJ(value))->name); break;
    case OBJ_CLOSURE:   print_function(AS_CLOSURE(value)->fun); break;
    case OBJ_UPVALUE:   printf("upvalue"); break;
    case OBJ_CLASS:
        printf("<class ");
        obj_print(VALUE_MKOBJ(AS_CLASS(value)->name), false);
        printf(">");
        break;
    case OBJ_INSTANCE:
        printf("<instance of class ");
        obj_print(VALUE_MKOBJ(AS_INSTANCE(value)->klass->name), false);
        printf(">");
        break;
    case OBJ_BOUND_METHOD: print_function(AS_BOUND_METHOD(value)->method->fun); break;
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
    case OBJ_CLASS: {
        ObjClass *klass = (ObjClass *)obj;
        table_free(&klass->methods);
        FREE(ObjClass, obj);
        break;
    }
    case OBJ_INSTANCE: {
        ObjInstance *inst = (ObjInstance *)obj;
        table_free(&inst->fields);
        FREE(ObjInstance, obj);
        break;
    }
    case OBJ_BOUND_METHOD:
        FREE(ObjBoundMethod, obj);
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

static inline Value fun_tostring(ObjFunction *fun)
{
    return fun->name == NULL ? obj_make_ssostring("<script>", 8)
                             : VALUE_MKOBJ(fun->name);
}

Value obj_tostring(Value value)
{
    switch (AS_OBJ(value)->type) {
    case OBJ_STRING:   return value;
    case OBJ_FUNCTION: return fun_tostring(AS_FUNCTION(value));
    case OBJ_NATIVE: {
        const char *name = AS_NATIVE_OBJ(value)->name;
        return obj_make_ssostring(name, strlen(name));
    }
    case OBJ_CLOSURE:  return fun_tostring(AS_CLOSURE(value)->fun);
    case OBJ_UPVALUE:  return obj_make_ssostring("upvalue", 7);
    case OBJ_CLASS:    return VALUE_MKOBJ(AS_CLASS(value)->name);
    case OBJ_INSTANCE: {
        ObjInstance *inst = AS_INSTANCE(value);
        ObjString *a = inst->klass->name;
        ObjString *b = obj_copy_string(" instance", 9);
        return obj_concat(VALUE_MKOBJ(a), VALUE_MKOBJ(b));
    }
    default: return VALUE_MKNIL(); // unreachable
    }
}

static inline void concat_cstr(char *dest, char *a, size_t alen, char *b, size_t blen)
{
    memcpy(dest,        a, alen);
    memcpy(dest + alen, b, blen);
    dest[alen+blen] = '\0';
}

#define OBJ_STRDATA(v)  IS_SSTR(v) ? AS_SSTR(v)         : AS_STRING(v)->data
#define OBJ_STRLEN(v)   IS_SSTR(v) ? strlen(AS_SSTR(v)) : AS_STRING(v)->len

Value obj_concat(Value a, Value b)
{
    char *as = OBJ_STRDATA(a);
    char *bs = OBJ_STRDATA(b);
    size_t alen = OBJ_STRLEN(a);
    size_t blen = OBJ_STRLEN(b);
    size_t len = alen + blen;
    if (len < VALUE_SSO_SIZE) {
        Value value = VALUE_MKSSTR();
        concat_cstr(AS_SSTR(value), as, alen, bs, blen);
        return value;
    }
    char *data = ALLOCATE(char, len+1);
    concat_cstr(data, as, alen, bs, blen);
    return VALUE_MKOBJ(obj_take_string(data, len));
}
