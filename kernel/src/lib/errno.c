// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <errno.h>

static const char *const err_tab[][2] = {
    { "OK", "Success" },                              // 0
    { "EPERM", "Operation not permitted" },           // 1
    { "ENOENT", "No such file or directory" },        // 2
    { "ESRCH", "No such process" },                   // 3
    { "EINTR", "Interrupted system call" },           // 4
    { "EIO", "Input/output error" },                  // 5
    { "ENXIO", "No such device or address" },         // 6
    { "E2BIG", "Argument list too long" },            // 7
    { "ENOEXEC", "Exec format error" },               // 8
    { "EBADF", "Bad file descriptor" },               // 9
    { "ECHILD", "No child processes" },               // 10
    { "EAGAIN", "Resource temporarily unavailable" }, // 11
    { "ENOMEM", "Cannot allocate memory" },           // 12
    { "EACCES", "Permission denied" },                // 13
    { "EFAULT", "Bad address" },                      // 14
    { "ENOTBLK", "Block device required" },           // 15
    { "EBUSY", "Device or resource busy" },           // 16
    { "EEXIST", "File exists" },                      // 17
    { "EXDEV", "Invalid cross-device link" },         // 18
    { "ENODEV", "No such device" },                   // 19
    { "ENOTDIR", "Not a directory" },                 // 20
    { "EISDIR", "Is a directory" },                   // 21
    { "EINVAL", "Invalid argument" },                 // 22
    { "ENFILE", "Too many open files in system" },    // 23
    { "EMFILE", "Too many open files" },              // 24
    { "ENOTTY", "Inappropriate ioctl for device" },   // 25
    { "ETXTBSY", "Text file busy" },                  // 26
    { "EFBIG", "File too large" },                    // 27
    { "ENOSPC", "No space left on device" },          // 28
    { "ESPIPE", "Illegal seek" },                     // 29
    { "EROFS", "Read-only file system" },             // 30
    { "EMLINK", "Too many links" },                   // 31
    { "EPIPE", "Broken pipe" },                       // 32
    { "EDOM", "Numerical argument out of domain" },   // 33
    { "ERANGE", "Numerical result out of range" },    // 34
};

static const char *err_lookup(int err, int field)
{
    int e = (err < 0) ? -err : err;

    if (e < 0 || e > ERRNO_MAX)
    {
        return (field == 0) ? "UNKNOWN" : "Unknown error";
    }
    return err_tab[e][field];
}

const char *err_str(int err)
{
    return err_lookup(err, 0);
}

const char *strerror(int errnum)
{
    return err_lookup(errnum, 1);
}
