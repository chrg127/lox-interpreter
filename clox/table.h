#ifndef TABLE_H_INCLUDED
#define TABLE_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include "value.h"

typedef Value TableKey;
typedef Value TableValue;

typedef struct {
    TableValue key;
    TableValue value;
} Entry;

typedef struct Table {
    size_t size;
    size_t cap;
    Entry *entries;
} Table;

// install() returns whether it created (true) or modified (false) the key's value
// lookup() returns whether the lookup went well
// delete() returns whether the delete went well
// add_all() copies elements from another table

void table_init(Table *tab);
void table_free(Table *tab);
bool table_install_value(Table *tab, TableKey key, TableValue value);
bool table_lookup_value(Table *tab, TableKey key, TableValue *value);
bool table_delete_value(Table *tab, TableKey key);
void table_add_all(Table *from, Table *to);
bool table_empty_entry(Entry *entry);
ObjString *table_find_string(Table *tab, const char *data, size_t len, u32 hash);

static inline bool table_install(Table *tab, ObjString *key, Value value) { return table_install_value(tab, VALUE_MKOBJ(key), value); }
static inline bool table_lookup(Table *tab, ObjString *key, Value *value) { return table_lookup_value(tab, VALUE_MKOBJ(key), value); }
static inline bool table_delete(Table *tab, ObjString *key)               { return table_delete_value(tab, VALUE_MKOBJ(key)); }

u32 hash_string(const char *str, size_t len);

#define TABLE_FOR_EACH(tab, entry) \
    for (Entry *entry = tab->entries; ((size_t) (entry - tab->entries)) < tab->cap; entry++)

#endif
