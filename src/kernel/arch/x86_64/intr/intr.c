#include <kernel/global.h>
#include <io.h>
#include <intr.h>
#include <task/task.h>

#include <log.h>

void (*irq_handler[IRQ_CNT])(intr_stack_t*);

PRIVATE char *intr_name[20] =
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

PRIVATE void default_irq_handler(uint8_t nr,intr_stack_t *stack)
{
    pr_log("\n");
    pr_log("\3INTR : 0x%x ( %s )\n",nr,nr < 20 ? intr_name[nr] : "Other Intr");
    uint64_t cr2,cr3;
    __asm__ __volatile__
    (
        "movq %%cr2,%0\n\t""movq %%cr3,%1\n\t":"=r"(cr2),"=r"(cr3)::
    );
    pr_log("CS:RIP %04x:%016x\n"
           "ERROR CODE: %016x\n"
        ,stack->cs,stack->rip,
        stack->error_code);
    pr_log("CR2 - %016x, CR3 - %016x\n",cr2,cr3);
    pr_log("DS  - %016x, ES  - %016x, FS  - %016x, GS  - %016x\n",
            stack->ds,stack->es,stack->fs,stack->gs);
    pr_log("RAX - %016x, PBX - %016x, RCX - %016x, RDX - %016x\n",
            stack->rax,stack->rbx,stack->rcx,stack->rdx);
    pr_log("RSP - %016x, RBP - %016x, RSI - %016x, RDI - %016x\n",
            stack,stack->rbp,stack->rsi,stack->rdi);
    pr_log("R8  - %016x, R9  - %016x, R10 - %016x, R11 - %016x\n",
        stack->r8,stack->r9,stack->r10,stack->r11);
    pr_log("R12 - %016x, R13 - %016x, R14 - %016x, R15 - %016x\n",
           stack->r12,stack->r13,stack->r14,stack->r15);
    if (running_task() != NULL)
    {
        task_struct_t *running = running_task();
        pr_log("running task: %s\n",running->name);
        pr_log("task context: %p\n",running->context);
    }
    while(1);
}

PUBLIC void ASMLINKAGE do_irq(uint8_t nr,intr_stack_t *stack)
{
    if (nr == 0x27)
    {
        return;
    }
    if (irq_handler[nr] != NULL)
    {
        irq_handler[nr](stack);
    }
    else
    {
        default_irq_handler(nr,stack);
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

PUBLIC void intr_init()
{
    idt_desc_init();
    int i;
    for (i = 0;i < IRQ_CNT;i++)
    {
        irq_handler[i] = NULL;
    }
    uint128_t idt_ptr = (((uint128_t)0 + ((uint128_t)((uint64_t)idt))) << 16) \
                        | (sizeof(idt) - 1);
    __asm__ __volatile__ ("lidt %[idt_ptr]"::[idt_ptr]"m"(idt_ptr):);
    return;
}

PUBLIC void register_handle(uint8_t nr,void (*handle)(intr_stack_t*))
{
    irq_handler[nr] = handle;
    return;
}

PUBLIC intr_status_t intr_get_status()
{
    wordsize_t flags;
    __asm__ __volatile__ ("pushf\n\t""popq %q0":"=a"(flags)::"memory");
    return ((flags & 0x00000200) ? INTR_ON : INTR_OFF);
}

PUBLIC intr_status_t intr_set_status(intr_status_t status)
{
    return (status == INTR_ON ? intr_enable() : intr_disable());
}

PUBLIC intr_status_t intr_enable()
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
        __asm__ ("sti \n\t");
        return old_status;
    }
}

PUBLIC intr_status_t intr_disable()
{
    intr_status_t old_status;
    if (intr_get_status() == INTR_ON)
    {
        old_status = INTR_ON;
        __asm__ ("cli \n\t");
        return old_status;
    }
    else
    {
        old_status = INTR_OFF;
        return old_status;
    }
}