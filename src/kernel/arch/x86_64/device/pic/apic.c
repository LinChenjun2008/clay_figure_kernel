#include <kernel/global.h>
#include <io.h>
#include <device/cpu.h>
#include <device/pic.h>

#include <log.h>

PUBLIC apic_t apic_struct;

PUBLIC bool support_apic()
{
    uint32_t a,b,c,d;
    cpuid(1,0,&a,&b,&c,&d);
    return (d & (1 << 9)) && (rdmsr(IA32_APIC_BASE) & (1 << 11));
}

PRIVATE void detect_cores()
{
    apic_struct.local_apic_address   = 0;
    apic_struct.ioapic_address       = 0;
    apic_struct.ioapic_index_address = NULL;
    apic_struct.ioapic_data_address  = NULL;
    apic_struct.ioapic_EOI_address   = NULL;
    apic_struct.number_of_cores = 0;

    MADT_t *madt = (MADT_t*)KADDR_P2V(g_boot_info->madt_addr);
    apic_struct.local_apic_address = madt->LocalApicAddress;
    uint8_t *p = (uint8_t*)(madt + 1);
    uint8_t *p2 = (uint8_t*)madt + madt->Header.Length;
    for (p = (uint8_t*)(madt + 1);p < p2;p += p[1]/* Record Length */)
    {
        switch(p[0]) // Entry Type
        {
            case 0:
                if (p[4] & 1)
                {
                    apic_struct.lapic_id[apic_struct.number_of_cores++] = p[3];
                }
                break;
            case 1:
                apic_struct.ioapic_address = (uint64_t)*(uint32_t*)(p + 4);
                break;
            case 5:
                apic_struct.local_apic_address = *(uint64_t*)(p + 4);
                break;
        }
    }
    apic_struct.ioapic_index_address = (uint8_t*)(apic_struct.ioapic_address + 0UL);
    apic_struct.ioapic_data_address  = (uint32_t*)(apic_struct.ioapic_address + 0x10UL);
    apic_struct.ioapic_EOI_address   = (uint32_t*)(apic_struct.ioapic_address + 0x40UL);
    pr_log("\1cores: %d, local apic addr %p,ioapic addr %p.\n",apic_struct.number_of_cores,apic_struct.local_apic_address,apic_struct.ioapic_address);
    return;
}

PUBLIC void local_apic_write(uint16_t index,uint32_t value)
{
    *(uint32_t*)KADDR_P2V(apic_struct.local_apic_address + index) = value;
    // wait for write finish
    value = *(uint32_t*)KADDR_P2V(apic_struct.local_apic_address + 0x020);
}

PRIVATE void local_apic_init()
{
    // enable SVR[8]
    pr_log("\1enable SVR[8].\n");
    local_apic_write(0x0f0,1 << 8);

    // Mask all LVT
    pr_log("\1Mask all LVT.\n");
    local_apic_write(0x2f0,0x10000);
    local_apic_write(0x320,0x10000);
    local_apic_write(0x330,0x10000);
    local_apic_write(0x340,0x10000);
    local_apic_write(0x350,0x10000);
    local_apic_write(0x360,0x10000);
    local_apic_write(0x370,0x10000);

    return;
}


PRIVATE uint64_t ioapic_rte_read(uint8_t index)
{
    uint64_t ret;
    *(uint8_t*)KADDR_P2V(apic_struct.ioapic_index_address) = index + 1;
    io_mfence();
    ret = *(uint32_t*)KADDR_P2V(apic_struct.ioapic_data_address);
    ret <<= 32;
    io_mfence();

    *(uint8_t*)KADDR_P2V(apic_struct.ioapic_index_address) = index;
    io_mfence();
    ret |= *(uint32_t*)KADDR_P2V(apic_struct.ioapic_data_address);
    io_mfence();

    return ret;
}

PRIVATE void ioapic_rte_write(uint8_t index,uint64_t value)
{
    *(uint8_t*)KADDR_P2V(apic_struct.ioapic_index_address) = index;
    io_mfence();
    *(uint32_t*)KADDR_P2V(apic_struct.ioapic_data_address) = value & 0xffffffff;
    value >>= 32;
    io_mfence();

    *(uint8_t*)KADDR_P2V(apic_struct.ioapic_index_address) = index + 1;
    io_mfence();
    *(uint32_t*)KADDR_P2V(apic_struct.ioapic_data_address) = value & 0xffffffff;
    io_mfence();
    return;
}

PUBLIC void ioapic_enable(uint64_t pin,uint64_t vector)
{
    uint64_t value = 0;
    value = ioapic_rte_read(pin * 2 + 0x10);
    value = value & (~0x10000UL);
    ioapic_rte_write(pin * 2 + 0x10,value | vector);
    return;
}

PRIVATE void ioapic_init()
{
    /* 屏蔽所有中断 */
    int i;
    for (i = 0x10;i < 0x40;i += 2)
    {
        ioapic_rte_write(i,0x10000);
    }
    ioapic_enable(2,0x20);
    return;
}

PUBLIC void apic_init()
{
    detect_cores();
    // 禁止8259A的所有中断
    io_out8(PIC_M_DATA, 0xff ); /* 11111111 禁止所有中断 */
    io_out8(PIC_S_DATA, 0xff ); /* 11111111 禁止所有中断 */

    local_apic_init();
    pr_log("\1Init ioapic.\n");
    ioapic_init();
    return;
}