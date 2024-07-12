#ifndef __PIC_H__
#define __PIC_H__

#define SIGNATURE32(A,B,C,D) ( D << 24 | C << 16 | B << 8 | A)
#define MADT_SIGNATURE SIGNATURE_32('A','P','I','C') //"APIC"


#pragma pack(1)
typedef struct
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

typedef struct
{
    ACPI_DESCRIPTION_HEADER_t Header;
    uint64_t                  Entry;
} XSDT_TABLE_t;

// MADT
typedef struct
{
    ACPI_DESCRIPTION_HEADER_t Header;
    uint32_t                  LocalApicAddress;
    uint32_t                  Flags;
} MADT_t;
#pragma pack()

typedef struct
{
    uint8_t  *index_addr;
    uint32_t *data_addr;
    uint32_t *EOI_addr;
} ioapic_t;

typedef struct
{
    uint64_t  local_apic_address;
    uint8_t   number_of_cores;
    uint8_t   lapic_id[256];
    uint8_t   number_of_ioapic;
    ioapic_t  ioapic[8];
} apic_t;

PUBLIC bool support_apic();
PUBLIC void detect_cores();

PUBLIC void local_apic_write(uint16_t index,uint32_t value);
PUBLIC uint32_t local_apic_read(uint16_t index);

PUBLIC void pic_init();
PUBLIC void local_apic_init();
PUBLIC void eoi(uint8_t irq);

#endif