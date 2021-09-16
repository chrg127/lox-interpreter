#include "table.h"

#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "vector.h"
#include "object.h"

// algorithm: FNV-1a
u32 hash_string(const char *str, size_t len)
{
    u32 hash = 2166136261u;
    for (size_t i = 0; i < len; i++) {
        hash ^= (u8) str[i];
        hash *= 16777619;
    }
    return hash;
}

static bool is_empty_key(Value v)        { return IS_NIL(v); }
static bool is_empty_value(Entry *entry) { return IS_NIL(entry->value); }
static u32 hash(Value v)                 { return value_hash(v); }

static void make_empty(Entry *entry)
{
    entry->key   = VALUE_MKNIL();
    entry->value = VALUE_MKNIL();
}

static void make_tombstone(Entry *entry)
{
    entry->key   = VALUE_MKNIL();
    entry->value = VALUE_MKBOOL(true);
}



// the actual table implementation

#define TABLE_MAX_LOAD 0.75

static Entry *find_entry(Entry *entries, size_t cap, TableKey key)
{
    u32 i = hash(key) % cap;
    Entry *first_tombstone = NULL;
    for (;;) {
        Entry *ptr = &entries[i];
        // an entry is a tombstone if the key is empty and the value is not
        if (is_empty_key(ptr->key)) {
            if (is_empty_value(ptr))
                return first_tombstone != NULL ? first_tombstone : ptr;
            else if (first_tombstone == NULL)
                first_tombstone = ptr;
        } else if (value_equal(ptr->key, key))
            return ptr;
        i = (i + 1) & (cap - 1);
    }
}

static void adjust_cap(Table *tab, size_t cap)
{
    Entry *entries = ALLOCATE(Entry, cap);
    for (size_t i = 0; i < cap; i++)
        make_empty(&entries[i]);

    tab->size = 0;
    for (size_t i = 0; i < tab->cap; i++) {
        Entry *entry = &tab->entries[i];
        if (is_empty_key(entry->key))
            continue;
        Entry *dest = find_entry(entries, cap, entry->key);
        dest->key   = entry->key;
        dest->value = entry->value;
        tab->size++;
    }
    FREE_ARRAY(Entry, tab->entries, tab->cap);

    tab->entries = entries;
    tab->cap     = cap;
}

void table_init(Table *tab)
{
    tab->size    = 0;
    tab->cap     = 0;
    tab->entries = NULL;
}

void table_free(Table *tab)
{
    FREE_ARRAY(Entry, tab->entries, tab->cap);
    table_init(tab);
}

bool table_install_value(Table *tab, TableKey key, TableValue value)
{
    // don't insert nil values
    if (is_empty_key(key))
        return false;

    if (tab->size + 1 > tab->cap * TABLE_MAX_LOAD) {
        size_t cap = vector_grow_cap(tab->cap);
        adjust_cap(tab, cap);
    }

    Entry *entry = find_entry(tab->entries, tab->cap, key);
    bool is_new = is_empty_key(entry->key);
    if (is_new && is_empty_value(entry))
        tab->size++;
    entry->key   = key;
    entry->value = value;
    return is_new;
}

bool table_lookup_value(Table *tab, TableKey key, TableValue *value)
{
    if (tab->size == 0)
        return false;
    Entry *entry = find_entry(tab->entries, tab->cap, key);
    if (is_empty_key(entry->key))
        return false;
    *value = entry->value;
    return true;
}

bool table_delete_value(Table *tab, TableKey key)
{
    if (tab->size == 0)
        return false;
    Entry *entry = find_entry(tab->entries, tab->cap, key);
    if (is_empty_key(entry->key))
        return false;
    // place a tombstone
    make_tombstone(entry);
    return true;
}

void table_add_all(Table *from, Table *to)
{
    for (size_t i = 0; i < from->cap; i++) {
        Entry *entry = &from->entries[i];
        if (is_empty_key(entry->key))
            table_install_value(to, entry->key, entry->value);
    }
}

bool table_empty_entry(Entry *entry)
{
    return is_empty_key(entry->key);
}

static bool is_objstring(Value v)
{
    return IS_OBJ(v) && AS_OBJ(v)->type == OBJ_STRING;
}

// find a string key that is equal to data. the table here is used as a Set.
ObjString *table_find_string(Table *tab, const char *data, size_t len, u32 hash)
{
    if (tab->size == 0)
        return NULL;
    u32 i = hash & (tab->cap - 1);
    for (;;) {
        Entry *entry = &tab->entries[i];
        if (is_empty_key(entry->key)) {
            if (is_empty_value(entry))
                return NULL;
        } else if (is_objstring(entry->key)) {
            ObjString *str = AS_STRING(entry->key);
            if (str->len == len
                && str->hash == hash
                && memcmp(str->data, data, len) == 0) {
                return str;
            }
        }
        i = (i + 1) & (tab->cap - 1);
    }
}
