#ifndef __CPU_H__
#define __CPU_H__

static __inline__ uint64_t rdmsr(uint64_t address)
{
    uint64_t edx = 0;
    uint64_t eax = 0;
    __asm__ __volatile__ ("rdmsr":"=d"(edx),"=a"(eax):"c"(address):"memory");
    return (edx << 32) | eax;
}

static __inline__ void wrmsr(uint64_t address,uint64_t value)
{
    __asm__ __volatile__ (
        "wrmsr"
        ::"d"(value >> 32),"a"(value & 0xffffffff),"c"(address):"memory");
    return;
}

static __inline__ void cpuid(
    uint32_t mop,
    uint32_t sop,
    uint32_t *a,
    uint32_t *b,
    uint32_t *c,
    uint32_t *d)
{
    __asm__ __volatile__
    (
        "cpuid"
        :"=a"(*a),"=b"(*b),"=c"(*c),"=d"(*d)
        :"a"(mop),"b"(sop)
        :"memory"
    );
    return;
}

static __inline__ void cpu_name(char *s)
{
    uint32_t i;
    for (i = 0x80000002; i < 0x80000005;i++)
    {
        cpuid(i,
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
    cpuid(1,0,&a,&b,&c,&d);
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