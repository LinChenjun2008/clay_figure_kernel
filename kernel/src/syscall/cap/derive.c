// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <mem/allocator.h>
#include <syscall/cap.h>

// 假设已持 dst_cnode 锁(held_a/held_b 为已持锁的 cnode):
// 在 dst_cnode 创建指向 parent 对象的 cap; derive=1 建派生树, derive=0 独立副本
static cap_handle_t cap_derive_lock(
    struct cap_node       *dst_cnode,
    struct cap_slot_entry *parent,
    uint32_t               rights,
    int                    derive,
    struct cap_node       *held_a,
    struct cap_node       *held_b
)
{
    cap_handle_t handle = cap_allocate_slot_lock(dst_cnode);
    if (handle == 0)
    {
        return 0;
    }
    uint32_t               slot_id    = CAP_HANDLE_GET_SLOT_ID(handle);
    struct cap_slot_entry *slot_entry = dst_cnode->slots[slot_id - 1].entry;
    slot_entry->head                  = parent->head;
    slot_entry->rights                = rights;

    if (derive)
    {
        // 挂派生树: 新 entry 作为父的直接派生(头插)
        slot_entry->parent = parent;
        slot_entry->next   = parent->child;
        parent->child      = slot_entry;
    }

    // 对象引用 +1(若对象即已持锁的 cnode, 避免重锁)
    struct cap_head *head = parent->head;
    if (head == &held_a->head || head == &held_b->head)
    {
        head->reference_count++;
    }
    else
    {
        spin_lock(&head->lock);
        head->reference_count++;
        spin_unlock(&head->lock);
    }
    return handle;
}

// 派生: 在 dst_cnode 创建父 cap 的受限权限派生(建派生树, 撤销父时子被继承撤销)
cap_handle_t cap_derive(
    struct cap_node *src_cnode,
    cap_handle_t     parent_handle,
    struct cap_node *dst_cnode,
    uint32_t         rights
)
{
    cap_handle_t handle = 0;
    if (src_cnode->head.type != CAP_NODE || dst_cnode->head.type != CAP_NODE)
    {
        return 0;
    }
    uint32_t key     = CAP_HANDLE_GET_KEY(parent_handle);
    uint32_t slot_id = CAP_HANDLE_GET_SLOT_ID(parent_handle);
    if (slot_id == 0 || slot_id > CAP_SLOTS)
    {
        return 0;
    }
    spin_lock_double(&src_cnode->head.lock, &dst_cnode->head.lock);
    struct cap_slot_entry *parent = src_cnode->slots[slot_id - 1].entry;
    if (parent == NULL || key != parent->key || parent->head == NULL)
    {
        goto fail;
    }
    // 需持有父的 CTRL(控制权)才能派生
    if (!(parent->rights & CAP_CTRL))
    {
        goto fail;
    }
    // 权限裁剪: 派生权限必须小于父权限
    if (rights & ~parent->rights)
    {
        goto fail;
    }
    handle =
        cap_derive_lock(dst_cnode, parent, rights, 1, src_cnode, dst_cnode);
fail:
    spin_unlock_double(&src_cnode->head.lock, &dst_cnode->head.lock);
    return handle;
}

// 复制: 在 dst_cnode 创建指向同一对象的独立 cap(不建派生树, 对象引用 +1)
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
    handle = cap_derive_lock(dst_cnode, src, rights, 0, src_cnode, dst_cnode);
fail:
    spin_unlock_double(&src_cnode->head.lock, &dst_cnode->head.lock);
    return handle;
}

// 移动: 把 src_cnode 的 cap 转移到 dst_cnode(源槽清空, 对象引用不变, 复用 entry)
cap_handle_t cap_move(
    struct cap_node *src_cnode,
    cap_handle_t     src_handle,
    struct cap_node *dst_cnode
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

    struct cap_slot       *src_slot = &src_cnode->slots[slot_id - 1];
    struct cap_slot_entry *src      = src_slot->entry;
    if (src == NULL || key != src->key)
    {
        goto fail;
    }
    // 需持有源的 CTRL(控制权)才能移动
    if (!(src->rights & CAP_CTRL))
    {
        goto fail;
    }
    // 目标找空槽
    int j;
    for (j = 0; j < CAP_SLOTS; j++)
    {
        if (dst_cnode->slots[j].entry == NULL)
        {
            break;
        }
    }
    if (j == CAP_SLOTS)
    {
        goto fail;
    }

    // 从源槽取出(旧 handle 失效)
    src_slot->entry = NULL;
    src_slot->key_seed++;

    // 保留派生关系: 不摘除父链(move 只改 owner/slot_id,
    // 父撤销仍能经本 entry 的反向指针定位, 避免锁第三 cnode)

    // 挂到目标槽, 分配新 key
    uint32_t new_key          = dst_cnode->slots[j].key_seed++;
    src->owner                = dst_cnode;
    src->slot_id              = j + 1;
    src->key                  = new_key;
    dst_cnode->slots[j].entry = src;
    handle                    = CAP_HANDLE(j + 1, new_key);
fail:
    spin_unlock_double(&src_cnode->head.lock, &dst_cnode->head.lock);
    return handle;
}
