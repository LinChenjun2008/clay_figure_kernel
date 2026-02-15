// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/drivers/timer.h>
#include <asm/mem/page.h> // PHYS_TO_VIRT
#include <asm/utils.h>

#include <drivers/acpi.h>
#include <print.h>

#define HPET_DEFAULT_ADDRESS 0xfed00000

#define HPET_GCAP_ID    00
#define HPET_GEN_CONF   0x10
#define HPET_MAIN_CNT   0xf0
#define HPET_TIME0_CONF 0x100
#define HPET_TIME0_COMP 0x108

#pragma pack(1)
typedef struct
{
    uint8_t  address_space_id; // 0 - system memory, 1 - system I/O
    uint8_t  register_bit_width;
    uint8_t  register_bit_offset;
    uint8_t  reserved;
    uint64_t address;
} hpet_address_t;

struct description_table_header
{
    char     signature[4]; // 'HPET' in case of HPET table
    uint32_t length;
    uint8_t  revision;
    uint8_t  checksum;
    char     oemid[6];
    uint64_t oem_tableid;
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
};

typedef struct
{
    acpi_description_header_t header;
    uint32_t                  id;
    hpet_address_t            address;
    uint8_t                   hpet_number;
    uint16_t                  minimum_tick;
    uint8_t                   page_protection;
} acpi_hpet_t;
#pragma pack()

typedef struct
{
    uintptr_t addr;
    uint64_t *gcap_id;
    uint64_t *gen_conf;
    uint64_t *main_cnt;
    uint64_t *time0_conf;
    uint64_t *time0_comp;
    uint64_t  period_fs;
} hpet_t;

static hpet_t hpet;

uint64_t get_nano_time(void)
{
    if (hpet.period_fs != 0)
    {
        return (*hpet.main_cnt) * hpet.period_fs / 1000000;
    }
    return 0;
}

void hpet_init(boot_info_t *boot_info)
{
    uint32_t     signature = SIGNATURE_32('H', 'P', 'E', 'T');
    acpi_hpet_t *hpet_table;
    hpet_table = (acpi_hpet_t *)xsdt_find_table(boot_info, signature);

    hpet_address_t *hpet_addr = (hpet_address_t *)&hpet_table->address;

    hpet.addr       = (uintptr_t)PHYS_TO_VIRT(hpet_addr->address);
    hpet.gcap_id    = (uint64_t *)(hpet.addr + HPET_GCAP_ID);
    hpet.gen_conf   = (uint64_t *)(hpet.addr + HPET_GEN_CONF);
    hpet.main_cnt   = (uint64_t *)(hpet.addr + HPET_MAIN_CNT);
    hpet.time0_conf = (uint64_t *)(hpet.addr + HPET_TIME0_CONF);
    hpet.time0_comp = (uint64_t *)(hpet.addr + HPET_TIME0_COMP);

    io_mfence();
    hpet.period_fs = (*hpet.gcap_id >> 32) & 0xffffffff;


    printk(
        MSG_HIGHLIGHT("HPET") ": address: %p, period: %lld(fs).\n",
        hpet.addr,
        hpet.period_fs
    );

    io_mfence();
    *hpet.gen_conf = 3;

    io_mfence();
    *hpet.time0_conf = 0x004c;

    io_mfence();
    *hpet.time0_comp = TIMER_FREQUENCY * 1000000000 / hpet.period_fs;

    io_mfence();
    *hpet.main_cnt = 0;

    io_mfence();
    return;
}