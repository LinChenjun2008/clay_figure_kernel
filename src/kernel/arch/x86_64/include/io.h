#ifndef __IO_H__
#define __IO_H__

static inline void io_hlt(void)
{
    __asm__ __volatile__ ("hlt":::"memory");
    return;
}

static inline void io_sti(void)
{
    __asm__ __volatile__ ("sti":::"memory");
    return;
}

static inline void io_cli(void)
{
    __asm__ __volatile__ ("cli":::"memory");
    return;
}

static inline void io_stihlt(void)
{
    __asm__ __volatile__ ("sti\n\t""hlt;":::"memory");
    return;
}

static inline void io_mfence(void)
{
    __asm__ __volatile("mfence":::"memory");
}

static inline uint32_t io_in8(uint32_t port)
{
    uint32_t data;
    __asm__ __volatile__
    (
        "inb %w1,%b0"
        :"=a"(data)
        :"d"(port)
        :"memory"
    );
    return data;
}

static inline uint32_t io_in16(uint32_t port)
{
    uint32_t data;
    __asm__ __volatile__
    (
        "inw %w1,%w0"
        :"=a"(data)
        :"d"(port)
        :"memory"
    );
    return data;
}

static inline uint32_t io_in32(uint32_t port)
{
    uint32_t data;
    __asm__ __volatile__
    (
        "inl %w1,%k0"
        :"=a"(data)
        :"d"(port)
        :"memory"
    );
    return data;
}



static inline void io_out8(uint32_t port,uint32_t data)
{
    __asm__ __volatile__
    (
        "outb %b0,%w1"
        :
        :"a"(data),"d"(port)
        :"memory"
    );
}

static inline void io_out16(uint32_t port,uint32_t data)
{
    __asm__ __volatile__
    (
        "outw %w0,%w1"
        :
        :"a"(data),"d"(port)
        :"memory"
    );
}

static inline void io_out32(uint32_t port,uint32_t data)
{
    __asm__ __volatile__
    (
        "outl %k0,%w1"
        :
        :"a"(data),"d"(port)
        :"memory"
    );
}

static inline uint32_t get_flages()
{
    uint32_t flages;
    __asm__ __volatile__
    (
        "pushf;"      /* 将flage寄存器压栈 */
        "popq %q0"
        :"=a"(flages)
        :
        :"memory"
    );
    return flages;
}

#endif