// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/desc.h>
#include <asm/ptrace.h>
#include <asm/syscall.h>
#include <asm/utils/regs.h>
#include <asm/x86.h>

#include <panic.h>
#include <syscall.h>

// syscall functions
#include <asm/task/signal.h>

#include <errno.h>
#include <mem.h>
#include <print.h>
#include <syscall/ipc.h>
#include <task.h>
#include <task/fork.h>
#include <task/process.h>
#include <task/signal.h>
#include <task/struct.h>
#include <task/wait.h>

extern ASMLINKAGE void arch_syscall_entry(void);

void *syscall_table[NR_CONT];

static int check_user_address(const void *addr, size_t size)
{
    // 部分情况下NULL属于合法地址
    if (addr == NULL)
    {
        return 0;
    }
    struct task *task = get_current_task();
    ASSERT(IS_USER_PROG(task));

    return mm_check_addr(task, (void *)addr, size, ADDR_ALLOCATED);
}

// 取得用户消息头的副本, 并校验消息头与数据区
// flags: 发送方数据要求已映射(ADDR_MAPPED), 接收方缓冲区允许未映射(ADDR_ALLOCATED)
static int get_msg(struct msg_head *user_msg, struct msg_head *msg, int flags)
{
    int ret = check_user_address(user_msg, sizeof(*user_msg));

    if (ret < 0)
    {
        return ret;
    }
    *msg = *user_msg;

    if (msg->size != 0)
    {
        if (msg->data == NULL)
        {
            return -EINVAL;
        }
        ret = mm_check_addr(get_current_task(), msg->data, msg->size, flags);
        if (ret < 0)
        {
            return ret;
        }
    }
    return 0;
}

static word_t sys_exit(struct pt_regs *regs)
{
    int status = (int)regs->rsi;

    process_exit(status);
    return 0; // process_exit 不返回
}

static word_t sys_fork(struct pt_regs *regs)
{
    (void)regs;
    return (word_t)process_fork();
}

static word_t sys_wait(struct pt_regs *regs)
{
    pid_t pid     = (pid_t)regs->rsi;
    int  *status  = (int *)regs->rdx;
    int   options = (int)regs->r10;
    int   ret;

    ret = check_user_address(status, sizeof(*status));
    if (ret < 0)
    {
        return (word_t)ret;
    }
    return (word_t)task_waitpid(pid, status, options);
}

// 发送消息
// 校验用户参数并构造msg_head副本供ipc_send使用
static word_t sys_send(struct pt_regs *regs)
{
    struct task     *task     = get_current_task();
    struct msg_head *user_msg = (struct msg_head *)regs->rsi;
    pid_t            dst_pid  = (pid_t)regs->rdx;
    struct msg_head  msg;
    int              ret;

    // 发送方的数据由接收方按页转换后读取, 要求已映射
    ret = get_msg(user_msg, &msg, ADDR_MAPPED);
    if (ret < 0)
    {
        return (word_t)ret;
    }

    // 此处为服务进程开后门,便于日志输出
    // 后期将删除,切勿依赖
    if (IS_SERV_TASK(task) && msg.type == MSG_LOG)
    {
        if (msg.data == NULL)
        {
            return (word_t)-EINVAL;
        }
        printk((char *)msg.data);
        return 0;
    }

    return (word_t)ipc_send(&msg, dst_pid);
}

// 接收消息
// 接收缓冲区留用户地址: 数据写到当前任务自己的地址空间, 懒分配/COW由内核缺页处理
static word_t sys_recv(struct pt_regs *regs)
{
    struct msg_head *user_msg = (struct msg_head *)regs->rsi;
    int              option   = (int)regs->rdx;
    struct msg_head  msg;
    int              ret;

    // 接收缓冲区允许未映射, 由内核态缺页补页
    ret = get_msg(user_msg, &msg, ADDR_ALLOCATED);
    if (ret < 0)
    {
        return (word_t)ret;
    }

    ret = ipc_recv(&msg, option);

    // 把接收结果(来源/类型/实际长度)写回用户消息头
    *user_msg = msg;
    return (word_t)ret;
}

static word_t sys_addr(struct pt_regs *regs)
{
    uintptr_t addr  = (uintptr_t)regs->rsi;
    size_t    pages = (size_t)regs->rdx;

    return mm_allocate_address(addr, pages);
}

static word_t sys_free(struct pt_regs *regs)
{
    uintptr_t addr  = (uintptr_t)regs->rsi;
    size_t    pages = (size_t)regs->rdx;

    mm_free_address(addr, pages);
    return 0;
}

static word_t sys_kill(struct pt_regs *regs)
{
    pid_t pid = (pid_t)regs->rsi;
    int   sig = (int)regs->rdx;

    return send_signal_from_user(pid, sig);
}

static word_t sys_sigaction(struct pt_regs *regs)
{
    int               sig    = (int)regs->rsi;
    struct sigaction *act    = (void *)regs->rdx;
    struct sigaction *oldact = (void *)regs->r10;
    int               ret;

    ret = check_user_address(act, sizeof(*act));
    if (ret < 0)
    {
        return (word_t)ret;
    }
    ret = check_user_address(oldact, sizeof(*oldact));
    if (ret < 0)
    {
        return (word_t)ret;
    }
    return (word_t)sigaction(sig, act, oldact);
}

static void register_syscall(uint64_t num, void *func)
{
    if (num >= NR_CONT)
    {
        return;
    }
    syscall_table[num] = func;
    return;
}

// 注册平台相关的系统调用
void arch_syscall_init(void)
{
    int i;
    for (i = 0; i < NR_CONT; i++)
    {
        syscall_table[i] = NULL;
    }
    register_syscall(NR_EXIT, sys_exit);
    register_syscall(NR_FORK, sys_fork);
    register_syscall(NR_WAIT, sys_wait);
    register_syscall(NR_SEND, sys_send);
    register_syscall(NR_RECV, sys_recv);
    register_syscall(NR_ADDR, sys_addr);
    register_syscall(NR_FREE, sys_free);
    register_syscall(NR_KILL, sys_kill);
    register_syscall(NR_SACT, sys_sigaction);
    register_syscall(NR_SRET, sys_sigret);
    return;
}

void arch_syscall_enable(void)
{
    uint64_t msr = 0;

    msr = rdmsr(IA32_EFER);
    msr |= IA32_EFER_SCE;
    wrmsr(IA32_EFER, msr);

    wrmsr(IA32_LSTAR, (uint64_t)arch_syscall_entry);

    // IA32_STAR[63:48] + 16 = user CS,IA32_STAR[63:48] + 8 = user SS
    msr = ((uint64_t)SELECTOR_KERNEL_CODE64 << 32);
    msr |= ((uint64_t)(SELECTOR_USER_CODE64 - 16) << 48);

    wrmsr(IA32_STAR, msr);
    wrmsr(IA32_FMASK, EFLAGS_IF_1);
    return;
}

void syscall_entry(struct pt_regs *regs);
void syscall_entry(struct pt_regs *regs)
{
    ASSERT(IS_USER_PROG(get_current_task()));

    word_t func = regs->rdi;
    if (func >= NR_CONT)
    {
        printk(MSG_WARN "syscall %#llx not supported.\n", func);
        regs->rax = (word_t)-1;
        return;
    }
    if (syscall_table[func] == NULL)
    {
        printk(MSG_WARN "syscall %#llx not supported.\n", func);
        regs->rax = (word_t)-2;
        return;
    }

    word_t (*sys_func)(struct pt_regs *);
    sys_func   = syscall_table[func];
    word_t ret = sys_func(regs);
    regs->rax  = (word_t)ret;

    signal_check(regs);
    return;
}