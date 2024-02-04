#include <kernel/global.h>
#include <device/cpu.h>
#include <task/task.h>
#include <kernel/syscall.h>

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

PUBLIC syscall_status_t ASMLINKAGE sys_sendrecv(uint32_t nr,pid_t src_dest,void *msg)
{
    syscall_status_t res;
    switch (nr)
    {
        case NR_SEND:
            res = msg_send(src_dest,msg);
            break;
        case NR_RECV:
            res = msg_recv(src_dest,msg);
            break;
        case NR_BOTH:
            res = msg_send(src_dest,msg);
            if (res == SYSCALL_SUCCESS)
            {
                res = msg_recv(src_dest,msg);
            }
            break;
        default:
            pr_log("\3 unknow syscall nr: 0x%x",nr);
            res = SYSCALL_NO_SYSCALL;
            break;
    }
    return res;
}

PUBLIC void syscall_entry();
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