/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.

Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU Lesser General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 宽通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 宽通用公共许可证，了解详情。

你应该随程序获得一份 GNU 宽通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

#ifndef __LIST_H__
#define __LIST_H__

#define OFFSET(CONTAINER_TYPE, MEMBER_NAME) \
    (uint64_t)(&((CONTAINER_TYPE*)0)->MEMBER_NAME)

#define CONTAINER_OF(CONTAINER_TYPE,MEMBER_NAME,MEMBER_PTR) \
    ((CONTAINER_TYPE*)((addr_t)MEMBER_PTR - OFFSET(CONTAINER_TYPE, MEMBER_NAME)))

typedef struct list_node_s list_node_t;

typedef struct list_node_s
{
    list_node_t *prev;
    list_node_t *next;
} list_node_t;

typedef struct list_s
{
    list_node_t head;
    list_node_t tail;
} list_t;

typedef bool (func) (list_node_t *,uint64_t);

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

PUBLIC list_node_t* list_next(list_node_t *node);
PUBLIC list_node_t* list_prev(list_node_t* node);

#endif