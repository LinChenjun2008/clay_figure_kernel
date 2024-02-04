#include <kernel/global.h>
#include <task/task.h>
#include <mem/mem.h>
#include <intr.h>
#include <std/string.h>

extern list_t task_list;

PRIVATE void start_process(void *process)
{
    void *func = process;
    task_struct_t *cur = running_task();
    uintptr_t kstack = (uintptr_t)cur->context;
    kstack += sizeof(task_context_t);
    intr_stack_t *proc_stack = (intr_stack_t*)kstack;

    proc_stack->r15 = 0;
    proc_stack->r14 = 0;
    proc_stack->r13 = 0;
    proc_stack->r12 = 0;
    proc_stack->r11 = 0;
    proc_stack->r10 = 0;
    proc_stack->r9 = 0;
    proc_stack->r8 = 0;

    proc_stack->rdi = 0;
    proc_stack->rsi = 0;
    proc_stack->rbp = 0;
    proc_stack->rdx = 0;
    proc_stack->rcx = 0;
    proc_stack->rbx = 0;
    proc_stack->rax = 0;

    proc_stack->gs = SELECTOR_DATA64_U;
    proc_stack->fs = SELECTOR_DATA64_U;
    proc_stack->es = SELECTOR_DATA64_U;
    proc_stack->ds = SELECTOR_DATA64_U;
    proc_stack->rip = func;
    proc_stack->cs = SELECTOR_CODE64_U;
    proc_stack->rflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);

    cur->context = (task_context_t*)kstack;
    // 分配用户态下的栈
    void *ustack     = alloc_physical_page(1);
    cur->ustack_base = (uintptr_t)ustack;
    cur->ustack_size = PG_SIZE;
    page_map(cur->page_dir,ustack,(void*)USER_STACK_VADDR_BASE);
    proc_stack->rsp = USER_STACK_VADDR_BASE + PG_SIZE;
    proc_stack->ss = SELECTOR_DATA64_U;
    __asm__ __volatile__
    (
        "movq %0, %%rsp\n\t"
        "jmp intr_exit"
        :
        :"g"(proc_stack)
        :"memory"
    );
}

PRIVATE void page_dir_activate(task_struct_t *task)
{
    void *page_dir_table_pos = (void*)KERNEL_PAGE_DIR_TABLE_POS;
    if (task->page_dir != NULL)
    {
        page_dir_table_pos = task->page_dir;
    }
    __asm__ __volatile__("movq %0,%%cr3"::"r"(page_dir_table_pos):"memory");
    return;
}

PUBLIC void prog_activate(task_struct_t *task)
{
    page_dir_activate(task);
    if (task->page_dir != NULL)
    {
        update_tss_rsp0(task);
    }
    return;
}

PRIVATE uint64_t *create_page_dir(void)
{
    uint64_t *pgdir_v = KADDR_P2V(pmalloc(PT_SIZE));
    if (pgdir_v == NULL)
    {
        return NULL;
    }
    memset(pgdir_v,0,PT_SIZE);
    memcpy(pgdir_v + 0x100,(uint64_t*)KADDR_P2V(KERNEL_PAGE_DIR_TABLE_POS)
                            + 0x100,2048);
    return (uint64_t*)KADDR_V2P(pgdir_v);
}

PRIVATE void user_vaddr_table_init(task_struct_t *task)
{
    size_t entry_size          = sizeof(*task->vaddr_table.entries);
    uint64_t number_of_entries = 1024;
    void *p;
    p = KADDR_P2V(pmalloc(entry_size * number_of_entries));
    allocate_table_init(&task->vaddr_table,p,number_of_entries);

    uint64_t index = USER_VADDR_START / PG_SIZE;
    uint64_t cnt   = (USER_STACK_VADDR_BASE - USER_VADDR_START) / PG_SIZE;
    free_units(&task->vaddr_table,index,cnt);
    return;
}

PUBLIC task_struct_t *prog_execute(void *prog,char *name,size_t kstack_size)
{
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
    init_task_struct(task,name,3,(uintptr_t)KADDR_P2V(kstack_base),kstack_size);
    create_task_struct(task,start_process,(uint64_t)prog);
    task->page_dir = create_page_dir();
    user_vaddr_table_init(task);
    list_append(&task_list,&(task->general_tag));
    return task;
}