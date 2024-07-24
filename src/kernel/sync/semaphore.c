#include <kernel/global.h>
#include <task/task.h>
#include <sync/semaphore.h>
#include <std/string.h>
#include <intr.h>

PUBLIC void init_semaphore(semaphore_t *sema,uint32_t value)
{
    memset(sema,0,sizeof(*sema));
    sema->value.value      = value;
    sema->holder           = NULL;
    sema->holder_repeat_nr = 0;
    list_init(&sema->waiters);
    return;
}

PUBLIC status_t sema_free(semaphore_t *sema)
{
    return sema->value.value != 0 ? K_SUCCESS : K_ERROR;
}

PUBLIC status_t sema_down(semaphore_t *sema)
{
    intr_status_t intr_status = intr_disable();
    while(sema->value.value == 0)
    {
        if (list_find(&sema->waiters,&running_task()->general_tag))
        {
            return K_ERROR;
        }
        list_append(&sema->waiters,&running_task()->general_tag);
        task_block(TASK_BLOCKED);
    }
    atomic_dec(&sema->value);
    intr_set_status(intr_status);
    return K_SUCCESS;
}

PUBLIC status_t sema_up(semaphore_t *sema)
{
    intr_status_t intr_status = intr_disable();
    if (!list_empty(&sema->waiters))
    {
        task_struct_t *blocked_task = CONTAINER_OF(task_struct_t,general_tag,list_pop(&sema->waiters));
        task_unblock(blocked_task->pid);
    }
    atomic_inc(&sema->value);
    intr_set_status(intr_status);
    return K_SUCCESS;
}