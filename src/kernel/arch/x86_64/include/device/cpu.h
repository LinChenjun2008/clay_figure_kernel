/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.

Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 通用公共许可证，了解详情。

你应该随程序获得一份 GNU 通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

#ifndef __CPU_H__
#define __CPU_H__

extern uint64_t rdmsr(uint64_t address);
extern void wrmsr(uint64_t address,uint64_t value);

extern void asm_cpuid(
    uint32_t mop,
    uint32_t sop,
    uint32_t *a,
    uint32_t *b,
    uint32_t *c,
    uint32_t *d);

static __inline__ void cpu_name(char *s)
{
    uint32_t i;
    for (i = 0x80000002; i < 0x80000005;i++)
    {
        asm_cpuid(i,
              0,
              (uint32_t*)s + (i - 0x80000002) * 4,
              (uint32_t*)s + (i - 0x80000002) * 4 + 1,
              (uint32_t*)s + (i - 0x80000002) * 4 + 2,
              (uint32_t*)s + (i - 0x80000002) * 4 + 3);
    }
}

static __inline__ bool is_bsp()
{
    return rdmsr(IA32_APIC_BASE) & IA32_APIC_BASE_BSP;
}

static __inline__ uint32_t apic_id()
{
    uint32_t a,b,c,d;
    asm_cpuid(1,0,&a,&b,&c,&d);
    return (b >> 24) & 0xff;
}

PUBLIC uint64_t make_icr(
    uint8_t  vector,
    uint8_t  deliver_mode,
    uint8_t  dest_mode,
    uint8_t  deliver_status,
    uint8_t  level,
    uint8_t  trigger,
    uint8_t  des_shorthand,
    uint32_t destination);

PUBLIC status_t smp_start();
PUBLIC void send_IPI(uint64_t icr);

extern uint8_t AP_BOOT_BASE[];
extern uint8_t AP_BOOT_END[];

#endif