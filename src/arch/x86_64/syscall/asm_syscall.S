/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPLv3-or-later

*/

.global send_recv
send_recv:
    pushq %r10
    movq %rcx, %r10 // save rcx in r10
    syscall
    popq %r10
    retq

.extern get_running_prog_kstack
.global syscall_entry
syscall_entry:
    // user stack
    pushq %rbp
    pushq %rcx // origin rip
    pushq %r11 // origin rflags
    movq %r10, %rcx // restore rcx

    // switch to kernel stack
    pushq %rdi
    pushq %rsi
    pushq %rdx

    leaq get_running_prog_kstack(%rip), %rax
    call *%rax

    popq %rdx
    popq %rsi
    popq %rdi

    movq %rsp, %rbp
    movq %rax, %rsp

    pushq %rbp // save rbp

    sti

    leaq sys_send_recv(%rip), %rax
    callq *%rax

    cli

    // switch to user stack
    popq %rbp
    movq %rbp, %rsp

    popq %r11
    popq %rcx
    popq %rbp

    sysretq
