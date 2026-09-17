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

// slots_map 的 bit i 表示 table[i] 已分配
#define CAP_TABLE_BIT(X) ((uint64_t)1 << (X))

static inline uint16_t cap_slot_to_table(uint16_t slot_id)
{
    return (uint16_t)((slot_id - 1) / CAP_SLOTS);
}

static inline uint16_t cap_slot_to_index(uint16_t slot_id)
{
    return (uint16_t)((slot_id - 1) % CAP_SLOTS);
}

static inline uint16_t cap_make_slot_id(uint16_t table_id, uint16_t slot_idx)
{
    return (uint16_t)(table_id * CAP_SLOTS + slot_idx + 1);
}

static void destory_root_cap_node(struct cap_head *head)
{
    struct cap_node *cnode = (struct cap_node *)head;
    ASSERT(cnode->head.type == CAP_NODE);

    if (cnode->slots_map == 0)
    {
        goto end;
    }
    uint16_t table_id;
    for (table_id = 0; table_id < CAP_TABLE_NR; table_id++)
    {
        if (!(cnode->slots_map & CAP_TABLE_BIT(table_id)))
        {
            continue;
        }
        struct cap_slot_table *table = cnode->table[table_id];

        uint16_t slot_idx;
        for (slot_idx = 0; slot_idx < CAP_SLOTS; slot_idx++)
        {
            if (table->slots[slot_idx].head != NULL)
            {
                destory_cap(table->slots[slot_idx].head);
            }
        }
        kfree(table);
    }
end:
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

// 分配 table[idx] 并将 slots_map 对应位置 1
static struct cap_slot_table *
cap_table_alloc(struct cap_node *cnode, uint16_t table_id)
{
    struct cap_slot_table *table = kmalloc(sizeof(*table), 0, 0);
    if (table == NULL)
    {
        return NULL;
    }
    uint16_t slot_idx;
    for (slot_idx = 0; slot_idx < CAP_SLOTS; slot_idx++)
    {
        table->slots[slot_idx].head   = NULL;
        table->slots[slot_idx].key    = 0;
        table->slots[slot_idx].rights = 0;
    }
    table->count           = 0;
    cnode->table[table_id] = table;
    cnode->slots_map |= CAP_TABLE_BIT(table_id);
    return table;
}

// 释放 table[id] 并清除 slots_map 对应位
static void cap_table_free(struct cap_node *cnode, uint16_t table_id)
{
    kfree(cnode->table[table_id]);
    cnode->table[table_id] = NULL;
    cnode->slots_map &= ~CAP_TABLE_BIT(table_id);
    return;
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
    cnode->slots_map      = 0;
    uint16_t table_id;
    for (table_id = 0; table_id < CAP_TABLE_NR; table_id++)
    {
        cnode->table[table_id] = NULL;
    }
    struct cap_opt opt;
    opt.destory = destory_root_cap_node;
    init_cap_head(head, CAP_NODE, &opt);
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
    cap_handle_t handle  = 0;
    uint16_t     slot_id = 0;
    uint16_t     table_id;
    uint16_t     slot_idx;

    spin_lock(&cnode->head.lock);

    for (table_id = 0; table_id < CAP_TABLE_NR; table_id++)
    {
        if (!(cnode->slots_map & CAP_TABLE_BIT(table_id)))
        {
            continue;
        }
        struct cap_slot_table *table = cnode->table[table_id];
        if (table->count >= CAP_SLOTS)
        {
            continue;
        }
        for (slot_idx = 0; slot_idx < CAP_SLOTS; slot_idx++)
        {
            if (table->slots[slot_idx].head == NULL)
            {
                break;
            }
        }
        if (slot_idx < CAP_SLOTS)
        {
            slot_id = cap_make_slot_id(table_id, slot_idx);
            break;
        }
    }
    // 所有slot已满,分配新的slot_table
    if (slot_id == 0)
    {
        for (table_id = 0; table_id < CAP_TABLE_NR; table_id++)
        {
            if (!(cnode->slots_map & CAP_TABLE_BIT(table_id)))
            {
                break;
            }
        }
        if (table_id == CAP_TABLE_NR)
        {
            goto fail; // 已达上限
        }
        if (cap_table_alloc(cnode, table_id) == NULL)
        {
            goto fail;
        }
        slot_id = cap_make_slot_id(table_id, 0);
    }
    // 3) 由 slot_id 换算 table 下标与槽内 id, 占用槽位
    table_id = cap_slot_to_table(slot_id);
    slot_idx = cap_slot_to_index(slot_id);

    struct cap_slot_table *table = cnode->table[table_id];
    struct cap_slot       *slot  = &table->slots[slot_idx];
    cap_reference(head);
    slot->head   = head;
    slot->key    = cnode->key_seed++;
    slot->rights = rights;
    table->count++;
    handle = CAP_HANDLE(slot_id, slot->key);

fail:
    spin_unlock(&cnode->head.lock);
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

    if (slot_id <= 0 || slot_id > MAX_SLOTS)
    {
        return;
    }
    uint16_t table_id = cap_slot_to_table(slot_id);
    uint16_t slot_idx = cap_slot_to_index(slot_id);

    spin_lock(&cnode->head.lock);
    if (!(cnode->slots_map & CAP_TABLE_BIT(table_id)))
    {
        spin_unlock(&cnode->head.lock);
        return;
    }
    struct cap_slot_table *table = cnode->table[table_id];
    struct cap_slot       *slot  = &table->slots[slot_idx];
    struct cap_head       *head  = slot->head;

    if (head == NULL || key != slot->key)
    {
        spin_unlock(&cnode->head.lock);
        return;
    }
    slot->head   = NULL;
    slot->key    = 0;
    slot->rights = 0;
    table->count--;
    if (table->count == 0)
    {
        cap_table_free(cnode, table_id);
    }
    spin_unlock(&cnode->head.lock);

    cap_release(head);
    return;
}

static struct cap_head *
cap_get_verified(struct cap_slot *slot, uint16_t key, uint32_t rights)
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

    spin_lock(&cnode->head.lock);
    if (slot_id <= 0 || slot_id > MAX_SLOTS)
    {
        spin_unlock(&cnode->head.lock);
        return NULL;
    }
    uint16_t table_id = cap_slot_to_table(slot_id);
    if (!(cnode->slots_map & CAP_TABLE_BIT(table_id)))
    {
        spin_unlock(&cnode->head.lock);
        return NULL;
    }
    uint16_t         slot_idx = cap_slot_to_index(slot_id);
    struct cap_slot *slot     = &cnode->table[table_id]->slots[slot_idx];

    // 验证key和right的有效性,通过检查则增加引用.
    head = cap_get_verified(slot, key, rights);

    spin_unlock(&cnode->head.lock);

    return head;
}

// 原样复制 cnode,用于 fork 继承, 保证父子 handle 一致。
// 要求 dst 为空 cnode(如新建), 且复制期间 src 不被并发修改
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

    // 复制全局key种子, 保证 fork 后 key 演化一致
    dst_node->key_seed = src_node->key_seed;

    // 按 slots_map 逐 table 原样复制, 对象引用 +1
    uint16_t table_id;
    for (table_id = 0; table_id < CAP_TABLE_NR; table_id++)
    {
        if (!(src_node->slots_map & CAP_TABLE_BIT(table_id)))
        {
            continue;
        }
        struct cap_slot_table *src_table = src_node->table[table_id];
        struct cap_slot_table *dst_table = kmalloc(sizeof(*dst_table), 0, 0);
        if (dst_table == NULL)
        {
            // 已复制的 table 由调用方 destory_cap(dst) 回收
            spin_unlock_double(&src->lock, &dst->lock);
            return -1;
        }
        uint16_t slot_idx;
        for (slot_idx = 0; slot_idx < CAP_SLOTS; slot_idx++)
        {
            dst_table->slots[slot_idx] = src_table->slots[slot_idx];
            if (src_table->slots[slot_idx].head != NULL)
            {
                cap_reference(src_table->slots[slot_idx].head);
            }
        }
        dst_table->count          = src_table->count;
        dst_node->table[table_id] = dst_table;
        dst_node->slots_map |= CAP_TABLE_BIT(table_id);
    }
    spin_unlock_double(&src->lock, &dst->lock);
    return 0;
}
