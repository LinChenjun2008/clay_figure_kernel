// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/init.h>

// TEST
#include <print.h>
#include <syscall.h>
#include <task.h>

static int proc()
{
    return 0;
}

int main(struct boot_info *boot_info)
{
    init_all(boot_info);
    process_execute("process", DEFAULT_PRIO, 1, 1, proc);

    int status = 0;
    while (1)
    {
        pid_t exited = task_waitpid(-1, &status, WNOHANG);
        if (exited > 0)
        {
            printk("task %d exit with status: %d.\n", exited, status);
        }
    }
    return 0;
}

int ap_main(uintptr_t stack)
{
    ap_init_all(stack);
    process_execute("process", DEFAULT_PRIO, 1, 1, proc);

    int status = 0;
    while (1)
    {
        pid_t exited = task_waitpid(-1, &status, WNOHANG);
        if (exited > 0)
        {
            printk("task %d exit with status: %d.\n", exited, status);
        }
    }
    return 0;
}