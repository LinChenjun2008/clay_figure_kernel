// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>
#include <kernel/syscall.h> // inform_intr

#include <log.h>

#include <device/cpu.h>   // make_icr,send_IPI
#include <device/pic.h>   // eoi
#include <device/timer.h> // COUNTER0_VALUE_LO,COUNTER0_VALUE_HI,IRQ0_FREQUENCY
#include <intr.h>         // register_handle
#include <io.h>           // io_out8
#include <mem/page.h>     // KADDR_P2V
#include <service.h>      // TICK
#include <task/task.h>    // do_schedule

PRIVATE volatile uint64_t current_ticks = 0;

typedef struct
{
    uint8_t  *addr;
    uint64_t *gcap_id;
    uint64_t *gen_conf;
    uint64_t *main_cnt;
    uint64_t *time0_conf;
    uint64_t *time0_comp;
    uint64_t  period_fs;
} hpet_t;

PRIVATE hpet_t hpet = { 0 };

PRIVATE void pit_timer_handler(intr_stack_t *stack)
{
    send_eoi(stack->int_vector);
    inform_intr(TICK);
    current_ticks++;
#ifdef __DISABLE_APIC_TIMER__
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
        0
    );
    send_IPI(icr);
#endif
    return;
}

PRIVATE void apic_timer_handler(intr_stack_t *stack)
{
    send_eoi(stack->int_vector);
    task_update();
    return;
}

PRIVATE status_t init_hpet(void)
{
    /// TODO: Get HPET address from acpi table.
    hpet.addr       = (uint8_t *)KADDR_P2V(0xfed00000);
    hpet.gcap_id    = (uint64_t *)(hpet.addr + HPET_GCAP_ID);
    hpet.gen_conf   = (uint64_t *)(hpet.addr + HPET_GEN_CONF);
    hpet.main_cnt   = (uint64_t *)(hpet.addr + HPET_MAIN_CNT);
    hpet.time0_conf = (uint64_t *)(hpet.addr + HPET_TIME0_CONF);
    hpet.time0_comp = (uint64_t *)(hpet.addr + HPET_TIME0_COMP);

    hpet.period_fs = (*hpet.gcap_id >> 32) & 0xffffffff;

    *hpet.gen_conf   = 3;
    *hpet.time0_conf = 0x004c;
    *hpet.time0_comp = IRQ0_FREQUENCY * 1000000000 / hpet.period_fs;
    *hpet.main_cnt   = 0;

    int i = 10000;
    while (i--) continue;
    // HPET not available.
    if (*hpet.main_cnt == 0)
    {
        return K_HW_NOSUPPORT;
    }
    return K_SUCCESS;
}

PRIVATE void init_8254pit(void)
{
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, COUNTER0_VALUE_LO);
    io_out8(PIT_CNT0, COUNTER0_VALUE_HI);
    return;
}

PUBLIC void pit_init(void)
{
    current_ticks = 0;
    register_handle(0x20, pit_timer_handler);
    register_handle(0x80, apic_timer_handler);
    ioapic_enable(2, 0x20);
    status_t status;
    status = init_hpet();
    if (ERROR(status))
    {
        init_8254pit();
    }
    return;
}

PUBLIC void apic_timer_init()
{
#ifndef __DISABLE_APIC_TIMER__
    local_apic_write(APIC_REG_TPR, 0); // Set TPR
    local_apic_write(APIC_REG_TIMER_DIV, 0x00000003);

    // map APIC timer to an interrupt, and by that enable it in one-shot mode.
    local_apic_write(APIC_REG_TIMER_ICNT, 0xffffffff);
    uint32_t          apic_ticks = 0;
    volatile uint32_t ticks      = get_current_ticks() + MSECOND_TO_TICKS(10);
    while (ticks >= get_current_ticks()) continue;

    // Stop APIC timer
    local_apic_write(APIC_REG_LVT_TIMER, 0x10000);
    apic_ticks = -local_apic_read(APIC_REG_TIMER_CCNT);

    // 1000 Hz
    apic_ticks /= 10;

    local_apic_write(APIC_REG_LVT_TIMER, 0x80 | 0x20000);
    local_apic_write(APIC_REG_TIMER_DIV, 0x00000003);
    local_apic_write(APIC_REG_TIMER_ICNT, apic_ticks);
#endif
    return;
}

PUBLIC uint64_t get_current_ticks(void)
{
    return current_ticks;
}

PUBLIC uint64_t get_nano_time(void)
{
    if (hpet.period_fs != 0)
        return (*hpet.main_cnt) * hpet.period_fs / 1000000;
    else
        return current_ticks * IRQ0_FREQUENCY * 1000;
}
