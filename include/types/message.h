// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __TYPES_MESSAGE_H__
#define __TYPES_MESSAGE_H__

enum msg_type
{
    MSG_NORMAL, // 正常消息传递
    MSG_LOG,    // 日志消息,将data部分的数据作为字符串输出(SERV_TASK)
};

struct msg_head
{
    pid_t         source;
    int           key;
    enum msg_type type;

    int args[8];

    void  *data;
    size_t size;
};

#endif /* __TYPES_MESSAGE_H__ */