#ifndef __TASK_H__
#define __TASK_H__

#include <lib/list.h>
#include <lib/alloc_table.h>
#include <device/spinlock.h>

typedef enum
{
    TASK_NO_TASK = 0,
    TASK_USING,
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_SENDING,
    TASK_RECEIVING,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
} task_status_t;

typedef struct
{
    wordsize_t r15;
    wordsize_t r14;
    wordsize_t r13;
    wordsize_t r12;

    wordsize_t rbp;
    wordsize_t rbx;
    wordsize_t rdi;
    wordsize_t rsi;
} task_context_t;

typedef struct
{
    task_context_t        *context;
    addr_t                 kstack_base;
    size_t                 kstack_size;

    addr_t                 ustack_base;
    size_t                 ustack_size;

    pid_t                  pid;

    char                   name[32];
    volatile task_status_t status;
    uint64_t               spinlock_count;
    uint64_t               priority;
    uint64_t               ticks;
    uint64_t               elapsed_ticks;

    list_node_t            general_tag;

    uint64_t               cpu_id;
    uint64_t              *page_dir;
    allocate_table_t       vaddr_table; // available when page_dir == NULL;

    message_t              msg;
    pid_t                  send_to;
    pid_t                  recv_from;
    uint8_t                has_intr_msg;
    spinlock_t             send_lock;
    list_t                 sender_list;
} task_struct_t;

typedef struct
{
    task_struct_t task_table[MAX_TASK];
    list_t        task_list[NR_CPUS];
    spinlock_t    task_lock;
} taskmgr_t;

PUBLIC task_struct_t* pid2task(pid_t pid);
PUBLIC bool task_exist(pid_t pid);
PUBLIC task_struct_t* running_task();
PUBLIC task_struct_t* running_prog();
PUBLIC addr_t get_running_prog_kstack();
PUBLIC status_t task_alloc(OUT(pid_t *pid));
PUBLIC void task_free(pid_t pid);
PUBLIC task_struct_t* init_task_struct
(
    task_struct_t* task,
    char* name,
    uint64_t priority,
    addr_t kstack_base,
    size_t kstack_size
);
PUBLIC void create_task_struct(task_struct_t *task,void *func,uint64_t arg);
PUBLIC task_struct_t* task_start
(
    char* name,
    uint64_t priority,
    size_t kstack_size,
    void* func,
    uint64_t arg
);
PUBLIC void task_init();

/// schedule.c
PUBLIC void schedule();
PUBLIC void task_block(task_status_t status);
PUBLIC void task_unblock(pid_t pid);

/// tss.c
PUBLIC void init_tss(uint8_t nr_cpu);
PUBLIC void update_tss_rsp0(task_struct_t *task);

/// prog.c
PUBLIC void prog_activate(task_struct_t *task);
PUBLIC task_struct_t *prog_execute
(
    char *name,
    uint64_t priority,
    size_t kstack_size,
    void *prog
);

#endif