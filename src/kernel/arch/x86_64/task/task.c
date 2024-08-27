#include <kernel/global.h>
#include <task/task.h>      // include sse,spinlock
#include <mem/mem.h>        // pmalloc,to_physical_address,init_alloc_physical_page
#include <intr.h>           // intr functions
#include <device/cpu.h>     // apic_id
#include <std/string.h>     // memset,strlen,strcpy
#include <service.h>        // MM_EXIT
#include <kernel/syscall.h> // sys_send_recv

#include <log.h>

PUBLIC taskmgr_t *tm;

PRIVATE void kernel_task(void)
{
    addr_t  func;
    wordsize_t arg;
    __asm__ __volatile__ (
        "movq %%rsi,%[func] \n\t"
        "movq %%rdi,%[arg] \n\t"
        "movq $0,%%rsi \n\t"
        "movq $0,%%rdi \n\t"
        :[func]"=g"(func),[arg]"=g"(arg)
        :
        :"rsi","rdi");
    intr_enable();
    ((void (*)(void*))func)((void*)arg);
    message_t msg;
    msg.type = MM_EXIT;
    sys_send_recv(NR_BOTH,MM,&msg);
    while (1) continue;
    return;
}

PUBLIC task_struct_t* pid2task(pid_t pid)
{
    if (pid <= MAX_TASK)
    {
        return &tm->task_table[pid];
    }
    return NULL;
}

PUBLIC bool task_exist(pid_t pid)
{
    return pid <= MAX_TASK ? tm->task_table[pid].status != TASK_NO_TASK : 0;
}

// running_task() can only be called in ring 0
PUBLIC task_struct_t* running_task()
{
    wordsize_t rsp;
    __asm__ __volatile__ ("movq %%rsp,%0":"=g"(rsp)::);
    intr_status_t intr_status = intr_disable();
    task_struct_t *res = NULL;
    int i;
    for (i = 0;i < MAX_TASK;i++)
    {
        if (tm->task_table[i].status == TASK_NO_TASK)
        {
            continue;
        }
        if (rsp >= tm->task_table[i].kstack_base
            &&   rsp <= tm->task_table[i].kstack_base
               + tm->task_table[i].kstack_size)
        {
            res = &tm->task_table[i];
            break;
        }
    }
    intr_set_status(intr_status);
    return res;
}

// running_prog() can only be called in syscall
PUBLIC task_struct_t* running_prog()
{
    wordsize_t rsp;
    wordsize_t pml4t;
    __asm__ __volatile__ ("movq %%rsp,%0 \n\t"
                          "movq %%cr3,%1":"=g"(rsp),"=g"(pml4t)::);
    rsp = (wordsize_t)to_physical_address((void*)pml4t,(void*)rsp);
    intr_status_t intr_status = intr_disable();
    task_struct_t *res = NULL;
    int i;
    for (i = 0;i < MAX_TASK;i++)
    {
        if (tm->task_table[i].status == TASK_NO_TASK)
        {
            continue;
        }
        if (rsp >= tm->task_table[i].ustack_base
            &&   rsp <= tm->task_table[i].ustack_base
               + tm->task_table[i].ustack_size)
        {
            res = &tm->task_table[i];
            break;
        }
    }
    if (i == MAX_TASK)
    {
        PANIC("running program not found.\n");
    }
    intr_set_status(intr_status);
    return res;
}

PUBLIC addr_t get_running_prog_kstack()
{
    task_struct_t *cur = running_prog();
    return cur->kstack_base + cur->kstack_size;
}

PUBLIC status_t task_alloc(pid_t *pid)
{
    status_t res = K_ERROR;
    spinlock_lock(&tm->task_table_lock);
    pid_t i;
    for (i = 0;i < MAX_TASK;i++)
    {
        if (tm->task_table[i].status == TASK_NO_TASK)
        {
            memset(&tm->task_table[i],0,sizeof(tm->task_table[i]));
            tm->task_table[i].status = TASK_USING;
            *pid = i;
            res = K_SUCCESS;
            break;
        }
    }
    spinlock_unlock(&tm->task_table_lock);
    return res;
}

PUBLIC void task_list_insert(list_t *list,task_struct_t *task)
{
    list_node_t *node = list->head.next;
    task_struct_t *tmp;
    while (node != &list->tail)
    {
        tmp = CONTAINER_OF(task_struct_t,general_tag,node);
        if ((int64_t)(task->vrun_time - tmp->vrun_time) < 0)
        {
            break;
        }
        node = list_next(node);
    }
    list_in(&task->general_tag,node);
    return;
}

PUBLIC void task_free(pid_t pid)
{
    if (pid < MAX_TASK)
    {
        spinlock_lock(&tm->task_table_lock);
        tm->task_table[pid].status = TASK_NO_TASK;
        spinlock_unlock(&tm->task_table_lock);
    }
    return;
}

PUBLIC status_t init_task_struct(
    task_struct_t* task,
    char* name,
    uint64_t priority,
    addr_t kstack_base,
    size_t kstack_size)
{
    memset(task,0,sizeof(*task));
    addr_t kstack      = kstack_base + kstack_size;
    task->context     = (task_context_t*)kstack;
    task->kstack_base = kstack_base;
    task->kstack_size = kstack_size;

    task->ustack_base = 0;
    task->ustack_size = 0;

    task->pid         = ((addr_t)task - (addr_t)tm->task_table) / sizeof(*task);
    task->ppid        = running_task()->pid;

    if (strlen(name) > 31)
    {
        name[31] = 0;
    }
    strcpy(task->name,name);
    task->status         = TASK_READY;
    task->spinlock_count = 0;
    task->priority       = priority;
    task->jiffies        = 0;
    task->vrun_time      = running_task()->vrun_time;

    task->cpu_id         = apic_id();
    task->page_dir       = NULL;

    task->send_to        = MAX_TASK;
    task->recv_from      = MAX_TASK;
    task->has_intr_msg   = 0;
    init_spinlock(&task->send_lock);
    list_init(&task->sender_list);
    addr_t fxsave_region;
    status_t status = pmalloc(sizeof(*task->fxsave_region),&fxsave_region);
    if (ERROR(status))
    {
        return status;
    }
    task->fxsave_region = KADDR_P2V(fxsave_region);
    return K_SUCCESS;
}

PUBLIC void create_task_struct(task_struct_t *task,void *func,uint64_t arg)
{
    addr_t kstack = (addr_t)task->context;
    kstack -= sizeof(intr_stack_t);
    kstack -= sizeof(addr_t);
    *(addr_t*)kstack = (addr_t)kernel_task;
    kstack -= sizeof(task_context_t);
    task->context = (task_context_t*)kstack;
    task_context_t *context =task->context;
    context->rsi = (wordsize_t)func;
    context->rdi = (wordsize_t)arg;
}

PUBLIC task_struct_t* task_start(
    char* name,
    uint64_t priority,
    size_t kstack_size,
    void* func,
    uint64_t arg)
{
    // kstack_size == 2 ** n
    if (kstack_size & (kstack_size - 1))
    {
        return NULL;
    }
    status_t status;
    pid_t pid;
    status = task_alloc(&pid);
    if (ERROR(status))
    {
        return NULL;
    }
    void *kstack_base;
    status = pmalloc(kstack_size,&kstack_base);
    if (ERROR(status))
    {
        task_free(pid);
        return NULL;
    }
    task_struct_t *task = pid2task(pid);
    init_task_struct(task,
                     name,
                     priority,
                     (addr_t)KADDR_P2V(kstack_base),
                     kstack_size);
    create_task_struct(task,func,arg);

    spinlock_lock(&tm->task_list_lock[apic_id()]);
    task_list_insert(&tm->task_list[apic_id()],task);
    spinlock_unlock(&tm->task_list_lock[apic_id()]);
    return task;
}

PRIVATE void make_main_task(void)
{
    tm->task_table[0].status  = TASK_USING;
    task_struct_t *main_task = pid2task(0);
    init_task_struct(main_task,
                     "Main task",
                     DEFAULT_PRIORITY,
                     (addr_t)KADDR_P2V(KERNEL_STACK_BASE),
                     KERNEL_STACK_SIZE);
    tm->idle_task[apic_id()] = main_task->pid;
    spinlock_lock(&tm->task_list_lock[apic_id()]);
    task_list_insert(&tm->task_list[apic_id()],main_task);
    spinlock_unlock(&tm->task_list_lock[apic_id()]);

    return;
}

PUBLIC void task_init()
{
    addr_t addr;
    status_t status;
    status = init_alloc_physical_page(sizeof(*tm) / PG_SIZE + 1,&addr);
    if (ERROR(status))
    {
        PANIC("Can not allocate memory for task manager.");
    }
    tm = KADDR_P2V(addr);
    memset(tm,0,sizeof(*tm));
    pid_t i;
    for (i = 0;i < MAX_TASK;i++)
    {
        tm->task_table[i].status = TASK_NO_TASK;
    }
    for (i = 0;i < NR_CPUS;i++)
    {
        list_init(&tm->task_list[i]);
        init_spinlock(&tm->task_list_lock[i]);
    }
    init_spinlock(&tm->task_table_lock);

    make_main_task();
    return;
}