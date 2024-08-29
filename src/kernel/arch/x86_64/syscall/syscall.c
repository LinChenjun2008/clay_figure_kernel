#include <kernel/global.h>
#include <device/cpu.h>     // wrmsr,rdmsr
#include <kernel/syscall.h> // msg_send,msg_recv
#include <service.h>        // is_service_id,service_id_to_pid

#include <log.h>

PUBLIC syscall_status_t ASMLINKAGE sys_send_recv(
    uint32_t nr,
    pid_t src_dest,
    message_t *msg)
{
    if (is_service_id(src_dest))
    {
        src_dest = service_id_to_pid(src_dest);
    }
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
    if (res != SYSCALL_SUCCESS)
    {
        pr_log("\3 syscall error: %x\n",res);
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
    wrmsr(IA32_STAR,(uint64_t)SELECTOR_CODE64_K << 32
          | (uint64_t)(SELECTOR_CODE64_U - 16) << 48);
    wrmsr(IA32_FMASK,EFLAGS_IF_1);
    return;
}