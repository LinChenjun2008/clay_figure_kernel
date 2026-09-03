// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <mem/allocator.h>
#include <syscall/cap.h>

// 从父 entry 的 child 链表中摘除
static void cap_unlink_lock(struct cap_slot_entry *entry)
{
    struct cap_slot_entry *parent = entry->parent;
    if (parent == NULL)
    {
        return;
    }
    if (parent->child == entry)
    {
        parent->child = entry->next;
        return;
    }
    struct cap_slot_entry *prev = parent->child;
    while (prev != NULL && prev->next != entry)
    {
        prev = prev->next;
    }
    if (prev != NULL)
    {
        prev->next = entry->next;
    }
    return;
}

// 统计 entry 派生子树规模(含自身)
static int cap_count(struct cap_slot_entry *entry)
{
    if (entry == NULL)
    {
        return 0;
    }
    int                    n       = 1;
    struct cap_slot_entry *derived = entry->child;
    while (derived != NULL)
    {
        struct cap_slot_entry *sibling = derived->next;
        n += cap_count(derived);
        derived = sibling;
    }
    return n;
}

// 收集 entry 的派生子树(含自身)到数组, 深度优先(假设已持根 cnode 锁)
static void cap_collect(
    struct cap_slot_entry  *entry,
    struct cap_slot_entry **list,
    int                    *count
)
{
    if (entry == NULL)
    {
        return;
    }
    list[(*count)++]               = entry;
    struct cap_slot_entry *derived = entry->child;
    while (derived != NULL)
    {
        struct cap_slot_entry *sibling = derived->next;
        cap_collect(derived, list, count);
        derived = sibling;
    }
    return;
}

// 删除单个 entry: 经反向指针定位槽位(支持跨 cnode)
static void cap_destroy_entry(struct cap_slot_entry *entry)
{
    struct cap_node       *owner  = entry->owner;
    struct cap_slot_entry *parent = entry->parent;

    // 同一 cnode: unlink + 清槽位在同一把锁内
    spin_lock(&owner->head.lock);
    if (parent != NULL && parent->owner == owner)
    {
        cap_unlink_lock(entry);
    }
    struct cap_slot *slot = &owner->slots[entry->slot_id - 1];
    slot->entry           = NULL;
    slot->key_seed++;
    spin_unlock(&owner->head.lock);

    // 父跨 cnode: 单独锁父 owner 摘链(父仍存活)
    if (parent != NULL && parent->owner != owner)
    {
        struct cap_node *powner = parent->owner;
        spin_lock(&powner->head.lock);
        cap_unlink_lock(entry);
        spin_unlock(&powner->head.lock);
    }

    // 对象引用计数 -1
    struct cap_head *head = entry->head;
    if (head != NULL)
    {
        spin_lock(&head->lock);
        head->reference_count--;
        spin_unlock(&head->lock);
    }
    kfree(entry);
    return;
}

// 撤销: 撤销 handle 对应的 cap 及其全部派生(继承撤销, 支持跨 cnode)
int cap_revoke(struct cap_node *cnode, cap_handle_t handle)
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
    spin_lock(&cnode->head.lock);
    struct cap_slot_entry *root = cnode->slots[slot_id - 1].entry;
    if (root == NULL || key != root->key)
    {
        goto fail;
    }
    ret = 0;

    // 先统计派生规模, 再动态分配收集数组
    int                     total = cap_count(root);
    struct cap_slot_entry **list  = kmalloc(sizeof(*list) * total, 0, 0);
    if (list == NULL)
    {
        goto fail;
    }

    // 锁内收集整棵派生子树(可能跨 cnode)
    int count = 0;
    cap_collect(root, list, &count);
    spin_unlock(&cnode->head.lock);

    // 逆序删除(先叶子后根, 保证父链仍有效)
    int i;
    for (i = count - 1; i >= 0; i--)
    {
        cap_destroy_entry(list[i]);
    }
    kfree(list);
    return ret;
fail:
    spin_unlock(&cnode->head.lock);
    return ret;
}

// 删除单个 cap(不撤销派生子树): 释放 entry/slot + dec 引用计数
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
    spin_lock(&cnode->head.lock);
    struct cap_slot       *slot       = &cnode->slots[slot_id - 1];
    struct cap_slot_entry *slot_entry = slot->entry;
    if (slot_entry == NULL || key != slot_entry->key)
    {
        goto fail;
    }
    struct cap_head *head = slot_entry->head;
    if (head == NULL)
    {
        goto fail;
    }

    slot->entry = NULL;
    slot->key_seed++;
    kfree(slot_entry);

    // 返回值: 0=正常删除, 1=对象引用计数归零(需销毁对象)
    spin_lock(&head->lock);
    head->reference_count--;
    ret = (head->reference_count == 0) ? 1 : 0;
    spin_unlock(&head->lock);

fail:
    spin_unlock(&cnode->head.lock);
    return ret;
}
