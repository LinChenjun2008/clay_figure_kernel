/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.

Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 通用公共许可证，了解详情。

你应该随程序获得一份 GNU 通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

#include <kernel/global.h>
#include <lib/list.h>
#include <intr.h> // intr functions

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
    node->next = NULL;
    node->prev = NULL;
    intr_set_status(intr_status);
    return;
}

PUBLIC list_node_t* list_pop(list_t *list)
{
    intr_status_t intr_status = intr_disable();
    list_node_t *node = list->head.next;
    list_remove(node);
    intr_set_status(intr_status);
    return node;
}

PUBLIC bool list_find(list_t *list,list_node_t *objnode)
{
    intr_status_t intr_status = intr_disable();
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
    intr_set_status(intr_status);
    return res;
}

PUBLIC list_node_t* list_traversal(list_t *list,func function,int arg)
{
    intr_status_t intr_status = intr_disable();
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
    intr_set_status(intr_status);
    return res;
}

PUBLIC int list_len(list_t *list)
{
    intr_status_t intr_status = intr_disable();
    list_node_t *node = list->head.next;
    int len = 0;
    while (node != &list->tail)
    {
        len++;
        node = node->next;
    }
    intr_set_status(intr_status);
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