#include <kernel/global.h>
#include <io.h>
#include <device/cpu.h>
#include <device/pic.h>
#include <std/string.h>

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
 0xfee00300   | 0x830       | ICR (bit 31 :  0)  | RW
 0xfee00310   | 0x830       | ICR (bit 63 : 32)  | RW
 0xfee00350   | 0x835       | LVT LINT0          | RW
 0xfee00360   | 0x836       | LVT LINT1          | RW
 0xfee00370   | 0x837       | LVT Error          | RW
*/


PUBLIC apic_t apic;

PUBLIC bool support_apic()
{
    uint32_t a,b,c,d;
    cpuid(1,0,&a,&b,&c,&d);
    return (d & (1 << 9)) && (rdmsr(IA32_APIC_BASE) & (1 << 11));
}

PUBLIC void detect_cores()
{
    apic.local_apic_address   = 0;
    apic.number_of_cores      = 0;
    apic.number_of_ioapic     = 0;
    memset(&apic.lapic_id, 0, sizeof(apic.lapic_id));
    memset(  &apic.ioapic, 0,   sizeof(apic.ioapic));

    MADT_t *madt = (MADT_t*)KADDR_P2V(g_boot_info->madt_addr);
    apic.local_apic_address = madt->LocalApicAddress;
    uint8_t *p = (uint8_t*)(madt + 1);
    uint8_t *p2 = (uint8_t*)madt + madt->Header.Length;
    for (p = (uint8_t*)(madt + 1);p < p2;p += p[1]/* Record Length */)
    {
        switch(p[0]) // Entry Type
        {
            case 0:
                if (p[4] & 1)
                {
                    apic.lapic_id[apic.number_of_cores++] = p[3];
                }
                break;
            case 1:
                uint64_t ioapic_addr = (uint64_t)(*(uint32_t*)(p + 4) + 0UL);
                apic.ioapic[apic.number_of_ioapic].index_addr = (uint8_t*)ioapic_addr;
                apic.ioapic[apic.number_of_ioapic].data_addr  = (uint32_t*)(ioapic_addr + 0x10UL);
                apic.ioapic[apic.number_of_ioapic].EOI_addr   = (uint32_t*)(ioapic_addr + 0x40UL);
                apic.number_of_ioapic++;
                pr_log("\2 Get IOAPIC address: %p\n",ioapic_addr);
                break;
        }
    }
    pr_log("\1 cores: %d, local apic addr %p\n",apic.number_of_cores,apic.local_apic_address);
    return;
}

PUBLIC void local_apic_write(uint16_t index,uint32_t value)
{
    *(uint32_t*)KADDR_P2V(apic.local_apic_address + index) = value;
    io_mfence();
}

PUBLIC uint32_t local_apic_read(uint16_t index)
{
    return *(uint32_t*)KADDR_P2V(apic.local_apic_address + index);
    io_mfence();
}

PRIVATE void xapic_init()
{
    // enable SVR[8]
    uint32_t svr = local_apic_read(0x0f0);
    svr |= 1 << 8;
    if (local_apic_read(0x030) >> 24 & 1)
    {
        svr |= 1 << 12;
    }
    local_apic_write(0x0f0,svr);

    // Mask all LVT
    local_apic_write(0x2f0,0x10000);
    local_apic_write(0x320,0x10000);
    local_apic_write(0x330,0x10000);
    local_apic_write(0x340,0x10000);
    local_apic_write(0x350,0x10000);
    local_apic_write(0x360,0x10000);
    local_apic_write(0x370,0x10000);
}

PRIVATE void x2apic_init()
{
    // enable SVR[8]
    uint32_t svr = rdmsr(0x80f);
    svr |= 1 << 8;
    if (rdmsr(0x803) >> 24 & 1)
    {
        svr |= 1 << 12;
    }
    wrmsr(0x80f,svr);

    // Mask all LVT
    wrmsr(0x82f,0x10000);
    wrmsr(0x832,0x10000);
    wrmsr(0x833,0x10000);
    wrmsr(0x834,0x10000);
    wrmsr(0x835,0x10000);
    wrmsr(0x836,0x10000);
    wrmsr(0x837,0x10000);
}

PUBLIC void local_apic_init()
{
    uint32_t a,b,c,d;
    cpuid(1,0,&a,&b,&c,&d);
    uint64_t ia32_apic_base = rdmsr(IA32_APIC_BASE);
    if ((c & (1 << 21)) && (ia32_apic_base & 0x400))
    {
        x2apic_init();
    }
    else
    {
        if ((d & (1 << 9)) && (ia32_apic_base & 0x800))
        {
            xapic_init();
        }
    }
    return;
}

// PRIVATE uint64_t ioapic_rte_read(uint8_t index)
// {
//     uint64_t ret;
//     *(uint8_t*)KADDR_P2V(apic.ioapic[0].index_addr) = index + 1;
//     io_mfence();
//     ret = *(uint32_t*)KADDR_P2V(apic.ioapic[0].data_addr);
//     ret <<= 32;
//     io_mfence();

//     *(uint8_t*)KADDR_P2V(apic.ioapic[0].index_addr) = index;
//     io_mfence();
//     ret |= *(uint32_t*)KADDR_P2V(apic.ioapic[0].data_addr);
//     io_mfence();

//     return ret;
// }

PRIVATE void ioapic_rte_write(uint8_t index,uint64_t value)
{
    *(uint8_t*)KADDR_P2V(apic.ioapic[0].index_addr) = index;
    io_mfence();
    *(uint32_t*)KADDR_P2V(apic.ioapic[0].data_addr) = value & 0xffffffff;
    value >>= 32;
    io_mfence();

    *(uint8_t*)KADDR_P2V(apic.ioapic[0].index_addr) = index + 1;
    io_mfence();
    *(uint32_t*)KADDR_P2V(apic.ioapic[0].data_addr) = value & 0xffffffff;
    io_mfence();
    return;
}

PUBLIC void ioapic_enable(uint64_t pin,uint64_t vector)
{
    ioapic_rte_write(pin * 2 + 0x10,vector & 0xff);
    return;
}

PRIVATE void ioapic_init()
{
    *(uint8_t*)KADDR_P2V(apic.ioapic[0].index_addr) = 0;
    io_mfence();
    *(uint32_t*)KADDR_P2V(apic.ioapic[0].data_addr) = 0x0f000000;
    io_mfence();

    /* 屏蔽所有中断 */
    int i;
    for (i = 0x10;i < 0x40;i += 2)
    {
        ioapic_rte_write(i,0x10000);
    }
    ioapic_enable(2,0x20);
    ioapic_enable(1,0x21);
    return;
}

PUBLIC void apic_init()
{

    // 禁止8259A的所有中断
    io_out8(PIC_M_DATA, 0xff ); /* 11111111 禁止所有中断 */
    io_out8(PIC_S_DATA, 0xff ); /* 11111111 禁止所有中断 */

    // IMCR
    io_out8(0x22,0x70);
    io_out8(0x23,0x01);

    pr_log("\1 init local apic.\n");
    local_apic_init();
    pr_log("\1 init ioapic.\n");
    ioapic_init();
    pr_log("\1 init ioapic done.\n");
    return;
}