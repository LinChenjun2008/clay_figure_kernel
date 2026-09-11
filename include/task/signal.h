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
#define sa_sigcation __sigaction_handler.sa_sigaction

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
    struct pt_regs regs;
    uint8_t        trampoline[16];
};

#define SIGNAL_PENDING(task) ((task)->signal.pending & ~(task)->signal.blocked)

void init_signal(struct signal_struct *signal);
void copy_signal(struct signal_struct *dst, struct signal_struct *src);
int  send_signal(pid_t pid, int sig, struct siginfo *info);

#endif /* __TASK_SIGNAL_H__ */