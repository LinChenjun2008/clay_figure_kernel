// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __TASK_SIGNAL_H__
#define __TASK_SIGNAL_H__

#include <asm/ptrace.h>

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

union sigval
{
    int   sival_int;
    void *sival_ptr;
};

typedef struct siginfo
{
    int          si_signo;
    int          si_errno;
    int          si_code;
    pid_t        si_pid;
    int          rsvd; // uid_t si_uid;
    void        *si_addr;
    int          si_status;
    union sigval si_value;
} siginfo_t;

typedef uint64_t sigset_t;

struct sigaction
{
    union
    {
        void (*sa_handler)(int);
        void (*sa_sigaction)(int, siginfo_t *, void *);
    } __sigaction_handler;
    sigset_t sa_mask;
    int      sa_flags;
    void (*sa_restorer)(void);
};

#define sa_handler   __sigaction_handler.sa_handler
#define sa_sigaction __sigaction_handler.sa_sigaction

/// TODO: 备用信号栈(sigaltstack)
typedef struct
{
    void  *ss_sp;
    int    ss_flags;
    size_t ss_size;
} stack_t;

// 用户态机器上下文(uc_mcontext)
struct sigcontext
{
    word_t r15;
    word_t r14;
    word_t r13;
    word_t r12;
    word_t r11;
    word_t r10;
    word_t r9;
    word_t r8;

    word_t rdi;
    word_t rsi;
    word_t rbp;
    word_t rbx;
    word_t rdx;
    word_t rax;
    word_t rcx;

    word_t rip;
    word_t rflags;
    word_t rsp;

    word_t cs;
    word_t ss;
    word_t ds;
    word_t es;
    word_t fs;
    word_t gs;
};

typedef struct ucontext
{
    uint64_t          uc_flags;
    struct ucontext  *uc_link;
    stack_t           uc_stack;
    struct sigcontext uc_mcontext;
    sigset_t          uc_sigmask;
} ucontext_t;

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

#define SIGNAL_PENDING(task) ((task)->signal.pending & ~(task)->signal.blocked)

void init_signal(struct signal_struct *signal);
void copy_signal(struct signal_struct *dst, struct signal_struct *src);
int  send_signal(pid_t pid, int sig, struct siginfo *info);
int  send_signal_from_user(pid_t pid, int sig);
int  send_signal_fault(pid_t pid, int sig, int code, void *addr);
int  send_signal_child(pid_t pid, int code, pid_t child, int status);

#endif /* __TASK_SIGNAL_H__ */