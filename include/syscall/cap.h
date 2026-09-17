// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __SYSCALL_CAP_H__
#define __SYSCALL_CAP_H__

// slot id( >= 1 )
#define CAP_HANDLE_SLOT_ID_SHIFT 16
#define CAP_HANDLE_SLOT_ID_MASK  0xffff
#define CAP_HANDLE_KEY_SHIFT     0
#define CAP_HANDLE_KEY_MASK      0xffff

#define CAP_HANDLE(SLOT, KEY) \
    (SET_FIELD(0, CAP_HANDLE_SLOT_ID, SLOT) + SET_FIELD(0, CAP_HANDLE_KEY, KEY))

#define CAP_HANDLE_GET_SLOT_ID(HANDLE) GET_FIELD(HANDLE, CAP_HANDLE_SLOT_ID)
#define CAP_HANDLE_GET_KEY(HANDLE)     GET_FIELD(HANDLE, CAP_HANDLE_KEY)

struct cap_head;
struct cap_node;
struct cap_opt;

enum cap_type
{
    CAP_NONE = 0,
    CAP_NODE,
    CAP_POINT,
    CAP_IPC,
    CAP_MEM,
    CAP_TASK,
    CAP_NR // 有效对象类型数量
};

typedef uint32_t cap_handle_t;

void init_cap_head(
    struct cap_head      *head,
    enum cap_type         type,
    const struct cap_opt *opt
);

int32_t cap_reference(struct cap_head *head);
int32_t cap_release(struct cap_head *head);

struct cap_head *create_root_cap_node(void);
void             destory_cap(struct cap_head *head);
cap_handle_t
cap_insert(struct cap_head *node_head, struct cap_head *head, uint32_t rights);
void cap_delete(struct cap_head *node_head, cap_handle_t handle);
struct cap_head *
cap_lookup(struct cap_head *node_head, cap_handle_t handle, uint32_t rights);
int copy_cnode(struct cap_head *dst, struct cap_head *src);

#endif /* __SYSCALL_CAP_H__ */
