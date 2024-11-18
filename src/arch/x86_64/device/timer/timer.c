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
#include <io.h>             // io_out8
#include <intr.h>           // register_handle
#include <device/pic.h>     // eoi
#include <device/cpu.h>     // make_icr,send_IPI
#include <task/task.h>      // do_schedule
#include <kernel/syscall.h> // inform_intr
#include <device/timer.h>   // COUNTER0_VALUE_LO,COUNTER0_VALUE_HI

PUBLIC volatile uint64_t global_ticks;

PRIVATE void irq_timer_handler()
{
    eoi(0x20);
    inform_intr(TICK);
    global_ticks++;
    // send IPI
    uint64_t icr;
    icr = make_icr(
        0x80,
        ICR_DELIVER_MODE_FIXED,
        ICR_DEST_MODE_PHY,
        ICR_DELIVER_STATUS_IDLE,
        ICR_LEVEL_DE_ASSEST,
        ICR_TRIGGER_EDGE,
        ICR_ALL_EXCLUDE_SELF,
        0);
    send_IPI(icr);

    schedule();
    return;
}

PUBLIC void pit_init()
{
    global_ticks = 0;
    register_handle(0x20,irq_timer_handler);
    ioapic_enable(2,0x20);
#if defined __TIMER_HPET__
    uint8_t *HPET_addr = (uint8_t *)0xfed00000;

    *(uint64_t*)(HPET_addr +  0x10) = 3;
    *(uint64_t*)(HPET_addr + 0x100) = 0x004c;
    *(uint64_t*)(HPET_addr + 0x108) = 1428571;
    *(uint64_t*)(HPET_addr + 0xf0) = 0;
#else
    io_out8(PIT_CTRL,0x34);
    io_out8(PIT_CNT0,COUNTER0_VALUE_LO);
    io_out8(PIT_CNT0,COUNTER0_VALUE_HI);
#endif
}