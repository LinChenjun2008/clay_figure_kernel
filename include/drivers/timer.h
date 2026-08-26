// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __DRIVERS_TIMER_H__
#define __DRIVERS_TIMER_H__

int run_timeout(int (*func)(void *), void *arg, uint64_t timeout);

#endif /* __DRIVERS_TIMER_H__ */