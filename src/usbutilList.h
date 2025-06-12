#ifndef USBUTILLIST_H
#define USBUTILLIST_H

#define INIT_DL_LIST(list) init_list(list);

#include "utilities.h"

struct list_head{
    struct list_head *prev, *next;
};

static inline void list_init(struct list_head* ptr){
    ptr->prev = ptr;
    ptr->next = ptr;
}

static inline void list_del(struct list_head* ptr){
    ptr->next->prev = ptr->prev;
    ptr->prev->next = ptr->next;
    ptr->next = NULL;
    ptr->prev = NULL;
}

static inline void list_add(struct list_head* list, struct list_head* add){
    list->next = add;
    add->prev = list;
}

#define extract_from_list(_list, structure, member)     \
        container_of(_list, structure, member)          

#define extract_next_from_list(_list, structure, member)    \
        extract_from_list((_list)->next, structure, member) 

#define list_for_each(_list, head)  \
    for(_list = (head); _list != NULL; _list = _list->next)


#endif