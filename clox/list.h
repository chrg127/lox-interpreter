#ifndef LIST_H_INCLUDED
#define LIST_H_INCLUDED

#define LIST_FOR_EACH(type, head, entry) \
    for (type *entry = head; entry != NULL; entry = entry->next)

#endif
