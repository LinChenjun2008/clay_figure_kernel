// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <device/cpu.h>     // wrmsr,rdmsr,IA32_EFER
#include <kernel/syscall.h> // msg_send,msg_recv
#include <service.h>        // is_service_id,service_id_to_pid

PUBLIC syscall_status_t ASMLINKAGE
sys_send_recv(uint32_t function, pid_t src_dst, message_t *msg)
{
    if (is_service_id(src_dst))
    {
        src_dst = service_id_to_pid(src_dst);
    }
    if (function & 0x7ffffffc)
    {
        PR_LOG(LOG_WARN, "unknow syscall nr: 0x%x", function);
        return SYSCALL_NO_SYSCALL;
    }
    syscall_status_t ret = SYSCALL_SUCCESS;

    uint32_t func_send = function & FUNC_SEND;
    uint32_t func_recv = function & FUNC_RECV;

    if (func_send)
    {
        if (src_dst == SEND_TO_KERNEL)
        {
            ret = kernel_services(msg);
        }
        else
        {
            ret = msg_send(src_dst, msg);
        }
    }

    if (func_recv && ret == SYSCALL_SUCCESS)
    {
        ret = msg_recv(src_dst, msg);
    }

    if (ret != SYSCALL_SUCCESS)
    {
        PR_LOG(LOG_WARN, "syscall error: %#x\n", ret);
    }
    return ret;
}

PUBLIC void syscall_entry(void);
PUBLIC void syscall_init(void)
{
    uint64_t val;
    val = rdmsr(IA32_EFER);
    val |= IA32_EFER_SCE;
    wrmsr(IA32_EFER, val);

    wrmsr(IA32_LSTAR, (uint64_t)syscall_entry);

    // IA32_STAR[63:48] + 16 = user CS,IA32_STAR[63:48] + 8 = user SS
    val = ((uint64_t)SELECTOR_CODE64_K << 32) |
          ((uint64_t)(SELECTOR_CODE64_U - 16) << 48);

    wrmsr(IA32_STAR, val);
    wrmsr(IA32_FMASK, EFLAGS_IF_1);
    return;
}