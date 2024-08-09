#include <kernel/global.h>
#include <task/task.h>
#include <device/cpu.h>
#include <intr.h>

extern taskmgr_t tm;

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
    if (cur_task->spinlock_count > 0)
    {
        return;
    }
    if (cur_task->status == TASK_RUNNING)
    {
        spinlock_lock(&tm.task_list_lock[apic_id()]);
        list_append(&tm.task_list[apic_id()],&cur_task->general_tag);
        spinlock_unlock(&tm.task_list_lock[apic_id()]);
        cur_task->ticks = 0;
        cur_task->status = TASK_READY;
    }
    task_struct_t *next = NULL;
    list_node_t *next_task_tag = NULL;

    spinlock_lock(&tm.task_list_lock[apic_id()]);
    next_task_tag = list_pop(&tm.task_list[apic_id()]);
    spinlock_unlock(&tm.task_list_lock[apic_id()]);
    next = CONTAINER_OF(task_struct_t,general_tag,next_task_tag);
    next->status = TASK_RUNNING;

    prog_activate(next);
    // fpu_set(cur_task,next);
    next->cpu_id = apic_id();
    switch_to(&cur_task->context,&next->context);
    return;
}

PUBLIC void task_block(task_status_t status)
{
    intr_status_t intr_status = intr_disable();
    task_struct_t *cur_task = running_task();
    cur_task->status = status;
    schedule();
    intr_set_status(intr_status);
    return;
}

PUBLIC void task_unblock(pid_t pid)
{
    task_struct_t *task = pid2task(pid);
    if (task == NULL)
    {
        return;
    }
    intr_status_t intr_status = intr_disable();
    if (task->status != TASK_READY)
    {
        spinlock_lock(&tm.task_list_lock[task->cpu_id]);
        list_push(&tm.task_list[task->cpu_id],&task->general_tag);
        spinlock_unlock(&tm.task_list_lock[task->cpu_id]);
        task->status = TASK_READY;
    };
    intr_set_status(intr_status);
    return;
}