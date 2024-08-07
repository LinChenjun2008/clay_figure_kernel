#include <kernel/global.h>
#include <kernel/init.h>
#include <task/task.h>
#include <std/string.h>

#include <device/cpu.h>

extern segmdesc_t gdt_table[];

#pragma pack(1)
struct TSS64
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
};
#pragma pack()

PRIVATE struct TSS64 tss[NR_CPUS];

PUBLIC void init_tss(uint8_t nr_cpu)
{
    uint32_t tss_size = sizeof(struct TSS64);
    memset(&tss[nr_cpu],0,tss_size);
    tss[nr_cpu].io_map = tss_size << 16;
    uint64_t tss_base_l = ((uint64_t)&tss[nr_cpu]) & 0xffffffff;
    uint64_t tss_base_h = (((uint64_t)&tss[nr_cpu]) >> 32) & 0xffffffff;

            gdt_table[5 + nr_cpu * 2] = make_segmdesc
            (
                (uint32_t)(tss_base_l & 0xffffffff),
                tss_size - 1,
                AR_TSS64
            );
    memcpy(&gdt_table[5 + nr_cpu * 2 + 1],&tss_base_h,8);
    return;
}

PUBLIC void update_tss_rsp0(task_struct_t *task)
{
    tss[apic_id()].rsp0 = task->kstack_base + task->kstack_size;
    return;
}