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

    spin_lock(&cnode->head.lock);
    handle = cap_allocate_slot_lock(cnode);
    if (handle != 0)
    {
        uint32_t               slot_id    = CAP_HANDLE_GET_SLOT_ID(handle);
        struct cap_slot       *slot       = &cnode->slots[slot_id - 1];
        struct cap_slot_entry *slot_entry = slot->entry;
        slot_entry->head                  = head;
        slot_entry->rights                = rights;

        // 对象引用 +1(若对象即当前 cnode 已在锁内, 避免重锁)
        if (head == &cnode->head)
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
    spin_unlock(&cnode->head.lock);
    return handle;
}

// 解析 handle: 校验 slot 存在 + key 匹配 + 权限满足, 返回 entry(或 NULL)
struct cap_slot_entry *
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
    struct cap_slot_entry *ret = NULL;
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
    ret = slot_entry;
fail:
    spin_unlock(&cnode->head.lock);
    return ret;
}

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

// ============ 派生 / 复制 / 移动 ============

// 按地址升序获取两个 cnode 的锁(统一锁序, 避免 AB-BA 死锁)
static void cap_lock_double(struct cap_node *a, struct cap_node *b)
{
    if (a == b)
    {
        spin_lock(&a->head.lock);
        return;
    }
    if ((uintptr_t)a < (uintptr_t)b)
    {
        spin_lock(&a->head.lock);
        spin_lock(&b->head.lock);
    }
    else
    {
        spin_lock(&b->head.lock);
        spin_lock(&a->head.lock);
    }
    return;
}

static void cap_unlock_double(struct cap_node *a, struct cap_node *b)
{
    if (a == b)
    {
        spin_unlock(&a->head.lock);
        return;
    }
    if ((uintptr_t)a < (uintptr_t)b)
    {
        spin_unlock(&b->head.lock);
        spin_unlock(&a->head.lock);
    }
    else
    {
        spin_unlock(&a->head.lock);
        spin_unlock(&b->head.lock);
    }
    return;
}

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
    cap_lock_double(src_cnode, dst_cnode);
    struct cap_slot_entry *parent = src_cnode->slots[slot_id - 1].entry;
    if (parent == NULL || key != parent->key || parent->head == NULL)
    {
        goto fail;
    }
    // 权限裁剪: 派生权限必须 ⊆ 父权限
    if (rights & ~parent->rights)
    {
        goto fail;
    }
    handle =
        cap_derive_lock(dst_cnode, parent, rights, 1, src_cnode, dst_cnode);
fail:
    cap_unlock_double(src_cnode, dst_cnode);
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
    cap_lock_double(src_cnode, dst_cnode);
    struct cap_slot_entry *src = src_cnode->slots[slot_id - 1].entry;
    if (src == NULL || key != src->key || src->head == NULL)
    {
        goto fail;
    }
    // 复制权限必须 ⊆ 源权限
    if (rights & ~src->rights)
    {
        goto fail;
    }
    handle = cap_derive_lock(dst_cnode, src, rights, 0, src_cnode, dst_cnode);
fail:
    cap_unlock_double(src_cnode, dst_cnode);
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
    cap_lock_double(src_cnode, dst_cnode);

    struct cap_slot       *src_slot = &src_cnode->slots[slot_id - 1];
    struct cap_slot_entry *src      = src_slot->entry;
    if (src == NULL || key != src->key)
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

    // 从父链摘除(父 owner 须已持锁或为 NULL)
    struct cap_slot_entry *parent = src->parent;
    if (parent != NULL)
    {
        struct cap_node *powner = parent->owner;
        if (powner == src_cnode || powner == dst_cnode)
        {
            cap_unlink_lock(src); // 已持该 cnode 锁
        }
        else
        {
            spin_lock(&powner->head.lock);
            cap_unlink_lock(src);
            spin_unlock(&powner->head.lock);
        }
    }

    // 挂到目标槽, 分配新 key
    uint32_t new_key          = dst_cnode->slots[j].key_seed++;
    src->owner                = dst_cnode;
    src->slot_id              = j + 1;
    src->key                  = new_key;
    dst_cnode->slots[j].entry = src;
    handle                    = CAP_HANDLE(j + 1, new_key);
fail:
    cap_unlock_double(src_cnode, dst_cnode);
    return handle;
}