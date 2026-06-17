// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_PAGE_H__
#define __ASM_PAGE_H__

#define PT_SIZE 0x1000

#define PG_4K_SIZE 0x1000
#define PG_2M_SIZE 0x200000

#define PG_SIZE PG_4K_SIZE

// Present
#define PG_P (1 << 0)

// Read/Write
#define PG_RW_R (0 << 1)
#define PG_RW_W (1 << 1)

// User/Supervisor
#define PG_US_S (0 << 2)
#define PG_US_U (1 << 2)

// Page Chace Disable
#define PG_PCD           (1 << 4)
#define PG_SIZE_2M       (1 << 7)
#define PG_DEFAULT_FLAGS (PG_US_U | PG_RW_W | PG_P)

#define ADDR_PML4T_INDEX_SHIFT 39
#define ADDR_PML4T_INDEX_MASK  0x1ff
#define ADDR_PDPT_INDEX_SHIFT  30
#define ADDR_PDPT_INDEX_MASK   0x1ff
#define ADDR_PDT_INDEX_SHIFT   21
#define ADDR_PDT_INDEX_MASK    0x1ff
#define ADDR_PT_INDEX_SHIFT    12
#define ADDR_PT_INDEX_MASK     0x1ff
#define ADDR_OFFSET_SHIFT      0
#define ADDR_OFFSET_MASK       0x0fff

#define KERNEL_VMA_BASE  0xffff800000000000
#define KERNEL_TEXT_BASE 0xffffffff80000000

#define PHYS_TO_VIRT(ADDR) ((void *)((uintptr_t)(ADDR) + KERNEL_VMA_BASE))
#define VIRT_TO_PHYS(ADDR) ((void *)((uintptr_t)(ADDR) - KERNEL_VMA_BASE))

#ifndef __ASSEMBLER__

#    include <base.h>

enum mm_type
{
    MM_TYPE_FREE = 1,
    MM_TYPE_RESERVED,
    MM_TYPE_UNUSEABLE,
    MAX_MM_TYPE,
};

void      page_init(struct boot_info *boot_info);
int       allocate_pages(size_t pages, void **addr);
void      free_pages(void **addr, size_t pages);
uint64_t *get_page_table(void);
void      set_page_table(void *page_table);
void      page_map(uint64_t *pml4t, void *paddr, void *vaddr, uint64_t count);
void     *to_physical_address(void *pml4t, void *vaddr);

#endif /* __ASSEMBLER__ */

#endif /* __ASM_PAGE_H__ */