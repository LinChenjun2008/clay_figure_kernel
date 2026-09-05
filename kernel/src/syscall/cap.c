// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <mem/allocator.h>
#include <syscall/cap.h>

struct cap_node *create_root_cap_node(void)
{
    struct cap_node *cnode = kmalloc(sizeof(*cnode), 0, 0);
    if (cnode == NULL)
    {
        return 0;
    }
    cnode->head.type            = CAP_NODE;
    cnode->head.reference_count = 1;
    init_spinlock(&cnode->head.lock);
    int i;
    for (i = 0; i < CAP_SLOTS; i++)
    {
        cnode->slots[i].entry    = NULL;
        cnode->slots[i].key_seed = 0;
    }
    return cnode;
}

// 分配槽位并初始化 entry(假设已持 cnode->head.lock)
static cap_handle_t cap_allocate_slot_lock(struct cap_node *cnode)
{
    int i;
    for (i = 0; i < CAP_SLOTS; i++)
    {
        if (cnode->slots[i].entry != NULL)
        {
            continue;
        }
        break;
    }
    if (i == CAP_SLOTS)
    {
        return 0;
    }
    struct cap_slot_entry *slot_entry = kmalloc(sizeof(*slot_entry), 0, 0);
    if (slot_entry == NULL)
    {
        return 0;
    }
    cap_handle_t handle = CAP_HANDLE(i + 1, cnode->slots[i].key_seed++);

    cnode->slots[i].entry = slot_entry;
    slot_entry->head      = NULL;
    slot_entry->key       = CAP_HANDLE_GET_KEY(handle);
    slot_entry->rights    = 0;
    return handle;
}

cap_handle_t
cap_insert(struct cap_node *cnode, struct cap_head *head, uint32_t rights)
{
    if (head == NULL || cnode->head.type != CAP_NODE)
    {
        return 0;
    }
    cap_handle_t handle = 0;

    spin_lock_double(&cnode->head.lock, &head->lock);
    handle = cap_allocate_slot_lock(cnode);
    if (handle != 0)
    {
        uint32_t               slot_id    = CAP_HANDLE_GET_SLOT_ID(handle);
        struct cap_slot_entry *slot_entry = cnode->slots[slot_id - 1].entry;
        slot_entry->head                  = head;
        slot_entry->rights                = rights;

        // 对象引用 +1(head 可能即当前 cnode, spin_lock_double 已防自锁)
        head->reference_count++;
    }
    spin_unlock_double(&cnode->head.lock, &head->lock);
    return handle;
}

// 解析 handle: 校验 slot 存在 + key 匹配 + 权限满足, 返回对象 head(或 NULL)
struct cap_head *
cap_lookup(struct cap_node *cnode, cap_handle_t handle, uint32_t rights)
{
    if (cnode->head.type != CAP_NODE)
    {
        return NULL;
    }
    uint32_t key     = CAP_HANDLE_GET_KEY(handle);
    uint32_t slot_id = CAP_HANDLE_GET_SLOT_ID(handle);

    if (slot_id == 0 || slot_id > CAP_SLOTS)
    {
        return NULL;
    }
    struct cap_head *ret = NULL;
    spin_lock(&cnode->head.lock);
    struct cap_slot       *slot       = &cnode->slots[slot_id - 1];
    struct cap_slot_entry *slot_entry = slot->entry;
    if (slot_entry == NULL)
    {
        goto fail;
    }
    if (key != slot_entry->key)
    {
        goto fail;
    }
    if (rights & ~slot_entry->rights)
    {
        goto fail;
    }
    if (slot_entry->head == NULL)
    {
        goto fail;
    }
    ret = slot_entry->head;
fail:
    spin_unlock(&cnode->head.lock);
    return ret;
}

// 复制: 在 dst_cnode 创建指向同一对象的独立 cap(对象引用 +1)
cap_handle_t cap_copy(
    struct cap_node *src_cnode,
    cap_handle_t     src_handle,
    struct cap_node *dst_cnode,
    uint32_t         rights
)
{
    cap_handle_t handle = 0;
    if (src_cnode->head.type != CAP_NODE || dst_cnode->head.type != CAP_NODE)
    {
        return 0;
    }
    uint32_t key     = CAP_HANDLE_GET_KEY(src_handle);
    uint32_t slot_id = CAP_HANDLE_GET_SLOT_ID(src_handle);
    if (slot_id == 0 || slot_id > CAP_SLOTS)
    {
        return 0;
    }
    spin_lock_double(&src_cnode->head.lock, &dst_cnode->head.lock);
    struct cap_slot_entry *src = src_cnode->slots[slot_id - 1].entry;
    if (src == NULL || key != src->key || src->head == NULL)
    {
        goto fail;
    }
    // 需持有源的 CTRL(控制权)才能复制
    if (!(src->rights & CAP_CTRL))
    {
        goto fail;
    }
    // 复制权限必须小于源权限
    if (rights & ~src->rights)
    {
        goto fail;
    }

    handle = cap_allocate_slot_lock(dst_cnode);
    if (handle != 0)
    {
        uint32_t               dst_slot_id = CAP_HANDLE_GET_SLOT_ID(handle);
        struct cap_slot_entry *dst = dst_cnode->slots[dst_slot_id - 1].entry;
        dst->head                  = src->head;
        dst->rights                = rights;

        // 对象引用 +1(对象可能即 src/dst cnode, 已在双锁内直接递增,
        // 否则需单独锁对象头)
        struct cap_head *head = src->head;
        if (head == &src_cnode->head || head == &dst_cnode->head)
        {
            head->reference_count++;
        }
        else
        {
            spin_lock(&head->lock);
            head->reference_count++;
            spin_unlock(&head->lock);
        }
    }
fail:
    spin_unlock_double(&src_cnode->head.lock, &dst_cnode->head.lock);
    return handle;
}

// 删除单个 cap: 释放 entry/slot + dec 引用计数
int cap_delete(struct cap_node *cnode, cap_handle_t handle)
{
    int ret = -1;
    if (cnode->head.type != CAP_NODE)
    {
        return -1;
    }
    uint32_t key     = CAP_HANDLE_GET_KEY(handle);
    uint32_t slot_id = CAP_HANDLE_GET_SLOT_ID(handle);

    if (slot_id == 0 || slot_id > CAP_SLOTS)
    {
        return -1;
    }
    struct cap_head *head = NULL;

    // 阶段一: 单锁解析对象 head(槽位可能并发变化, 阶段二会复核)
    spin_lock(&cnode->head.lock);
    struct cap_slot_entry *slot_entry = cnode->slots[slot_id - 1].entry;
    if (slot_entry != NULL && key == slot_entry->key)
    {
        head = slot_entry->head;
    }
    spin_unlock(&cnode->head.lock);
    if (head == NULL)
    {
        return -1;
    }

    // 阶段二: 双锁原子执行删除(head 可能即当前 cnode, spin_lock_double 已防自锁)
    spin_lock_double(&cnode->head.lock, &head->lock);
    struct cap_slot *slot = &cnode->slots[slot_id - 1];
    slot_entry            = slot->entry;
    if (slot_entry == NULL || key != slot_entry->key ||
        slot_entry->head != head)
    {
        ret = -1;
    }
    else
    {
        slot->entry = NULL;
        slot->key_seed++;
        kfree(slot_entry);

        // 返回值: 0=正常删除, 1=对象引用计数归零(需销毁对象)
        head->reference_count--;
        ret = (head->reference_count == 0) ? 1 : 0;
    }
    spin_unlock_double(&cnode->head.lock, &head->lock);
    return ret;
}
