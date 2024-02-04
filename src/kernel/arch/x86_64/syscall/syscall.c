#include <kernel/global.h>
#include <device/cpu.h>

#include <task/task.h>
#include <log.h>

PUBLIC void syscall(uint32_t nr,pid_t src_dest,void *msg)
{
    __asm__ __volatile__
    (
        "movq %0,%%rdi \n\t"
        "movq %1,%%rsi \n\t"
        "movq %2,%%rdx \n\t"

        "pushq %%r10 \n\t"
        "movq %%rcx,%%r10 \n\t" // save rcx in r10
        "syscall \n\t"
        "popq %%r10 \n\t"
    :
    :"r"((wordsize_t)nr),"r"((wordsize_t)src_dest),"r"((wordsize_t)msg)
    );
}

void syscall_entry();
__asm__
(
    ".global syscall_entry \n\t"
    "syscall_entry: \n\t"
        "cli \n\t"
        // user stack
        "pushq %rbp \n\t"
        "pushq %rcx \n\t" // origin rip
        "pushq %r11 \n\t" // origin rflags
        "movq %r10,%rcx \n\t" // restore rcx

    // switch to kernel stack
        "pushq %rdi \n\t"
        "pushq %rsi \n\t"
        "pushq %rdx \n\t"

        "leaq get_running_prog_kstack(%rip),%rax \n\t"
        "call *%rax \n\t"

        "popq %rdx \n\t"
        "popq %rsi \n\t"
        "popq %rdi \n\t"

        "movq %rsp,%rbp \n\t"
        "movq %rax,%rsp \n\t"
        "pushq %rbp \n\t" // save rbp

        "leaq sys_sendrecv(%rip),%rax \n\t"
        "callq *%rax \n\t"

    // switch to user stack
        "popq %rbx \n\t"
        "movq %rbx,%rsp \n\t"

        "popq %r11 \n\t"
        "popq %rcx \n\t"
        "popq %rbp \n\t"

        "sti \n\t"
        "sysretq \n\t"
);

PUBLIC void ASMLINKAGE sys_sendrecv(uint32_t nr,pid_t src_dest,void *msg)
{
    pr_log("\2syscall!! ");
    pr_log(" task name is %s\n",running_task()->name);
    pr_log("\2 Nr: %d, src_dest %d,msg %p\n",nr,src_dest,msg);
    return;
}

PUBLIC void syscall_init()
{
    wordsize_t val;
    val = rdmsr(IA32_EFER);
    val |= IA32_EFER_SCE;
    wrmsr(IA32_EFER,val);

    wrmsr(IA32_LSTAR,(uint64_t)syscall_entry);
    // IA32_STAR[63:48] + 16 = user CS,IA32_STAR[63:48] + 8 = user SS
    wrmsr(IA32_STAR,(uint64_t)SELECTOR_CODE64_K << 32 | (uint64_t)(SELECTOR_CODE64_U - 16) << 48);
    wrmsr(IA32_FMASK,0);
    return;
}