// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __PRINT_FORMAT_H__
#define __PRINT_FORMAT_H__

#define MSG_INFO "\033[102;30m INFO  \033[0m "
#define MSG_WARN "\033[103;30m WARN  \033[0m "
#define MSG_DBG  "\033[106;30m DEBUG \033[0m "
#define MSG_ERR  "\033[101;30m ERROR \033[0m "

#define MSG_HIGHLIGHT(MSG) "\033[97m" MSG "\033[0m"

#endif /* __PRINT_FORMAT_H__ */
