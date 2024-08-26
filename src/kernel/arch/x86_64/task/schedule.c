#include <kernel/global.h>
#include <task/task.h>
#include <device/cpu.h>
#include <device/sse.h>
#include <intr.h>

#include <log.h>

extern taskmgr_t *tm;

PRIVATE void switch_to(task_context_t **cur,task_context_t **next)
{
    __asm__ __volatile__ (
        "pushq %1 \n\t"
        "pushq %0 \n\t"
        "leaq asm_switch_to(%%rip),%%rax \n\t"
        "callq *%%rax \n\t"
        :
        :"g"(cur),"g"(next));
}

PUBLIC void do_schedule()
{
    ASSERT(intr_get_status() == INTR_OFF);
    task_struct_t *cur_task = running_task();
    cur_task->jiffies   += 1;
    cur_task->vrun_time += cur_task->priority;
    if (cur_task->jiffies >= cur_task->priority)
    {
        schedule();
    }
    return;
}

PUBLIC void schedule()
{
    task_struct_t *cur_task = running_task();
    uint32_t cpu_id = apic_id();
    if (cur_task->spinlock_count > 0)
    {
        return;
    }
    if (cur_task->status == TASK_RUNNING)
    {
        spinlock_lock(&tm->task_list_lock[cpu_id]);
        task_list_insert(&tm->task_list[cpu_id],cur_task);
        spinlock_unlock(&tm->task_list_lock[cpu_id]);
        cur_task->jiffies = 0;
        cur_task->status  = TASK_READY;
    }
    task_struct_t *next = NULL;
    list_node_t *next_task_tag = NULL;

    spinlock_lock(&tm->task_list_lock[cpu_id]);
    if (list_empty(&tm->task_list[cpu_id]))
    {
        spinlock_unlock(&tm->task_list_lock[cpu_id]);
        task_unblock(tm->idle_task[cpu_id]);
        spinlock_lock(&tm->task_list_lock[cpu_id]);
    }
    next_task_tag = list_pop(&tm->task_list[cpu_id]);
    spinlock_unlock(&tm->task_list_lock[cpu_id]);
    ASSERT(next_task_tag != NULL);
    next = CONTAINER_OF(task_struct_t,general_tag,next_task_tag);
    next->status = TASK_RUNNING;
    prog_activate(next);
    fxsave(cur_task->fxsave_region);
    fxrstor(next->fxsave_region);
    next->cpu_id = cpu_id;
    switch_to(&cur_task->context,&next->context);
    return;
}

PUBLIC void task_block(task_status_t status)
{
    intr_status_t intr_status = intr_disable();
    task_struct_t *cur_task = running_task();
    ASSERT(cur_task->spinlock_count == 0);
    cur_task->status = status;
    schedule();
    intr_set_status(intr_status);
    return;
}

PUBLIC void task_unblock(pid_t pid)
{
    task_struct_t *task = pid2task(pid);
    ASSERT(task != NULL);
    ASSERT(task->status != TASK_READY);

    intr_status_t intr_status = intr_disable();
    spinlock_lock(&tm->task_list_lock[task->cpu_id]);

    task->vrun_time = running_task()->vrun_time;

    task_list_insert(&tm->task_list[task->cpu_id],task);
    task->status = TASK_READY;
    ASSERT(list_find(&tm->task_list[task->cpu_id],&task->general_tag));
    spinlock_unlock(&tm->task_list_lock[task->cpu_id]);
    intr_set_status(intr_status);
    return;
}

PUBLIC void task_yield()
{
    task_struct_t *cur_task = running_task();
    intr_status_t intr_status = intr_disable();
    spinlock_lock(&tm->task_list_lock[cur_task->cpu_id]);
    task_list_insert(&tm->task_list[cur_task->cpu_id],cur_task);
    cur_task->status = TASK_READY;
    spinlock_unlock(&tm->task_list_lock[cur_task->cpu_id]);
    schedule();
    intr_set_status(intr_status);
    return;
}