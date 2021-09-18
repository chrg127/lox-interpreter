#ifndef LIST_H_INCLUDED
#define LIST_H_INCLUDED

#define LIST_FOR_EACH(type, head, entry) \
    for (type *entry = head; entry != NULL; entry = entry->next)

#define LIST_FOR_EACH_SAFE(type, head, entry)                 \
    for (type *entry = head, *n = entry ? entry->next : NULL; \
         entry != NULL && (n = entry->next, true);            \
         entry = n)

#define LIST_APPEND(item, list, next) \
    do { (item)->next = (list); (list) = (item); } while (0)

#endif
