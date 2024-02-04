#ifndef __LIST_H__
#define __LIST_H__

typedef struct _list_node_t list_node_t;

#define OFFSET(CONTAINER_TYPE, MEMBER_NAME) \
    (uint64_t)(&((CONTAINER_TYPE*)0)->MEMBER_NAME)

#define CONTAINER_OF(CONTAINER_TYPE,MEMBER_NAME,MEMBER_PTR) \
    ((CONTAINER_TYPE*)((uintptr_t)MEMBER_PTR - OFFSET(CONTAINER_TYPE, MEMBER_NAME)))

struct _list_node_t
{
    list_node_t *prev;
    list_node_t *next;
};

typedef struct
{
    list_node_t head;
    list_node_t tail;
} list_t;

typedef bool (func) (list_node_t * ,wordsize_t arg);

PUBLIC void list_init(list_t *list);
PUBLIC void list_in(list_node_t *node,list_node_t *in_before);
PUBLIC void list_push(list_t *list,list_node_t *node);
PUBLIC void list_append(list_t *list,list_node_t *node);
PUBLIC void list_remove(list_node_t *node);
PUBLIC list_node_t* list_pop(list_t *list);
PUBLIC bool list_find(list_t *list,list_node_t *objnode);
PUBLIC list_node_t* list_traversal(list_t *list,func function,int arg);
PUBLIC int list_len(list_t *list);
PUBLIC bool list_empty(list_t *list);

#endif