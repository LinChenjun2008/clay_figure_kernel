// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __TASK_SIGNAL_H__
#define __TASK_SIGNAL_H__

#include <sync/spinlock.h>

#define SIGHUP    1
#define SIGINT    2
#define SIGQUIT   3
#define SIGILL    4
#define SIGTRAP   5
#define SIGABRT   6
#define SIGBUS    7
#define SIGFPE    8
#define SIGKILL   9
#define SIGUSR1   10
#define SIGSEGV   11
#define SIGUSR2   12
#define SIGPIPE   13
#define SIGALRM   14
#define SIGTERM   15
#define SIGTKFLT  16
#define SIGCHLD   17
#define SIGCONT   18
#define SIGSTOP   19
#define SIGTSTP   20
#define SIGTITN   21
#define SIGTTOU   22
#define SIGURG    23
#define SIGXCPU   24
#define SIGXFSZ   25
#define SIGVTALRM 26
#define SIGPROF   27
#define SIGWINCH  28
#define SIGIO     29
#define SIGPWR    30

#define SIG_DFL ((void (*)(int))0)
#define SIG_IGN ((void (*)(int))1)

// struct sigaction.sa_flags
#define SA_SIGINFO 0x00000004

// stack_t.ss_flags
#define SS_ONSTACK 1
#define SS_DISABLE 2

// struct siginfo.si_code
#define SI_USER   0    // kill() / raise()
#define SI_QUEUE  (-1) // sigqueue()
#define SI_KERNEL 0x80 // 内核产生(如 SIGSEGV / SIGCHLD)

// struct siginfo.si_code: 子类型
#define CLD_EXITED  1 // SIGCHLD: 正常退出
#define CLD_KILLED  2 // SIGCHLD: 被信号杀死
#define SEGV_MAPERR 1 // SIGSEGV: 地址未映射
#define SEGV_ACCERR 2 // SIGSEGV: 权限不符
#define ILL_ILLOPC  1 // SIGILL:  非法操作码
#define FPE_INTDIV  1 // SIGFPE:  整数除零
#define BUS_ADRALN  1 // SIGBUS:  地址未对齐
#define TRAP_BRKPT  1 // SIGTRAP: 断点

#define SIGNAL_PENDING(task) ((task)->signal.pending & ~(task)->signal.blocked)

struct signal_struct
{
    struct spinlock  lock;
    uint32_t         pending;
    uint32_t         blocked;
    struct sigaction actions[32];
    struct siginfo   info[32];
};

struct sigframe
{
    word_t         retcode;
    uint32_t       signum;
    uint32_t       blocked;
    struct siginfo info;
    ucontext_t     uc;
    uint8_t        trampoline[16];
};

void init_signal(struct signal_struct *signal);
void copy_signal(struct signal_struct *dst, struct signal_struct *src);
int  send_signal(pid_t pid, int sig, struct siginfo *info);
int  send_signal_from_user(pid_t pid, int sig);
int  send_signal_fault(pid_t pid, int sig, int code, void *addr);
int  send_signal_child(pid_t pid, int code, pid_t child, int status);

#endif /* __TASK_SIGNAL_H__ */