/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.

Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU Lesser General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 宽通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 宽通用公共许可证，了解详情。

你应该随程序获得一份 GNU 宽通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

#include <kernel/global.h>
#include <kernel/init.h> // segmdesc
#include <task/task.h>   // task_struct
#include <std/string.h>  // memset,memcpy
#include <device/cpu.h>  // apic_id

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

            gdt_table[5 + nr_cpu * 2] = make_segmdesc(
                                            (uint32_t)(tss_base_l & 0xffffffff),
                                            tss_size - 1,
                                            AR_TSS64);
    memcpy(&gdt_table[5 + nr_cpu * 2 + 1],&tss_base_h,8);
    return;
}

PUBLIC void update_tss_rsp0(task_struct_t *task)
{
    tss[apic_id()].rsp0 = task->kstack_base + task->kstack_size;
    return;
}