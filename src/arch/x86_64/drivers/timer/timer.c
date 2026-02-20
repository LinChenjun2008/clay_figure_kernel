// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/drivers/apic.h> // ioapic_irq_enable
#include <asm/drivers/pic.h>  // send_eoi
#include <asm/drivers/timer.h>
#include <asm/interrupt.h>

#include <task.h>

static void timer()
{
    send_eoi();
    static uint64_t tick = 0;
    tick++;
    if (tick >= 1000)
    {
        tick = 0;
        task_balance();
    }
    return;
}

static void apic_timer(void)
{
    send_eoi();
    task_update();
    return;
}

void timer_init(boot_info_t *boot_info)
{
    register_handler(0x20, timer);
    register_handler(0x80, apic_timer);

    hpet_init(boot_info);
    apic_timer_init();
    ioapic_irq_enable(0, 0x20, 0);
    return;
}