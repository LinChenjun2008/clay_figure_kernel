// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __TASK_PROCESS_H__
#define __TASK_PROCESS_H__

struct task *process_execute(
    const char *file,
    uint64_t    prio,
    size_t      kstack_pages,
    size_t      ustack_pages,
    void       *arg
);
void process_exit(int status);

// exec.h
void *load_segment(void *file);

#endif /* __TASK_PROCESS_H__ */