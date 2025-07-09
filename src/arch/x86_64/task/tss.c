/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPLv3-or-later

*/

#include <kernel/global.h>
#include <kernel/init.h> // segmdesc

#include <device/cpu.h> // apic_id
#include <std/string.h> // memset,memcpy
#include <task/task.h>  // task_struct

extern segmdesc_t gdt_table[];

#pragma pack(1)
typedef struct tss64_s
{
    uint32_t reserved1;

    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;

    uint32_t reserved2;
    uint32_t reserved3;

    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;

    uint32_t reserved4;
    uint32_t reserved5;

    uint32_t io_map;
} tss64_t;
#pragma pack()

PRIVATE tss64_t tss[NR_CPUS];

PUBLIC void init_tss(uint8_t cpu_id)
{
    uint32_t tss_size = sizeof(tss[0]);
    memset(&tss[cpu_id], 0, tss_size);
    tss[cpu_id].io_map  = tss_size << 16;
    uint64_t tss_base_l = ((uint64_t)&tss[cpu_id]) & 0xffffffff;
    uint64_t tss_base_h = (((uint64_t)&tss[cpu_id]) >> 32) & 0xffffffff;

    gdt_table[5 + cpu_id * 2] = make_segmdesc(
        (uint32_t)(tss_base_l & 0xffffffff), tss_size - 1, AR_TSS64
    );
    memcpy(&gdt_table[5 + cpu_id * 2 + 1], &tss_base_h, 8);
    return;
}

PUBLIC void update_tss_rsp0(task_struct_t *task)
{
    tss[apic_id()].rsp0 = task->kstack_base + task->kstack_size;
    return;
}

PUBLIC addr_t get_running_prog_kstack(void)
{
    return (addr_t)tss[apic_id()].rsp0;
}