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
#define PG_PCD            (1 << 4)
#define PG_SIZE_2M        (1 << 7)
#define PG_DEFAULT_FLAGS  (PG_US_U | PG_RW_W | PG_P)
#define PG_KERNEL_FLAGS   (PG_US_S | PG_RW_W | PG_P)
#define PG_USER_FLAGS     (PG_US_U | PG_RW_W | PG_P)
#define PG_USER_COW_FLAGS (PG_US_U | PG_RW_R | PG_P)
#define PG_UNMAPPED       0

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

#define PG_SIZE_SHIFT 12

#define PAGE_PFN_SHIFT 12
#define PAGE_PFN_MASK  0x000fffffffffffff

#define PFN_TO_ADDR(PFN)  SET_FIELD(0, PAGE_PFN, PFN)
#define ADDR_TO_PFN(ADDR) GET_FIELD(ADDR, PAGE_PFN)

#define KERNEL_VMA_BASE  0xffff800000000000
#define KERNEL_TEXT_BASE 0xffffffff80000000

#define PHYS_TO_VIRT(ADDR) ((void *)((uintptr_t)(ADDR) + KERNEL_VMA_BASE))
#define VIRT_TO_PHYS(ADDR) ((void *)((uintptr_t)(ADDR) - KERNEL_VMA_BASE))

#ifndef __ASSEMBLER__

void set_pg_table(void *pg_table);

void  page_map(uint64_t *pg_dir, void *paddr, void *vaddr, uint64_t count);
void  set_page_flags(uint64_t *pg_dir, void *vaddr, uint64_t flags);
void *to_physical_address(void *pg_dir, void *vaddr);
void  arch_mm_map(struct task *task, void *phys, void *virt);
void  arch_mm_unmap(struct task *task, void *virt);
void  arch_mm_map_cow(struct task *task, void *phys, void *virt);
void  free_pg_table(uint64_t *pg_dir);
void  page_faule(struct pt_regs *regs);

#endif /* __ASSEMBLER__ */

#endif /* __ASM_PAGE_H__ */
