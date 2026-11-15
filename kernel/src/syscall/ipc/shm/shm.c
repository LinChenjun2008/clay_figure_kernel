// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <errno.h>
#include <lib/string.h>
#include <mem/allocator.h>
#include <mem/page.h>
#include <panic.h>
#include <syscall/ipc/shm.h>
#include <task.h>
#include <task/struct.h>

// shmflg 中属于权限位的部分
#define SHM_MODE_MASK 0777

// shmid = (gen << 16) + (slot_index + 1)
#define SHM_ID_SLOT_SHIFT 0
#define SHM_ID_SLOT_MASK  0xffff
#define SHM_ID_GEN_SHIFT  16
#define SHM_ID_GEN_MASK   0x7fff

struct shm_table shm_table;

// 代际在 15 位内回绕, 与 shmid 里 gen 字段的宽度保持一致
static int shm_gen_next(int gen)
{
    return (gen + 1) & SHM_ID_GEN_MASK;
}

static int shm_id_encode(int slot, int gen)
{
    int id = 0;
    id |= SET_FIELD(0, SHM_ID_SLOT, slot + 1);
    id |= SET_FIELD(0, SHM_ID_GEN, gen);
    ASSERT(id > 0);
    return id;
}

// 解码, 供 shmat / shmdt / shmctl 使用
static inline int shm_id_slot(int shmid)
{
    return GET_FIELD(shmid, SHM_ID_SLOT) - 1;
}

static inline int shm_id_gen(int shmid)
{
    return GET_FIELD(shmid, SHM_ID_GEN);
}

void shm_init(void)
{
    init_spinlock(&shm_table.lock);
    memset(shm_table.slot, 0, sizeof(shm_table.slot));
    int i;
    for (i = 0; i < SHMMNI; i++)
    {
        shm_table.gen[i] = 0;
    }
    return;
}

// 销毁一个shm段,同步释放对应的物理页
static void destroy_shm_seg(struct kobject *kobj)
{
    struct shm_seg *seg = CONTAINER_OF(struct shm_seg, kobj, kobj);

    size_t i;
    for (i = 0; i < seg->pages; i++)
    {
        if (seg->phys[i] == 0)
        {
            continue;
        }
        free_a_page(seg->phys[i]);
    }

    kfree(seg->phys);
    kfree(seg);
    return;
}

// 将key转为已存在的槽位下标,-1为找不到重复key
static int key_to_slot_locked(int key)
{
    if (key == IPC_PRIVATE)
    {
        return -1;
    }
    int i;
    for (i = 0; i < SHMMNI; i++)
    {
        if (shm_table.slot[i] == NULL)
        {
            continue;
        }
        if (shm_table.slot[i]->key == key)
        {
            break;
        }
    }
    return i == SHMMNI ? -1 : i;
}

static int init_shm_seg(struct shm_seg *seg, size_t size, int key, int mode)
{
    size_t pages = DIV_ROUND_UP(size, PG_SIZE);

    seg->phys = kmalloc(sizeof(seg->phys[0]) * pages, 0, 0);
    if (seg->phys == NULL)
    {
        return -ENOMEM;
    }
    seg->pages = pages;
    memset(seg->phys, 0, sizeof(seg->phys[0]) * seg->pages);
    seg->key  = key;
    seg->mode = mode;

    /// TODO: 将seg->phys改为懒分配.
    size_t phys_index;
    for (phys_index = 0; phys_index < pages; phys_index++)
    {
        phys_addr_t phys = allocate_a_page();
        if (phys == 0)
        {
            break;
        }
        memset(PHYS_TO_VIRT(phys), 0, PG_SIZE);
        seg->phys[phys_index] = phys;
    }
    if (phys_index < pages)
    {
        return -ENOMEM;
    }
    return 0;
}

// 创建一个新的shm段
int create_shm_seg(size_t size, int key, uint32_t mode)
{
    if (size == 0 || size > SHMMAX)
    {
        return -EINVAL;
    }
    int             status = 0;
    struct shm_seg *seg    = kmalloc(sizeof(*seg), 0, 0);
    if (seg == NULL)
    {
        return -ENOMEM;
    }
    memset(seg, 0, sizeof(*seg));
    init_kobject(&seg->kobj, destroy_shm_seg);

    status = init_shm_seg(seg, size, key, mode);
    if (status < 0)
    {
        goto end;
    }
    seg->cpid = get_current_task()->pid;

    status   = -ENOSPC;
    int slot = -1;

    spin_lock(&shm_table.lock);

    if (key_to_slot_locked(seg->key) >= 0)
    {
        status = -EEXIST;
    }
    else
    {
        int i;
        for (i = 0; i < SHMMNI; i++)
        {
            // 空槽位则插入, 并进入新的代际
            if (shm_table.slot[i] != NULL)
            {
                continue;
            }
            shm_table.slot[i] = seg;
            shm_table.gen[i]  = shm_gen_next(shm_table.gen[i]);
            slot              = i;
            status            = shm_id_encode(slot, shm_table.gen[slot]);
            break;
        }
    }

    spin_unlock(&shm_table.lock);

    if (status < 0)
    {
        goto end;
    }
    return status;

end:
    put_kobject(&seg->kobj);
    return status;
}

int shm_get(size_t size, int key, int shmflg)
{
    if (size == 0 || size > SHMMAX)
    {
        return -EINVAL;
    }

    int      create = ((shmflg & IPC_CREAT) != 0) || key == IPC_PRIVATE;
    int      excl   = (shmflg & IPC_EXCL) != 0;
    uint32_t mode   = (uint32_t)(shmflg & SHM_MODE_MASK);

    // IPC_PRIVATE 每次都是新段, 不参与 key 查找
    if (key == IPC_PRIVATE)
    {
        return create_shm_seg(size, key, mode);
    }

    while (1)
    {
        int    shmid    = -1;
        size_t seg_size = 0;

        spin_lock(&shm_table.lock);
        int slot = key_to_slot_locked(key);
        if (slot >= 0)
        {
            shmid    = shm_id_encode(slot, shm_table.gen[slot]);
            seg_size = shm_table.slot[slot]->pages * PG_SIZE;
        }
        spin_unlock(&shm_table.lock);

        if (shmid >= 0)
        {
            if (create && excl)
            {
                return -EEXIST;
            }
            // 已有段比请求的小, 不能直接返回(调用方会越界写)
            if (size > seg_size)
            {
                return -EINVAL;
            }
            return shmid;
        }
        if (!create)
        {
            return -ENOENT;
        }

        int id = create_shm_seg(size, key, mode);
        if (id != -EEXIST)
        {
            return id;
        }
        // 其他任务抢先创建了同 key 的段, 回到循环按"已存在"处理
    }
    return 0;
}
