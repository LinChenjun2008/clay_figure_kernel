/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPLv3-or-later

*/

#ifndef __PIC_H__
#define __PIC_H__

#define MADT_SIGNATURE SIGNATURE_32('A', 'P', 'I', 'C') //"APIC"

#define APIC_REG_ID         0x020
#define APIC_REG_VERSION    0x030
#define APIC_REG_TPR        0x080
#define APIC_REG_PPR        0x0a0
#define APIC_REG_EOI        0x0b0
#define APIC_REG_SVR        0x0f0
#define APIC_REG_ICR_LO     0x300
#define APIC_REG_ICR_HI     0x310
#define APIC_REG_LVT_TIMER  0x320
#define APIC_REG_LVT_LINT0  0x350
#define APIC_REG_LVT_LINT1  0x360
#define APIC_REG_LVT_ERROR  0x370
#define APIC_REG_TIMER_ICNT 0x380
#define APIC_REG_TIMER_CCNT 0x390
#define APIC_REG_TIMER_DIV  0x3e0

#define IRQ_CNT 0xff

#define PIC_M_CTRL 0x20 /* 8259A主片的控制端口是0x20 */
#define PIC_M_DATA 0x21 /* 8259A主片的数据端口是0x21 */
#define PIC_S_CTRL 0xa0 /* 8259A从片的控制端口是0xa0 */
#define PIC_S_DATA 0xa1 /* 8259A从片的数据端口是0xa1 */

#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040

#define ICR_DELIVER_MODE_FIXED    0
#define ICR_DELIVER_MODE_SMI      2
#define ICR_DELIVER_MODE_NMI      4
#define ICR_DELIVER_MODE_INIT     5
#define ICR_DELIVER_MODE_START_UP 6

#define ICR_DEST_MODE_PHY   0
#define ICR_DEST_MODE_LOGIC 1

#define ICR_LEVEL_DE_ASSEST 0
#define ICR_LEVEL_ASSERT    1

#define ICR_TRIGGER_EDGE  0
#define ICR_TRIGGER_LEVEL 1

#define ICR_DELIVER_STATUS_IDLE         0
#define ICR_DELIVER_STATUS_SEND_PENDING 1

#define ICR_NO_SHORTHAND     0
#define ICR_SELF             1
#define ICR_ALL_INCLUDE_SELF 2
#define ICR_ALL_EXCLUDE_SELF 3

#define EFLAGS_IF (1 << 9)


#pragma pack(1)
typedef struct ACPI_DESCRIPTION_HEADER_s
{
    uint32_t Signature;
    uint32_t Length;
    uint8_t  Revision;
    uint8_t  Checksum;
    uint8_t  OemId[6];
    uint64_t OemTableId;
    uint32_t OemRevision;
    uint32_t CreatorId;
    uint32_t CreatorRevision;
} ACPI_DESCRIPTION_HEADER_t;

typedef struct XSDT_TABLE_s
{
    ACPI_DESCRIPTION_HEADER_t Header;
    uint64_t                  Entry;
} XSDT_TABLE_t;

// MADT
typedef struct MADT_s
{
    ACPI_DESCRIPTION_HEADER_t Header;
    uint32_t                  LocalApicAddress;
    uint32_t                  Flags;
} MADT_t;
#pragma pack()

typedef struct ioapic_s
{
    uint8_t  *index_addr;
    uint32_t *data_addr;
    uint32_t *EOI_addr;
} ioapic_t;

typedef struct apic_s
{
    uint64_t local_apic_address;
    uint8_t  number_of_cores;
    uint8_t  lapic_id[256];
    uint8_t  number_of_ioapic;
    ioapic_t ioapic[8];
} apic_t;

/**
 * @brief 解析MADT,获取apic信息和cpu核心数
 */
PUBLIC void detect_cores(void);

/**
 * @brief 写入local apic寄存器
 * @param index 寄存器索引(MMIO偏移量)
 * @param value 写入值
 */
PUBLIC void local_apic_write(uint16_t index, uint32_t value);

/**
 * @brief 读取local apic寄存器
 * @param index 寄存器索引(MMIO偏移量)
 * @return 寄存器值
 */
PUBLIC uint32_t local_apic_read(uint16_t index);

/**
 * @brief 初始化设定的可编程中断控制器
 */
PUBLIC void pic_init(void);

/**
 * @brief 初始化8259A中断控制器
 */
PUBLIC void init_8259a(void);

/**
 * @brief 禁用8259A并初始化local apic和ioapic
 */
PUBLIC void apic_init(void);

/**
 * @brief 对local apic进行初始化
 */
PUBLIC void local_apic_init(void);

/**
 * @brief 向中断控制器发送EOI
 * @param irq 中断向量号
 */
PUBLIC void send_eoi(uint8_t irq);

/**
 * @brief 启用IO apic对应引脚的中断
 * @param pin 中断引脚
 * @param vector 向量号
 */
PUBLIC void ioapic_enable(uint64_t pin, uint64_t vector);

#endif