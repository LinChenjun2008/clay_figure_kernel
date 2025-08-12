// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 LinChenjun
 */

.global send_recv
.type send_recv,@function
send_recv:
    xchgq %rcx, %r10 // save rcx in r10
    syscall
    retq
