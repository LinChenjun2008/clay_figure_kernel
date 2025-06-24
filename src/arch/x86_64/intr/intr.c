/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <intr.h>
#include <io.h>               // get_flags
#include <task/task.h>        // task_struct_t,running_task
#include <device/cpu.h>       // cpuid
#include <service.h>          // MM_EXIT
#include <kernel/syscall.h>   // sys_send_recv
#include <device/spinlock.h>  // spinlock_t,spinlock_lock,spinlock_unlock
#include <kernel/symbols.h>   // addr_to_symbol

#include <log.h>

void (*irq_handler[IRQ_CNT])(intr_stack_t*);

PRIVATE const char *intr_name[20] =
{
    "#DE","#DB","NMI","#BP","#OF",
    "#BR","#UD","#NM","#DF","CSO",
    "#TS","#NP","#SS","#GP","#PF",
    "RSV","#MF","#AC","#MC","#XF",
};

#pragma pack(1)
typedef struct
{
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  attribute;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} gate_desc_t;
#pragma pack()

PRIVATE gate_desc_t idt[IRQ_CNT];
PRIVATE spinlock_t intr_lock;

PRIVATE void set_gatedesc(gate_desc_t *gd,void *func,int selector,int ar)
{
    gd->offset_low = ((uint64_t)func) & 0x000000000000ffff;
    gd->selector = selector;
    gd->ist = 0;
    gd->attribute = (ar & 0xff);
    gd->offset_mid = (uint32_t)((((uint64_t)func) >> 16) & 0x000000000000ffff);
    gd->offset_high = (uint32_t)((((uint64_t)func) >> 32) & 0x00000000ffffffff);
    return;
}

PRIVATE void pr_debug_info(intr_stack_t *stack)
{
    pr_log(0,"\n");
    int i;
    for (i = 0;i < 19;i++)
    {
        pr_log(0,"=====");
    }
    pr_log(0,"\n");
    pr_log(0,"Registers:\n\n");
    uint64_t cr2,cr3;
    cr2 = get_cr2();
    cr3 = get_cr3();
    pr_log(0,"CS:RIP %04x:%p\n"
           "ERROR CODE: %016x "
        ,stack->cs,stack->rip,
        stack->error_code);
    pr_log(0,"\n");
    pr_log(0,"CR2 - %016lx, CR3 - %016lx\n",cr2,cr3);
    pr_log(0,"DS  - %016lx, ES  - %016lx, FS  - %016lx, GS  - %016lx\n",
            stack->ds,stack->es,stack->fs,stack->gs);
    pr_log(0,"RAX - %016lx, RBX - %016lx, RCX - %016lx, RDX - %016lx\n",
            stack->rax,stack->rbx,stack->rcx,stack->rdx);
    pr_log(0,"RSP - %016lx, RBP - %016lx, RSI - %016lx, RDI - %016lx\n",
            stack->rsp,stack->rbp,stack->rsi,stack->rdi);
    pr_log(0,"R8  - %016lx, R9  - %016lx, R10 - %016lx, R11 - %016lx\n",
        stack->r8,stack->r9,stack->r10,stack->r11);
    pr_log(0,"R12 - %016lx, R13 - %016lx, R14 - %016lx, R15 - %016lx\n",
           stack->r12,stack->r13,stack->r14,stack->r15);
    uint32_t a,b,c,d;
    asm_cpuid(1,0,&a,&b,&c,&d);
    b >>= 24;
    pr_log(0,"CPUID: %d\n",b);

    PANIC(running_task() == NULL,"Can not Get Running Task.");

    task_struct_t *running = running_task();
    pr_log(0,"running task: %s\n",running->name);
    pr_log(0,"task context: %p\n",running->context);

    // Backtrace
    for (i = 0;i < 19;i++)
    {
        pr_log(0,"=====");
    }
    pr_log(0,"\nKernel Stack Backtrace:\n\n");
    int sym_idx;
    addr_t *rip = (addr_t*)stack->rip;
    addr_t *rbp = (addr_t*)stack->rbp;
    for (i = 0;i < 8;i++)
    {
        status_t status = get_symbol_index_by_addr(rip,&sym_idx);
        if (ERROR(status))
        {
            break;
        }
        pr_log(0,"    At address: %p [ %s + %#x ]\n",
                rip,
                index_to_symbol(sym_idx),
                (addr_t)rip - (addr_t)index_to_addr(sym_idx));
        rip = (addr_t*)*(rbp + 1);
        rbp = (addr_t*)*rbp;
    }
    for (i = 0;i < 19;i++)
    {
        pr_log(0,"=====");
    }
    pr_log(0,"\n");
}

PRIVATE void default_irq_handler(intr_stack_t *stack)
{
    int int_vector = stack->int_vector;
    if (int_vector == 0x27)
    {
        return;
    }
    spinlock_lock(&intr_lock);
    pr_log(0,"\n");
    pr_log(0,"INTR : 0x%x",int_vector);
    if (int_vector < 20)
    {
        pr_log(0,": %s",intr_name[int_vector]);
    }
    pr_log(0,"\n");
    spinlock_unlock(&intr_lock);
    pr_debug_info(stack);
    task_struct_t *running = running_task();
    if (running->page_dir != NULL)
    {
        message_t msg;
        msg.type = MM_EXIT;
        msg.m1.i1 = K_ERROR;
        sys_send_recv(NR_BOTH,MM,&msg);
    }
    while (1) continue;
}

PUBLIC void ASMLINKAGE do_irq(intr_stack_t *stack)
{
    int int_vector = stack->int_vector;
    if (irq_handler[int_vector])
    {
        irq_handler[int_vector](stack);
    }
    else
    {
        default_irq_handler(stack);
    }
    return;
}

#define INTR_HANDLER(ENTRY,NR,ERROR_CODE) extern void ENTRY(intr_stack_t*);
#include <intr.h>
#undef INTR_HANDLER

PRIVATE void idt_desc_init(void)
{
    #define INTR_HANDLER(ENTRY,NR,ERROR_CODE) \
            set_gatedesc(&idt[NR],ENTRY,SELECTOR_CODE64_K,AR_IDT_DESC_DPL0);
    #include <intr.h>
    #undef INTR_HANDLER
}

extern void asm_lidt(void *idt_ptr);
PUBLIC void intr_init(void)
{
    idt_desc_init();
    int i;
    for (i = 0;i < IRQ_CNT;i++)
    {
        irq_handler[i] = default_irq_handler;
    }
    uint128_t idt_ptr = (((uint128_t)0 + ((uint128_t)((uint64_t)idt))) << 16) \
                        | (sizeof(idt) - 1);
    asm_lidt(&idt_ptr);
    init_spinlock(&intr_lock);

    irq_handler[0x82] = pr_debug_info;
    return;
}

PUBLIC void ap_intr_init(void)
{
    uint128_t idt_ptr = (((uint128_t)0 + ((uint128_t)((uint64_t)idt))) << 16) \
                        | (sizeof(idt) - 1);
    asm_lidt(&idt_ptr);
}

PUBLIC void register_handle(uint8_t int_vector,void (*handle)(intr_stack_t*))
{
    irq_handler[int_vector] = handle;
    return;
}

PUBLIC intr_status_t intr_get_status(void)
{
    wordsize_t flags;
    flags = get_flags();
    return ((flags & 0x00000200) ? INTR_ON : INTR_OFF);
}

PUBLIC intr_status_t intr_set_status(intr_status_t status)
{
    return (status == INTR_ON ? intr_enable() : intr_disable());
}

PUBLIC intr_status_t intr_enable(void)
{
    intr_status_t old_status;
    if (intr_get_status() == INTR_ON)
    {
        old_status = INTR_ON;
        return old_status;
    }
    else
    {
        old_status = INTR_OFF;
        io_sti();
        return old_status;
    }
}

PUBLIC intr_status_t intr_disable(void)
{
    intr_status_t old_status;
    if (intr_get_status() == INTR_ON)
    {
        old_status = INTR_ON;
        io_cli();
        return old_status;
    }
    else
    {
        old_status = INTR_OFF;
        return old_status;
    }
}