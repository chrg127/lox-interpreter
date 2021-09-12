#ifndef TABLE_H_INCLUDED
#define TABLE_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include "value.h"

typedef struct {
    ObjString *key;
    Value value;
} Entry;

typedef struct Table {
    size_t size;
    size_t cap;
    Entry *entries;
} Table;

void table_init(Table *tab);
void table_free(Table *tab);
bool table_install(Table *tab, ObjString *key, Value value);
void table_add_all(Table *from, Table *to);
bool table_lookup(Table *tab, ObjString *key, Value *value);
bool table_delete(Table *tab, ObjString *key);
ObjString *table_find_string(Table *tab, const char *data, size_t len,
                             u32 hash);

#define TABLE_FOR_EACH(tab, entry) \
    for (Entry *entry = tab->entries; ((size_t) (entry - tab->entries)) < tab->cap; entry++)

#endif
