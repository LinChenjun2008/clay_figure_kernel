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

// 假设已持 cnode->head.lock: 分配槽位并初始化 entry(跨文件共享给 derive)
cap_handle_t cap_allocate_slot_lock(struct cap_node *cnode)
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
    slot_entry->owner     = cnode;
    slot_entry->slot_id   = i + 1;
    slot_entry->head      = NULL;
    slot_entry->key       = CAP_HANDLE_GET_KEY(handle);
    slot_entry->rights    = 0;
    slot_entry->parent    = NULL;
    slot_entry->next      = NULL;
    slot_entry->child     = NULL;
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
        struct cap_slot       *slot       = &cnode->slots[slot_id - 1];
        struct cap_slot_entry *slot_entry = slot->entry;
        slot_entry->head                  = head;
        slot_entry->rights                = rights;

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
