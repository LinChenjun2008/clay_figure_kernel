// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __PATH_H__
#define __PATH_H__

void        path_normalize(char *path);
const char *path_next(const char *path, char *buf);

#endif /* __PATH_H__ */