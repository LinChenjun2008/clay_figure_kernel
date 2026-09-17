// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __TYPES_MESSAGE_H__
#define __TYPES_MESSAGE_H__

enum msg_type
{
    MSG_NORMAL,      // 正常消息传递
    MSG_CAP_DELIVER, // 传递capability
};

struct msg_head
{
    pid_t         source;
    enum msg_type type;
    size_t        header_legnth; // sizeof(struct msg_head)
    size_t        legnth;        // head + body
};

struct msg_cap_deliver
{
    struct msg_head head;
    cap_handle_t    handle;
    uint32_t        rights;
};

#endif /* __TYPES_MESSAGE_H__ */