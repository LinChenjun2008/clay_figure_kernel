#include <kernel/global.h>
#include <lib/list.h>
#include <intr.h>

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
    intr_status_t intr_status= intr_disable();

    in_before->prev->next = node;

    node->prev = in_before->prev;
    node->next = in_before;

    in_before->prev = node;

    intr_set_status(intr_status);
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
    intr_status_t intr_status = intr_disable();
    node->prev->next = node->next;
    node->next->prev = node->prev;
    intr_set_status(intr_status);
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
    while (node != &(list->tail))
    {
        if (node == objnode)
        {
            return TRUE;
        }
        node = node->next;
    }
    return FALSE;
}

PUBLIC list_node_t* list_traversal(list_t *list,func function,int arg)
{
    list_node_t *node = list->head.next;
    while (node != &list->tail)
    {
        if (function(node,arg))
        {
            return node;
        }
        node = node->next;
    }
    return NULL;
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