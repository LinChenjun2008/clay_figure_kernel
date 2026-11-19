// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/page.h>

#include <errno.h>
#include <lib/string.h>
#include <mem.h>
#include <mem/allocator.h>
#include <mem/page.h>
#include <mem/struct.h>
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

// 按 shmid 找到段并持有引用(pin), 找到后必须 put_kobject 归还
static struct shm_seg *shm_pin(int shmid)
{
    int slot = shm_id_slot(shmid);
    if (slot < 0 || slot >= SHMMNI)
    {
        return NULL;
    }

    struct shm_seg *seg = NULL;

    // 锁序: shm_table.lock -> kobj.lock, 反向加锁会死锁
    spin_lock(&shm_table.lock);
    struct shm_seg *cur = shm_table.slot[slot];
    if (cur == NULL)
    {
        goto end;
    }
    if (shm_table.gen[slot] != shm_id_gen(shmid))
    {
        goto end;
    }
    if (get_kobject(&cur->kobj) <= 0)
    {
        goto end;
    }
    seg = cur;

end:
    spin_unlock(&shm_table.lock);

    return seg;
}

// 创建者总能附着, 其它任务按 mode 位判断
static int shm_access_check(struct task *task, struct shm_seg *seg, int ro)
{
    if (task->pid == seg->cpid)
    {
        return 1;
    }
    if (seg->mode == 0)
    {
        return 1;
    }
    int need = ro ? 0444 : 0666;
    return (seg->mode & need) == need;
}


// 把段映射进当前任务的地址空间.
// 成功返回用户虚拟地址; 失败返回负错误码, 调用方按 (intptr_t) 判断.
uintptr_t shm_at(int shmid, uintptr_t shmaddr, int shmflg)
{
    struct task *task = get_current_task();
    if (task->mm == NULL)
    {
        return (uintptr_t)-EPERM;
    }

    int ro = (shmflg & SHM_RDONLY) != 0;

    struct shm_seg *seg = shm_pin(shmid);
    if (seg == NULL)
    {
        return (uintptr_t)-EINVAL;
    }

    struct shm_area *area   = NULL;
    uintptr_t        virt   = 0;
    size_t           pages  = seg->pages;
    int              status = 0;

    if (!shm_access_check(task, seg, ro))
    {
        status = -EACCES;
        goto end;
    }

    // 1) 预留用户虚拟地址区间
    if (shmaddr != 0)
    {
        if ((shmaddr & (PG_SIZE - 1)) != 0)
        {
            status = -EINVAL;
            goto end;
        }
        if (mm_reserve_shared_range(task, shmaddr, pages) < 0)
        {
            status = -EINVAL;
            goto end;
        }
        virt = shmaddr;
    }
    else
    {
        virt = mm_allocate_high(task, pages);
        if (virt == 0)
        {
            status = -ENOMEM;
            goto end;
        }
    }

    // 2) 逐页建立页结构并映射.
    //    页结构登记在 mm->pg_map 中, 使 mm_unmap / mm_free_a_page 能找到它
    size_t i;
    for (i = 0; i < pages; i++)
    {
        struct page_struct *pg = kmalloc(sizeof(*pg), 0, 0);
        if (pg == NULL)
        {
            status = -ENOMEM;
            goto fail;
        }
        pg->pfn  = ADDR_TO_PFN(seg->phys[i]);
        pg->virt = virt + i * PG_SIZE;

        // 段自身持有的那份页引用之外, 每个附着再持有一份
        page_reference_inc(pg->pfn);
        list_append(&task->mm->pg_map.list, &pg->node);

        if (mm_map_shared(task, seg->phys[i], pg->virt, ro) >= 0)
        {
            flush_tlb(task, (void *)pg->virt);
            continue;
        }
        // 失败回滚
        list_remove(&pg->node);
        page_reference_dec(pg->pfn);
        kfree(pg);
        status = -EINVAL;
        goto fail;
    }

    area = kmalloc(sizeof(*area), 0, 0);
    if (area == NULL)
    {
        status = -ENOMEM;
        goto fail;
    }

    // 3) 提交: 登记附着, 段的引用转由 area 持有
    int removed = 0;

    spin_lock(&shm_table.lock);
    removed = (shm_table.slot[shm_id_slot(shmid)] != seg);
    if (!removed)
    {
        seg->nattch++;
    }
    spin_unlock(&shm_table.lock);

    // 期间被 IPC_RMID 摘除, 不再接受新的附着
    if (removed)
    {
        kfree(area);
        status = -EINVAL;
        goto fail;
    }

    area->shm_seg   = seg;
    area->virt      = virt;
    area->pages     = pages;
    area->read_only = ro;
    list_append(&task->mm->sh_map.list, &area->node);

    return virt;

fail:
    // 已映射的页会 unmap 并释放页引用, 仅预留未映射的页只还原区间类型
    mm_detach_shared(task, virt, pages);

end:
    put_kobject(&seg->kobj);
    return (uintptr_t)status;
}
