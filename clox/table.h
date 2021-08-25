#ifndef TABLE_H_INCLUDED
#define TABLE_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include "value.h"
#include "object.h"

typedef struct {
    Value key;
    // ObjString *key;
    Value value;
} Entry;

typedef struct {
    size_t size;
    size_t cap;
    Entry *entries;
} Table;

void table_init(Table *tab);
void table_free(Table *tab);
// bool table_install(Table *tab, ObjString *key, Value value);
// bool table_lookup(Table *tab, ObjString *key, Value *value);
// bool table_delete(Table *tab, ObjString *key);
bool table_install(Table *tab, Value key, Value value);
bool table_lookup(Table *tab, Value key, Value *value);
bool table_delete(Table *tab, Value key);
void table_add_all(Table *from, Table *to);
ObjString *table_find_string(Table *tab, const char *data, size_t len,
                             u32 hash);

#endif
