// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <errno.h>
#include <mem/allocator.h>
#include <panic.h>
#include <std/string.h>
#include <syscall/cap.h>
#include <syscall/cap/object.h>
#include <syscall/cap/struct.h>
#include <task.h>

static void destory_root_cap_node(struct cap_head *head)
{
    struct cap_node *cnode = (struct cap_node *)head;
    ASSERT(cnode->head.type == CAP_NODE);

    int i;
    for (i = 0; i < CAP_SLOTS; i++)
    {
        struct cap_slot *slot = &cnode->slots[i];
        if (slot->head == NULL)
        {
            continue;
        }
        ASSERT(slot->head != NULL);
        destory_cap(slot->head);
    }
    kfree(cnode);
    return;
}

void init_cap_head(
    struct cap_head *head,
    enum cap_type    type,
    struct cap_opt  *opt
)
{
    head->type = type;
    init_spinlock(&head->lock);
    head->reference_count = 1;
    head->opt             = *opt;
    return;
}

int32_t cap_reference(struct cap_head *head)
{
    if (head == NULL)
    {
        return 0;
    }
    spin_lock(&head->lock);
    ASSERT(head->reference_count >= 0);
    int32_t ref = head->reference_count++;
    spin_unlock(&head->lock);
    return ref;
}

int32_t cap_release(struct cap_head *head)
{
    if (head == NULL)
    {
        return 0;
    }
    spin_lock(&head->lock);
    int32_t ref = 0;
    if (head->reference_count > 0)
    {
        ref = head->reference_count--;
    }
    spin_unlock(&head->lock);
    return ref;
}

// 创建根cnode
struct cap_head *create_root_cap_node(void)
{
    struct cap_node *cnode = kmalloc(sizeof(*cnode), 0, 0);
    if (cnode == NULL)
    {
        return NULL;
    }
    memset(cnode, 0, sizeof(*cnode));
    struct cap_head *head = &cnode->head;
    struct cap_opt   opt;
    opt.destory = destory_root_cap_node;
    init_cap_head(head, CAP_NODE, &opt);
    int i;
    for (i = 0; i < CAP_SLOTS; i++)
    {
        struct cap_slot *slot = &cnode->slots[i];
        slot->head            = NULL;
        slot->key_seed        = 0;
        slot->key             = 0;
        slot->rights          = 0;
    }
    return head;
}

// 销毁cap,自动调用destory函数
void destory_cap(struct cap_head *head)
{
    if (head == NULL)
    {
        return;
    }
    int32_t ref = cap_release(head);
    if (ref == 1)
    {
        ASSERT(head->opt.destory != NULL);
        head->opt.destory(head);
    }
    return;
}

// 在cnode中分配一个槽,并插入head(不计算引用)
cap_handle_t
cap_insert(struct cap_head *node_head, struct cap_head *head, uint32_t rights)
{
    if (node_head == NULL || node_head->type != CAP_NODE)
    {
        return 0;
    }
    struct cap_node *cnode = (struct cap_node *)node_head;
    if (head == NULL)
    {
        return 0;
    }
    if (head->type == CAP_NODE)
    {
        return 0;
    }
    if (rights & ~(CAP_READ | CAP_WRITE | CAP_EXEC | CAP_CTRL))
    {
        return 0;
    }
    cap_handle_t handle = 0;
    spin_lock(&cnode->head.lock);
    uint16_t slot_id = 0;
    int      i;
    for (i = 0; i < CAP_SLOTS; i++)
    {
        struct cap_slot *slot = &cnode->slots[i];
        if (slot->head != NULL)
        {
            continue;
        }
        cap_reference(head);
        slot_id      = i + 1;
        slot->head   = head;
        slot->key    = slot->key_seed;
        slot->rights = rights;

        handle = CAP_HANDLE(slot_id, slot->key);
        break;
    }
    spin_unlock(&cnode->head.lock);
    if (slot_id == 0)
    {
        return 0;
    }
    return handle;
}

// 在cnode中删除cap
void cap_delete(struct cap_head *node_head, cap_handle_t handle)
{
    if (node_head == NULL || node_head->type != CAP_NODE)
    {
        return;
    }
    struct cap_node *cnode   = (struct cap_node *)node_head;
    uint16_t         key     = CAP_HANDLE_GET_KEY(handle);
    uint16_t         slot_id = CAP_HANDLE_GET_SLOT_ID(handle);
    if (slot_id <= 0 || slot_id > CAP_SLOTS)
    {
        return;
    }
    spin_lock(&cnode->head.lock);
    struct cap_slot *slot = &cnode->slots[slot_id - 1];
    struct cap_head *head = slot->head;

    int need_release = 1;

    if (key != slot->key)
    {
        need_release = 0;
        goto fail;
    }
    if (slot->head == NULL)
    {
        need_release = 0;
        goto fail;
    }
    slot->head = NULL;
    slot->key_seed++;
    slot->key    = 0;
    slot->rights = 0;

fail:
    spin_unlock(&cnode->head.lock);
    if (need_release)
    {
        cap_release(head);
    }

    return;
}

static struct cap_head *
cap_verify_reference(struct cap_slot *slot, uint16_t key, uint32_t rights)
{
    if (slot->head == NULL)
    {
        return NULL;
    }
    if (key != slot->key)
    {
        return NULL;
    }
    if (rights & ~slot->rights)
    {
        return NULL;
    }
    struct cap_head *head = slot->head;
    cap_reference(head);
    return head;
}

// 通过handle以right权限获取对应的cap_head
// 获取后自动增加引用
struct cap_head *
cap_lookup(struct cap_head *node_head, cap_handle_t handle, uint32_t rights)
{
    if (node_head == NULL)
    {
        return NULL;
    }
    if (node_head->type != CAP_NODE)
    {
        return NULL;
    }
    struct cap_node *cnode = (struct cap_node *)node_head;
    struct cap_head *head  = NULL;

    uint16_t key     = CAP_HANDLE_GET_KEY(handle);
    uint16_t slot_id = CAP_HANDLE_GET_SLOT_ID(handle);

    if (slot_id <= 0 || slot_id > CAP_SLOTS)
    {
        return NULL;
    }
    spin_lock(&cnode->head.lock);

    struct cap_slot *slot = &cnode->slots[slot_id - 1];

    // 验证key和right的有效性,通过检查则增加引用.
    head = cap_verify_reference(slot, key, rights);

    spin_unlock(&cnode->head.lock);

    return head;
}

// 原样复制 cnode,用于 fork 继承, 保证父子 handle 一致。
int copy_cnode(struct cap_head *dst, struct cap_head *src)
{
    if (dst == NULL || src == NULL)
    {
        return -1;
    }
    if (dst->type != CAP_NODE || src->type != CAP_NODE)
    {
        return -1;
    }
    if (dst == src)
    {
        return -1;
    }
    struct cap_node *dst_node = (struct cap_node *)dst;
    struct cap_node *src_node = (struct cap_node *)src;

    spin_lock_double(&src->lock, &dst->lock);

    // 逐槽原样复制, 对象引用 +1
    int i;
    for (i = 0; i < CAP_SLOTS; i++)
    {
        struct cap_slot *sslot = &src_node->slots[i];
        struct cap_slot *dslot = &dst_node->slots[i];

        dslot->head     = sslot->head;
        dslot->key_seed = sslot->key_seed;
        dslot->key      = sslot->key;
        dslot->rights   = sslot->rights;

        if (sslot->head != NULL)
        {
            cap_reference(sslot->head);
        }
    }
    spin_unlock_double(&src->lock, &dst->lock);
    return 0;
}
