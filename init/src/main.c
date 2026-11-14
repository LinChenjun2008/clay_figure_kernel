// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <errno.h>
#include <user/lib.h>

const char *sig_str[] = { "",          "SIGHUP",  "SIGINT",    "SIGQUIT",
                          "SIGILL",    "SIGTRAP", "SIGABRT",   "SIGBUS",
                          "SIGFPE",    "SIGKILL", "SIGUSR1",   "SIGSEGV",
                          "SIGUSR2",   "SIGPIPE", "SIGALRM",   "SIGTERM",
                          "SIGSTKFLT", "SIGCHLD", "SIGCONT",   "SIGSTOP",
                          "SIGTSTP",   "SIGTTIN", "SIGTTOU",   "SIGURG",
                          "SIGXCPU",   "SIGXFSZ", "SIGVTALRM", "SIGPROF",
                          "SIGWINCH",  "SIGIO",   "SIGPWR" };

int main(void)
{
    while (1)
    {
        int   status = 0;
        pid_t pid    = wait(-1, &status);
        if (pid >= 0)
        {
            const char *s   = "Normal exit";
            int         err = status > 0 ? status : -status;
            if (err == 0)
            {
                ;
            }
            else if (err > 128)
            {
                s = sig_str[err - 128];
            }
            else
            {
                s = err_str(err);
            }
            dprintf("task %d exit: %d ( %s )\n", pid, status, s);
        }
    }
    return 0;
}