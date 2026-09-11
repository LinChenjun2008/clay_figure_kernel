// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __LIB_H__
#define __LIB_H__

#include <types.h>

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
#define sa_sigaction __sigaction_handler.sa_sigaction

struct message
{
    pid_t    source;
    uint32_t type;
    union
    {
        uint32_t m32[14];
        uint64_t m64[7];
    };
};

// syscall
uint64_t syscall_0(uint64_t);
uint64_t syscall_1(uint64_t, uint64_t);
uint64_t syscall_2(uint64_t, uint64_t, uint64_t);
uint64_t syscall_3(uint64_t, uint64_t, uint64_t, uint64_t);
uint64_t syscall_4(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
uint64_t syscall_5(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

void  exit(int status);
pid_t fork(void);
pid_t waitpid(pid_t pid, int *status, int options);
pid_t wait(pid_t pid, int *status);

pid_t send(pid_t dst, struct message *msg);
pid_t recv(pid_t src, struct message *msg);
pid_t both(pid_t src_dst, struct message *msg);
int   kill(pid_t pid, int sig);
int   sigaction(int sig, const struct sigaction *act, struct sigaction *oldact);

#endif /* __LIB_H__ */