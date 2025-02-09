/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <lib/list.h>

PUBLIC void list_init(list_t *list)
{
    list->head.prev = NULL;
    list->head.next = &list->tail;
    list->tail.prev = &list->head;
    list->tail.next = NULL;
    return;
}

PUBLIC void list_in(list_node_t *node,list_node_t *in_before)
{
    in_before->prev->next = node;

    node->prev = in_before->prev;
    node->next = in_before;

    in_before->prev = node;
    return;
}

PUBLIC void list_push(list_t *list,list_node_t *node)
{
    list_in(node,list->head.next);
    return;
}

PUBLIC void list_append(list_t *list,list_node_t *node)
{
    list_in(node,&list->tail);
    return;
}

PUBLIC void list_remove(list_node_t *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next = NULL;
    node->prev = NULL;
    return;
}

PUBLIC list_node_t* list_pop(list_t *list)
{
    list_node_t *node = list->head.next;
    list_remove(node);
    return node;
}

PUBLIC bool list_find(list_t *list,list_node_t *objnode)
{
    list_node_t *node = list->head.next;
    bool res = FALSE;
    while (node != &(list->tail))
    {
        if (node == objnode)
        {
            res = TRUE;
            break;
        }
        node = node->next;
    }
    return res;
}

PUBLIC list_node_t* list_traversal(list_t *list,func function,int arg)
{
    list_node_t *node = list->head.next;
    list_node_t *res = NULL;
    while (node != &list->tail)
    {
        if (function(node,arg))
        {
            res = node;
            break;
        }
        node = node->next;
    }
    return res;
}

PUBLIC int list_len(list_t *list)
{
    list_node_t *node = list->head.next;
    int len = 0;
    while (node != &list->tail)
    {
        len++;
        node = node->next;
    }
    return len;
}

PUBLIC bool list_empty(list_t *list)
{
    return list->head.next == &list->tail;
}

PUBLIC list_node_t* list_next(list_node_t *node)
{
    return node->next;
}

PUBLIC list_node_t* list_prev(list_node_t *node)
{
    return node->prev;
}