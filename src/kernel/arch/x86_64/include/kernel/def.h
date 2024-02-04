#ifndef __DEF_H__
#define __DEF_H__

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef PUBLIC
#define PUBLIC
#endif

#ifndef PRIVATE
#define PRIVATE static
#endif

typedef int bool;
#ifndef TRUE
#define TRUE  (1 == 1)
#endif

#ifndef FALSE
#define FALSE (1 == 0)
#endif

#define KERNEL_PAGE_DIR_TABLE_POS 0x00000000005f9000
#define KERNEL_VMA_BASE           0xffff800000000000

#define KADDR_P2V(ADDR) ((void*)((uintptr_t)(ADDR) + KERNEL_VMA_BASE))
#define KADDR_V2P(ADDR) ((void*)((uintptr_t)(ADDR) - KERNEL_VMA_BASE))

#define ADDR_PML4T_INDEX(ADDR) ((((uintptr_t)(ADDR)) >> 39) & 0x1ff)
#define ADDR_PDPT_INDEX(ADDR)  ((((uintptr_t)(ADDR)) >> 30) & 0x1ff)
#define ADDR_PDT_INDEX(ADDR)   ((((uintptr_t)(ADDR)) >> 21) & 0x1ff)
#define ADDR_OFFSET(ADDR)      ((((uintptr_t)(ADDR)) & 0x1fffff))

#define ASMLINKAGE __attribute__((sysv_abi))

#define DIV_ROUND_UP(X ,STEP) (((X) + (STEP - 1)) / STEP)

#define PADDR_AVAILABLE(ADDR) (ADDR <= 0x00007fffffffffff)

typedef signed char          int8_t;
typedef signed short         int16_t;
typedef signed int           int32_t;
typedef signed long long int int64_t;
typedef signed __int128      int128_t;

typedef unsigned char          uint8_t;
typedef unsigned short         uint16_t;
typedef unsigned int           uint32_t;
typedef unsigned long long int uint64_t;
typedef unsigned __int128      uint128_t;

typedef unsigned long long int uintptr_t;
typedef unsigned long long int wordsize_t;
typedef unsigned long long int size_t;

typedef uint16_t pid_t;

typedef uint32_t syscall_status_t;

typedef struct
{
    pid_t    src;
    uint32_t type;
    uint64_t msg[8];
} message_t;

#endif