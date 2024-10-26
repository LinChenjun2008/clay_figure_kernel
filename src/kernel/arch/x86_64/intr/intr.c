/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.

Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 通用公共许可证，了解详情。

你应该随程序获得一份 GNU 通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

#include <kernel/global.h>
#include <intr.h>
#include <task/task.h>      // task_struct_t,running_task
#include <device/cpu.h>     // cpuid
#include <service.h>        // MM_EXIT
#include <kernel/syscall.h> // sys_send_recv

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
    pr_log("\3 INTR : 0x%x ( %s )\n",nr,nr < 20 ? intr_name[nr] : "Unknow");
    uint64_t cr2,cr3;
    __asm__ __volatile__(
        "movq %%cr2,%0\n\t""movq %%cr3,%1\n\t":"=r"(cr2),"=r"(cr3)::);
    pr_log("CS:RIP %04x:%016x\n"
           "ERROR CODE: %016x "
        ,stack->cs,stack->rip,
        stack->error_code);
    if (stack->error_code != 0)
    {
        pr_log("(");
        if (stack->error_code & (1 << 2))
        {
            pr_log(" TI");
        }
        if (stack->error_code & (1 << 1))
        {
            pr_log(" IDT");
        }
        if (stack->error_code & (1 << 0))
        {
            pr_log(" EXT");
        }
        pr_log(" ) ");
        pr_log("Selector: %04x",stack->error_code & 0xfff8);
    }
    pr_log("\n");
    pr_log("CR2 - %016x, CR3 - %016x\n",cr2,cr3);
    pr_log("DS  - %016x, ES  - %016x, FS  - %016x, GS  - %016x\n",
            stack->ds,stack->es,stack->fs,stack->gs);
    pr_log("RAX - %016x, RBX - %016x, RCX - %016x, RDX - %016x\n",
            stack->rax,stack->rbx,stack->rcx,stack->rdx);
    pr_log("RSP - %016x, RBP - %016x, RSI - %016x, RDI - %016x\n",
            stack->rsp,stack->rbp,stack->rsi,stack->rdi);
    pr_log("R8  - %016x, R9  - %016x, R10 - %016x, R11 - %016x\n",
        stack->r8,stack->r9,stack->r10,stack->r11);
    pr_log("R12 - %016x, R13 - %016x, R14 - %016x, R15 - %016x\n",
           stack->r12,stack->r13,stack->r14,stack->r15);
    uint32_t a,b,c,d;
    cpuid(1,0,&a,&b,&c,&d);
    b >>= 24;
    pr_log("CPUID: %x\n",b);
    if (running_task() == NULL)
    {
        PANIC("Can not Get Running Task.");
    }
    task_struct_t *running = running_task();
    pr_log("running task: %s\n",running->name);
    pr_log("task context: %p\n",running->context);
    if (running->page_dir != NULL)
    {
        message_t msg;
        msg.type = MM_EXIT;
        msg.m1.i1 = K_ERROR;
        sys_send_recv(NR_BOTH,MM,&msg);
    }
    PANIC("Kernel Error.");
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

PUBLIC void ap_intr_init()
{
    uint128_t idt_ptr = (((uint128_t)0 + ((uint128_t)((uint64_t)idt))) << 16) \
                        | (sizeof(idt) - 1);
    __asm__ __volatile__ ("lidt %[idt_ptr]"::[idt_ptr]"m"(idt_ptr):);
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