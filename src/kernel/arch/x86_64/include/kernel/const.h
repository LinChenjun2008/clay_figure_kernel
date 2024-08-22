#ifndef __CONST_H__
#define __CONST_H__

#define KERNEL_PAGE_DIR_TABLE_POS 0x00000000005f9000

#define KERNEL_STACK_BASE 0x300000
#define KERNEL_STACK_SIZE 0x10000

#define PT_SIZE 0x1000
#define PG_SIZE 0x200000

#define PAGE_BITMAP_BYTES_LEN 2048

#define MIN_ALLOCATE_MEMORY_SIZE 32
#define MAX_ALLOCATE_MEMORY_SIZE 524288
#define NUMBER_OF_MEMORY_BLOCK_TYPES 14

#define PG_P       0b00000001
#define PG_RW_R    0b00000000
#define PG_RW_W    0b00000010
#define PG_US_S    0b00000000
#define PG_US_U    0b00000100
#define PG_SIZE_2M 0b10000000

#define TSS_D_0 0
#define AR_TSS32 (AR_G_4K | TSS_D_0 | AR_L | AR_AVL | AR_P | AR_DPL_0 | AR_S_SYS | AR_TYPE_TSS)

#define EFLAGS_MBS    (1 << 1)
#define EFLAGS_IF_1   (1 << 9)
#define EFLAGS_IF_0   (0 << 9)
#define EFLAGS_IOPL_0 (0 << 12)
#define EFLAGS_IOPL_3 (3 << 12)

#define AR_G_4K      0x8000
#define AR_D_32      0x4000
#define AR_D_64      0x0000
#define AR_L         0x0000
#define AR_L_64      0x2000
#define AR_AVL       0x0000
#define AR_P         0x0080
#define AR_DPL_0     0x0000
#define AR_DPL_1     0x0020
#define AR_DPL_2     0x0040
#define AR_DPL_3     0x0060
#define AR_S_CODE    0x0010
#define AR_S_DATA    0x0010
#define AR_S_SYS     0x0000
#define AR_TYPE_CODE 0x0008
#define AR_TYPE_DATA 0x0002
#define AR_TYPE_TSS  0x0009

#define AR_CODE32      (AR_G_4K | AR_D_32 | AR_L | AR_AVL | AR_P | AR_DPL_0 | AR_S_CODE | AR_TYPE_CODE)
#define AR_CODE32_DPL3 (AR_G_4K | AR_D_32 | AR_L | AR_AVL | AR_P | AR_DPL_3 | AR_S_CODE | AR_TYPE_CODE)
#define AR_DATA32      (AR_G_4K | AR_D_32 | AR_L | AR_AVL | AR_P | AR_DPL_0 | AR_S_DATA | AR_TYPE_DATA)
#define AR_DATA32_DPL3 (AR_G_4K | AR_D_32 | AR_L | AR_AVL | AR_P | AR_DPL_3 | AR_S_DATA | AR_TYPE_DATA)

#define AR_CODE64      (AR_G_4K | AR_D_64 | AR_L_64 | AR_AVL | AR_P | AR_DPL_0 | AR_S_CODE | AR_TYPE_CODE)
#define AR_CODE64_DPL3 (AR_G_4K | AR_D_64 | AR_L_64 | AR_AVL | AR_P | AR_DPL_3 | AR_S_CODE | AR_TYPE_CODE)
#define AR_DATA64      (AR_G_4K | AR_D_64 | AR_L_64 | AR_AVL | AR_P | AR_DPL_0 | AR_S_DATA | AR_TYPE_DATA)
#define AR_DATA64_DPL3 (AR_G_4K | AR_D_64 | AR_L_64 | AR_AVL | AR_P | AR_DPL_3 | AR_S_DATA | AR_TYPE_DATA)

#define TSS_D_0 0
#define AR_TSS64 (AR_G_4K | TSS_D_0 | AR_L | AR_AVL | AR_P | AR_DPL_0 | AR_S_SYS | AR_TYPE_TSS)

#define RPL0 0x0
#define RPL1 0x1
#define RPL2 0x2
#define RPL3 0x3

#define TI_GDT 0x0
#define TI_LDT 0x4

#define SELECTOR_CODE64_K     ((1 << 3) | TI_GDT | RPL0) /* 代码段 */
#define SELECTOR_DATA64_K     ((2 << 3) | TI_GDT | RPL0) /* 数据段 */

#define SELECTOR_DATA64_U     ((3 << 3) | TI_GDT | RPL3) /* 用户数据段 */
#define SELECTOR_CODE64_U     ((4 << 3) | TI_GDT | RPL3) /* 用户代码段 */

#define SELECTOR_TSS(CPU)     (((5 + CPU * 2) << 3) | TI_GDT | RPL0) /* TSS段 */

#define AR_DESC_32 0xe
#define AR_DESC_16 0x6

#define AR_IDT_DESC_DPL0 (AR_P | AR_DPL_0 | AR_DESC_32)
#define AR_IDT_DESC_DPL3 (AR_P | AR_DPL_3 | AR_DESC_32)

#define USER_STACK_VADDR_BASE (0x0000800000000000 - PG_SIZE)
// #define USER_VADDR_START 0x804800
#define USER_VADDR_START 0x800000

#define MAX_TASK 4096

#define DEFAULT_PRIORITY 3
#define SERVICE_PRIORITY 1

#define IRQ_CNT 0xff

#define PIC_M_CTRL 0x20    /* 8259A主片的控制端口是0x20 */
#define PIC_M_DATA 0x21    /* 8259A主片的数据端口是0x21 */
#define PIC_S_CTRL 0xa0    /* 8259A从片的控制端口是0xa0 */
#define PIC_S_DATA 0xa1    /* 8259A从片的数据端口是0xa1 */

#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040

#define IA32_APIC_BASE        0x0000001b
#define IA32_APIC_BASE_BSP    (1 << 8)
#define IA32_APIC_BASE_ENABLE (1 << 11)

#define IA32_EFER   0xc0000080
#define IA32_STAR   0xc0000081
#define IA32_LSTAR  0xc0000082
#define IA32_FMASK  0xc0000084

#define NR_CPUS 256

#define AP_STACK_BASE_PTR 0x1000

#define ICR_DELIVER_MODE_FIXED          0
#define ICR_DELIVER_MODE_SMI            2
#define ICR_DELIVER_MODE_NMI            4
#define ICR_DELIVER_MODE_INIT           5
#define ICR_DELIVER_MODE_START_UP       6

#define ICR_DEST_MODE_PHY               0
#define ICR_DEST_MODE_LOGIC             1

#define ICR_LEVEL_DE_ASSEST             0
#define ICR_LEVEL_ASSERT                1

#define ICR_TRIGGER_EDGE                0
#define ICR_TRIGGER_LEVEL               1

#define ICR_DELIVER_STATUS_IDLE         0
#define ICR_DELIVER_STATUS_SEND_PENDING 1

#define ICR_NO_SHORTHAND                0
#define ICR_SELF                        1
#define ICR_ALL_INCLUDE_SELF            2
#define ICR_ALL_EXCLUDE_SELF            3

#define EFLAGS_IF     (1 << 9)

#define IA32_EFER_SCE 1

#define PCI_CONFIG_ADDRESS 0x0cf8
#define PCI_CONFIG_DATA    0x0cfc

#define NR_SEND 0x80000001
#define NR_RECV 0x80000002
#define NR_BOTH 0x80000003

#define RECV_FROM_INT (MAX_TASK + 1)
#define RECV_FROM_ANY (MAX_TASK + 2)

#define SERVICE_ID_BASE 0x80000000UL

#define SERVICES 4

#define TICK     SERVICE_ID_BASE
#define MM       SERVICE_ID_BASE + 1
#define VIEW     SERVICE_ID_BASE + 2
#define USB_SRV  SERVICE_ID_BASE + 3

#define SYSCALL_SUCCESS        0x80000000
#define SYSCALL_ERROR          0xc0000000
#define SYSCALL_NO_SYSCALL     (SYSCALL_ERROR | 0x00000001)
#define SYSCALL_DEADLOCK       (SYSCALL_ERROR | 0x00000002)
#define SYSCALL_DEST_NOT_EXIST (SYSCALL_ERROR | 0x00000003)
#define SYSCALL_SRC_NOT_EXIST  (SYSCALL_ERROR | 0x00000004)

// xhci
#define IRQ_XHCI 0x30

#endif