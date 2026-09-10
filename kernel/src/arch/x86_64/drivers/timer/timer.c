// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/drivers/apic/ioapic.h>
#include <asm/drivers/pic.h> // send_eoi
#include <asm/drivers/timer.h>
#include <asm/intr/handler.h>

#include <softirq.h>
#include <sysinfo.h>
#include <task/schedule.h>

static uint64_t ticks = 0;

static void timer_softirq(void)
{
    ticks++;
    return;
}

static void timer()
{
    send_eoi();
    raise_softirq(TIMER_SOFTIRQ);
    return;
}

static void apic_timer(void)
{
    send_eoi();
    task_update();
    return;
}

void timer_init(struct boot_info *boot_info)
{
    ticks = 0;
    register_handler(0x20, timer);
    register_handler(0x80, apic_timer);
    register_softirq(TIMER_SOFTIRQ, timer_softirq, NULL);

    hpet_init(boot_info);
    apic_timer_init();
    ioapic_irq_enable(0, 0x20, get_current_cpu_id());
    return;
}

// 1 ticks = 1 microsecond
uint64_t get_ticks(void)
{
    uint64_t nsecond = get_nano_time();
    uint64_t msecond = nsecond / 1000000;
    return msecond;
}