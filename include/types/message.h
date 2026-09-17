// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __TYPES_MESSAGE_H__
#define __TYPES_MESSAGE_H__

enum message_type
{
    MSG_NORMAL,      // 正常消息传递
    MSG_CAP_DELIVER, // 传递capability
};

struct message_head
{
    pid_t             source;
    enum message_type type;
    size_t            header_legnth; // sizeof(struct message_head)
    size_t            legnth;        // head + body
};

#endif /* __TYPES_MESSAGE_H__ */