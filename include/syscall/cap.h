// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __SYSCALL_CAP_H__
#define __SYSCALL_CAP_H__

#include <syscall/cap/struct.h>

typedef uint32_t cap_handle_t;

// slot id( >= 1 )
#define CAP_HANDLE_SLOT_ID_SHIFT 16
#define CAP_HANDLE_SLOT_ID_MASK  0xffff
#define CAP_HANDLE_KEY_SHIFT     0
#define CAP_HANDLE_KEY_MASK      0xffff

#define CAP_HANDLE(SLOT, KEY) \
    (SET_FIELD(0, CAP_HANDLE_SLOT_ID, SLOT) + SET_FIELD(0, CAP_HANDLE_KEY, KEY))

#define CAP_HANDLE_GET_SLOT_ID(HANDLE) GET_FIELD(HANDLE, CAP_HANDLE_SLOT_ID)
#define CAP_HANDLE_GET_KEY(HANDLE)     GET_FIELD(HANDLE, CAP_HANDLE_KEY)

struct cap_node *create_root_cap_node(void);
cap_handle_t
cap_insert(struct cap_node *cnode, struct cap_head *head, uint32_t rights);
struct cap_slot_entry *
cap_lookup(struct cap_node *cnode, cap_handle_t handle, uint32_t rights);
int cap_revoke(struct cap_node *cnode, cap_handle_t handle);
int cap_delete(struct cap_node *cnode, cap_handle_t handle);

cap_handle_t cap_derive(
    struct cap_node *src_cnode,
    cap_handle_t     parent_handle,
    struct cap_node *dst_cnode,
    uint32_t         rights
);
cap_handle_t cap_copy(
    struct cap_node *src_cnode,
    cap_handle_t     src_handle,
    struct cap_node *dst_cnode,
    uint32_t         rights
);
cap_handle_t cap_move(
    struct cap_node *src_cnode,
    cap_handle_t     src_handle,
    struct cap_node *dst_cnode
);

#endif /* __SYSCALL_CAP_H__ */
