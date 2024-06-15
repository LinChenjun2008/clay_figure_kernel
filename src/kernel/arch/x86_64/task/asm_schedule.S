.global asm_switch_to
asm_switch_to:
    /***    next     *** rsp + 8*10 */
    /***     cur     *** rsp + 8* 9 */
    /*** return addr *** rsp + 8* 8 */
    pushq %rsi
    pushq %rdi
    pushq %rbx
    pushq %rbp

    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    /* 栈切换 */
    movq 72(%rsp),%rax
    movq %rsp,(%rax)
    movq 80(%rsp),%rax
    movq (%rax),%rsp

    popq %r15
    popq %r14
    popq %r13
    popq %r12

    popq %rbp
    popq %rbx
    popq %rdi
    popq %rsi
    retq
