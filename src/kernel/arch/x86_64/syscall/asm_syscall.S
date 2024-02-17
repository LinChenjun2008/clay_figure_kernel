.extern get_running_prog_kstack
.global syscall_entry
syscall_entry:
    cli
    // user stack
    pushq %rbp
    pushq %rcx // origin rip
    pushq %r11 // origin rflags
    movq %r10,%rcx // restore rcx

// switch to kernel stack
    pushq %rdi
    pushq %rsi
    pushq %rdx

    leaq get_running_prog_kstack(%rip),%rax
    call *%rax

    popq %rdx
    popq %rsi
    popq %rdi

    movq %rsp,%rbp
    movq %rax,%rsp
    pushq %rbp // save rbp

    leaq sys_sendrecv(%rip),%rax
    callq *%rax

// switch to user stack
    popq %rbx
    movq %rbx,%rsp

    popq %r11
    popq %rcx
    popq %rbp

    sti
    sysretq
