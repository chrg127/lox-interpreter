#include "table.h"

#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "vector.h"
#include "object.h"

#define TABLE_MAX_LOAD 0.75

static bool is_empty(Entry *entry)
{
    return IS_NIL(entry->value);
}

static Value empty_value()
{
    return VALUE_MKNIL();
}

static ObjString *empty_key()
{
    return NULL;
}

static Value make_tombstone()
{
    return VALUE_MKBOOL(true);
}

static bool objstr_cmp(ObjString *a, ObjString *b)
{
    return a == b; // we have string interning
}

static bool objstring_is_null(ObjString *str) { return str == NULL; }

static u32 objstr_hash(ObjString *str) { return str->hash; }

static Entry *find_entry(Entry *entries, size_t cap, ObjString *key)
{
    u32 i = objstr_hash(key) % cap;
    Entry *first_tombstone = NULL;
    for (;;) {
        Entry *ptr = &entries[i];
        // our tombstone representation is key = NULL and value = true_obj
        // our empty entry representation is key = NULL and value = nil_obj
        if (objstring_is_null(ptr->key)) {
            if (is_empty(ptr))
                return first_tombstone != NULL ? first_tombstone : ptr;
            else if (first_tombstone == NULL)
                first_tombstone = ptr;
        } else if (objstr_cmp(ptr->key, key))
            return ptr;
        i = (i + 1) & (cap - 1);
    }
}

static void adjust_cap(Table *tab, size_t cap)
{
    Entry *entries = ALLOCATE(Entry, cap);
    for (size_t i = 0; i < cap; i++) {
        entries[i].key   = empty_key();
        entries[i].value = empty_value();
    }

    tab->size = 0;
    for (size_t i = 0; i < tab->cap; i++) {
        Entry *entry = &tab->entries[i];
        if (objstring_is_null(entry->key))
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

bool table_install(Table *tab, ObjString *key, Value value)
{
    if (tab->size + 1 > tab->cap * TABLE_MAX_LOAD) {
        size_t cap = vector_grow_cap(tab->cap);
        adjust_cap(tab, cap);
    }

    Entry *entry = find_entry(tab->entries, tab->cap, key);
    bool is_new = objstring_is_null(entry->key);
    if (is_new && is_empty(entry))
        tab->size++;
    entry->key   = key;
    entry->value = value;
    return is_new;
}

void table_add_all(Table *from, Table *to)
{
    for (size_t i = 0; i < from->cap; i++) {
        Entry *entry = &from->entries[i];
        if (objstring_is_null(entry->key))
            table_install(to, entry->key, entry->value);
    }
}

bool table_lookup(Table *tab, ObjString *key, Value *value)
{
    if (tab->size == 0)
        return false;
    Entry *entry = find_entry(tab->entries, tab->cap, key);
    if (objstring_is_null(entry->key))
        return false;
    *value = entry->value;
    return true;
}

bool table_delete(Table *tab, ObjString *key)
{
    if (tab->size == 0)
        return false;
    Entry *entry = find_entry(tab->entries, tab->cap, key);
    if (objstring_is_null(entry->key))
        return false;
    // place a tombstone
    entry->key   = NULL;
    entry->value = make_tombstone();
    return true;
}

/* find a string key that is equal to data.
 * the table here is used as a Set. */
ObjString *table_find_string(Table *tab, const char *data, size_t len,
                             u32 hash)
{
    if (tab->size == 0)
        return NULL;
    u32 i = hash & (tab->cap - 1);
    for (;;) {
        Entry *entry = &tab->entries[i];
        if (objstring_is_null(entry->key)) {
            if (is_empty(entry))
                return NULL;
        } else if (entry->key->len == len
                && entry->key->hash == hash
                && memcmp(entry->key->data, data, len) == 0) {
            return entry->key;
        }
        i = (i + 1) & (tab->cap - 1);
    }
}

