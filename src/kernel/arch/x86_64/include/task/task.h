#ifndef __TASK_H__
#define __TASK_H__

#include <lib/list.h>

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
    // stack in range [stack_base,stack_base + stack_size]
    task_context_t        *context;
    uintptr_t              kstack_base;
    size_t                 kstack_size;

    wordsize_t             ustack;
    size_t                 ustack_size;

    pid_t                  pid;

    char                   name[32];
    volatile task_status_t status;

    uint64_t               priority;
    uint64_t               ticks;
    uint64_t               elapsed_ticks;

    list_node_t            general_tag;

    uint64_t              *page_dir;

    void                  *message;
    pid_t                  send_to;
    pid_t                  recv_from;
    uint8_t                has_intr_msg;
    list_t                 sender_list;
} task_struct_t;

PUBLIC task_struct_t* pid2task(pid_t pid);
PUBLIC task_struct_t* running_task();
PUBLIC pid_t task_alloc();
PUBLIC void task_free(pid_t pid);
PUBLIC task_struct_t* init_task_struct
(
    task_struct_t* task,
    char* name,
    uint64_t priority,
    uintptr_t kstack_base,
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

/// tss.c
PUBLIC void init_tss();

#endif