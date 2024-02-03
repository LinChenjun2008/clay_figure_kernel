#include <kernel/global.h>
#include <task/task.h>
#include <intr.h>

extern list_t task_list;

PRIVATE void switch_to(task_context_t **cur,task_context_t **next)
{
    __asm__ __volatile__
    (
        "pushq %1 \n\t"
        "pushq %0 \n\t"
        "leaq asm_switch_to(%%rip),%%rax \n\t"
        "callq *%%rax \n\t"
        :
        :"g"(cur),"g"(next)
    );
}

PUBLIC void schedule()
{
    task_struct_t *cur_task = running_task();
    if (cur_task->status == TASK_RUNNING)
    {
        list_append(&task_list,&cur_task->general_tag);
        cur_task->ticks = cur_task->priority;
        cur_task->status = TASK_READY;
    }
    task_struct_t *next = NULL;
    if (list_empty(&task_list))
    {
        // thread_unblock(idle_thread);
    }
    list_node_t *next_task_tag = NULL;
    next_task_tag = list_pop(&task_list);
    next = CONTAINER_OF(task_struct_t,general_tag,next_task_tag);
    next->status = TASK_RUNNING;

    // process_activate(next);
    // fpu_set(cur_thread,next);
    switch_to(&cur_task->context,&next->context);
    return;
}

__asm__
(
    "asm_switch_to: \n\t"
    /***    next     *** rsp + 8*10 */
    /***     cur     *** rsp + 8* 9 */
    /*** return addr *** rsp + 8* 8 */
    "pushq %rsi \n\t"
    "pushq %rdi \n\t"
    "pushq %rbx \n\t"
    "pushq %rbp \n\t"

    "pushq %r12 \n\t"
    "pushq %r13 \n\t"
    "pushq %r14 \n\t"
    "pushq %r15 \n\t"

    /* 栈切换 */
    "movq 72(%rsp),%rax \n\t"
    "movq %rsp,(%rax) \n\t"
    "movq 80(%rsp),%rax \n\t"
    "movq (%rax),%rsp \n\t"

    "popq %r15 \n\t"
    "popq %r14 \n\t"
    "popq %r13 \n\t"
    "popq %r12 \n\t"

    "popq %rbp \n\t"
    "popq %rbx \n\t"
    "popq %rdi \n\t"
    "popq %rsi \n\t"
    "retq \n\t"
);
