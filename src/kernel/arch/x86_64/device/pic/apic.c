#include <kernel/global.h>
#include <io.h>
#include <device/cpu.h>
#include <device/pic.h>

#include <log.h>

/*
 Base Address | MSR address | Name               | Attribute
 0xfee00020   | 0x802       | Local APIC ID      | RW
 0xfee00030   | 0x803       | Local APIC Version | RO
 0xfee00080   | 0x808       | TPR                | RW
 0xfee00090   | -           | APR                | RO
 0xfee000a0   | 0x80a       | PPR                | RO
 0xfee000b0   | 0x80b       | EOI                | WO
 0xfee000f0   | 0x80f       | SVR                | RW
 0xfee00350   | 0x835       | LVT LINT0          | RW
 0xfee00360   | 0x836       | LVT LINT1          | RW
 0xfee00370   | 0x837       | LVT Error          | RW
*/


PUBLIC apic_t apic_struct;

PUBLIC bool support_apic()
{
    uint32_t a,b,c,d;
    cpuid(1,0,&a,&b,&c,&d);
    return (d & (1 << 9)) && (rdmsr(IA32_APIC_BASE) & (1 << 11));
}

PUBLIC void detect_cores()
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
    io_mfence();
}

PUBLIC uint32_t local_apic_read(uint16_t index)
{
    return *(uint32_t*)KADDR_P2V(apic_struct.local_apic_address + index);
    io_mfence();
}

PRIVATE void local_apic_init()
{
    uint32_t a,b,c,d;
    cpuid(1,0,&a,&b,&c,&d);
    if (a & 0x800)
    {
        pr_log("\1APIC & xAPIC enabled\n");
    }
    if (a & 0x400)
    {
        pr_log("\1x2APIC enabled\n");
    }

    pr_log("\1APIC ID: %x\n",local_apic_read(0x020));
    pr_log("\1APIC version: %x\n",local_apic_read(0x030) & 0xff);
    pr_log("\1APIC Max LVT Entry: %x\n",((local_apic_read(0x030) >> 16) & 0xff) + 1);
    pr_log("\1Suppress EOI Broadcast: %s\n",(local_apic_read(0x030) >> 24) & 1 ? "Yes" : "No");
    // enable SVR[8]
    pr_log("\1enable SVR[8].\n");
    uint32_t svr = local_apic_read(0x0f0);
    svr |= 1 << 8;
    if (local_apic_read(0x030) >> 24 & 1)
    {
        pr_log("\1enable SVR[12]\n");
        svr |= 1 << 12;
    }
    local_apic_write(0x0f0,svr);

    // Mask all LVT
    pr_log("\1Mask all LVT.\n");
    local_apic_write(0x2f0,0x10000);
    local_apic_write(0x320,0x10000);
    local_apic_write(0x330,0x10000);
    local_apic_write(0x340,0x10000);
    local_apic_write(0x350,0x10000);
    local_apic_write(0x360,0x10000);
    local_apic_write(0x370,0x10000);

    pr_log("\1APIC LVT TPR: %x\n",local_apic_read(0x080));
    pr_log("\1APIC LVT PPR: %x\n",local_apic_read(0x0a0));

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
    value = value & (~0x100ffUL);
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
    // 禁止8259A的所有中断
    io_out8(PIC_M_DATA, 0xff ); /* 11111111 禁止所有中断 */
    io_out8(PIC_S_DATA, 0xff ); /* 11111111 禁止所有中断 */

    pr_log("\1init local apic.\n");
    local_apic_init();
    pr_log("\1init ioapic.\n");
    ioapic_init();
    return;
}