#ifndef TABLE_H_INCLUDED
#define TABLE_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include "value.h"

typedef struct {
    Value key;
    // ObjString *key;
    Value value;
} Entry;

typedef struct Table {
    size_t size;
    size_t cap;
    Entry *entries;
} Table;

void table_init(Table *tab);
void table_free(Table *tab);

// install returns whether it created (true) or modified (false) the key's value
// lookup returns whether the lookup went well
// delete returns whether the delete went well

bool table_install_value(Table *tab, Value key, Value value);
bool table_lookup_value(Table *tab, Value key, Value *value);
bool table_delete_value(Table *tab, Value key);

static inline bool table_install(Table *tab, ObjString *key, Value value) { return table_install_value(tab, VALUE_MKOBJ(key), value); }
static inline bool table_lookup(Table *tab, ObjString *key, Value *value) { return table_lookup_value(tab, VALUE_MKOBJ(key), value); }
static inline bool table_delete(Table *tab, ObjString *key)               { return table_delete_value(tab, VALUE_MKOBJ(key)); }

void table_add_all(Table *from, Table *to);
ObjString *table_find_string(Table *tab, const char *data, size_t len, u32 hash);

#define TABLE_FOR_EACH(tab, entry) \
    for (Entry *entry = tab->entries; ((size_t) (entry - tab->entries)) < tab->cap; entry++)

#endif
