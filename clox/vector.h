#ifndef VECTOR_H_INCLUDED
#define VECTOR_H_INCLUDED

#include "memory.h"

#define VECTOR_INIT(v, data_name) \
    do { \
        (v)->size = 0; \
        (v)->cap  = 0; \
        (v)->data_name = NULL; \
    } while (0)

static inline size_t vector_grow_cap(size_t old_cap)
{
    return old_cap < 8 ? 8 : old_cap * 2;
}

#define VECTOR_DECLARE(T, TVal, header)         \
    void header##_init(T *arr);                 \
    void header##_write(T *arr, TVal value);    \
    void header##_free(T *arr);                 \

#define VECTOR_DEFINE(T, TVal, header, data_name)       \
    void header##_init(T *arr)                          \
    {                                                   \
        arr->size = 0;                                  \
        arr->cap  = 0;                                  \
        arr->data_name = NULL;                          \
    }                                                   \
                                                        \
    void header##_write(T *arr, TVal value)             \
    {                                                   \
        if (arr->cap < arr->size + 1) {                 \
            size_t old = arr->cap;                      \
            arr->cap = vector_grow_cap(old);            \
            arr->data_name = GROW_ARRAY(TVal,    \
                arr->data_name, old, arr->cap);         \
        }                                               \
        arr->data_name[arr->size++] = value;            \
    }                                                   \
                                                        \
    void header##_free(T *arr)                          \
    {                                                   \
        FREE_ARRAY(TVal, arr->data_name, arr->cap);     \
        header##_init(arr);                             \
    }                                                   \

#endif
