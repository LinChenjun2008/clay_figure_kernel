/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.

Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU Lesser General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 宽通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 宽通用公共许可证，了解详情。

你应该随程序获得一份 GNU 宽通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

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