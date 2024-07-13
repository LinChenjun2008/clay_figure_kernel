#include <kernel/global.h>
#include <device/pic.h>
#include <io.h>
#include <device/cpu.h>

#include <log.h>

PUBLIC bool support_apic();
PUBLIC void init_8259a();
PUBLIC void apic_init();
PUBLIC void local_apic_write(uint16_t index,uint32_t value);

extern apic_t apic;

PUBLIC void pic_init()
{
    if (support_apic())
    {
        apic_init();
    }
    else
    {
        init_8259a();
    }
    return;
}

PUBLIC void eoi(uint8_t irq)
{
    (void)irq;
    #if !__DISABLE_APIC__
    if (support_apic())
    {
        local_apic_write(0x0b0,0);
    }
    else
    #endif
    {
        io_out8(PIC_M_CTRL,0x20);
    }
}