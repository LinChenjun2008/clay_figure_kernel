#include <kernel/global.h>
#include <task/task.h>
#include <mem/mem.h>
#include <intr.h>
#include <std/string.h>

PRIVATE task_struct_t task_table[MAX_TASK];
PUBLIC  list_t        task_list;

PRIVATE void kernel_thread(void)
{
    uintptr_t  func;
    wordsize_t arg;
    __asm__ __volatile__
    (
        "movq %%rsi,%[func] \n\t"
        "movq %%rdi,%[arg] \n\t"
        "movq $0,%%rsi \n\t"
        "movq $0,%%rdi \n\t"
        :[func]"=g"(func),[arg]"=g"(arg)
        :
        :"rsi","rdi"
    );
    intr_enable();
    ((void (*)(void*))func)((void*)arg);
    return;
}

PUBLIC task_struct_t* pid2task(pid_t pid)
{
    if (pid >= MAX_TASK)
    {
        return NULL;
    }
    return &task_table[pid];
}


// running_task() can only be called in ring 0
PUBLIC task_struct_t* running_task()
{
    wordsize_t rsp;
    __asm__ __volatile__ ("movq %%rsp,%0":"=g"(rsp)::);
    int i;
    for (i = 0;i < MAX_TASK;i++)
    {
        if (task_table[i].status == TASK_NO_TASK)
        {
            continue;
        }
        if (rsp >= task_table[i].kstack_base
            && rsp <= task_table[i].kstack_base + task_table[i].kstack_size)
        {
            return &task_table[i];
        }
    }
    return NULL;
}

// running_prog() can only be called in syscall
PUBLIC task_struct_t* running_prog()
{
    wordsize_t rsp;
    wordsize_t pml4t;
    __asm__ __volatile__ ("movq %%rsp,%0 \n\t""movq %%cr3,%1":"=g"(rsp),"=g"(pml4t)::);

    rsp = (wordsize_t)to_physical_address((void*)pml4t,(void*)rsp);
    int i;
    for (i = 0;i < MAX_TASK;i++)
    {
        if (task_table[i].status == TASK_NO_TASK)
        {
            continue;
        }
        if (rsp >= task_table[i].ustack_base
            && rsp <= task_table[i].ustack_base + task_table[i].ustack_size)
        {
            return &task_table[i];
        }
    }
    return NULL;
}

PUBLIC uintptr_t get_running_prog_kstack()
{
    task_struct_t *cur = running_prog();
    return cur->kstack_base + cur->kstack_size;
}

PUBLIC pid_t task_alloc()
{
    pid_t i;
    for (i = 0;i < MAX_TASK;i++)
    {
        if (task_table[i].status == TASK_NO_TASK)
        {
            task_table[i].status = TASK_USING;
            return i;
        }
    }
    return MAX_TASK;
}

PUBLIC void task_free(pid_t pid)
{
    if (pid < MAX_TASK)
    {
        task_table[pid].status = TASK_NO_TASK;
    }
    return;
}

PUBLIC task_struct_t* init_task_struct
(
    task_struct_t* task,
    char* name,
    uint64_t priority,
    uintptr_t kstack_base,
    size_t kstack_size
)
{
    memset(task,0,sizeof(*task));
    uintptr_t kstack  = kstack_base + kstack_size;
    task->context     = (task_context_t*)kstack;
    task->kstack_base = kstack_base;
    task->kstack_size = kstack_size;

    task->ustack_base = 0;
    task->ustack_size = 0;

    task->pid         = ((uintptr_t)task - (uintptr_t)task_table) / sizeof(*task);

    if (strlen(name) > 31)
    {
        name[31] = 0;
    }
    strcpy(task->name,name);
    task->status        = TASK_READY;

    task->priority      = priority;
    task->ticks         = 0;
    task->elapsed_ticks = 0;

    task->page_dir      = NULL;

    task->send_to       = MAX_TASK;
    task->recv_from     = MAX_TASK;
    task->has_intr_msg  = 0;
    list_init(&task->sender_list);
    return task;
}

PUBLIC void create_task_struct(task_struct_t *task,void *func,uint64_t arg)
{
    uintptr_t kstack = (uintptr_t)task->context;
    kstack -= sizeof(intr_stack_t);
    kstack -= sizeof(uintptr_t);
    *(uintptr_t*)kstack = (uintptr_t)kernel_thread;
    kstack -= sizeof(task_context_t);
    task->context = (task_context_t*)kstack;
    task_context_t *context =task->context;
    context->rsi = (wordsize_t)func;
    context->rdi = (wordsize_t)arg;
}

PUBLIC task_struct_t* task_start
(
    char* name,
    uint64_t priority,
    size_t kstack_size,
    void* func,
    uint64_t arg
)
{
    // kstack_size == 2 ** n
    if (kstack_size & (kstack_size - 1))
    {
        return NULL;
    }
    pid_t pid = task_alloc();
    if (pid == MAX_TASK)
    {
        return NULL;
    }
    void *kstack_base = pmalloc(kstack_size);
    if (kstack_base == NULL)
    {
        task_free(pid);
        return NULL;
    }
    task_struct_t *task = pid2task(pid);
    init_task_struct(task,name,priority,(uintptr_t)KADDR_P2V(kstack_base),kstack_size);
    create_task_struct(task,func,arg);
    list_append(&task_list,&task->general_tag);
    return task;
}

PRIVATE void make_main_task(void)
{
    task_struct_t *main_task = pid2task(task_alloc());
    init_task_struct(main_task,
                    "Main task",
                    3,
                    (uintptr_t)KADDR_P2V(KERNEL_STACK_BASE),
                    KERNEL_STACK_SIZE);
    list_append(&task_list,&main_task->general_tag);
    return;
}

PUBLIC void task_init()
{
    memset(task_table,0,sizeof(task_table[0]) * MAX_TASK);
    pid_t i;
    for (i = 0;i < MAX_TASK;i++)
    {
        task_table[i].status = TASK_NO_TASK;
    }
    list_init(&task_list);
    make_main_task();
    return;
}