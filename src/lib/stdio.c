// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>

#include <std/stdarg.h>
#include <std/stdio.h>
#include <std/string.h>


#define FORMAT_LEFT    (1 << 0)
#define FORMAT_SPACE   (1 << 1)
#define FORMAT_ZERO    (1 << 2)
#define FORMAT_SIGN    (1 << 3)
#define FORMAT_PLUS    (1 << 4)
#define FORMAT_SPECIAL (1 << 5)
#define FORMAT_SMALL   (1 << 6)

#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')

PRIVATE int skip_atoi(const char **s)
{
    int i = 0;
    while (IS_DIGIT(**s))
    {
        i = i * 10 + *((*s)++) - '0';
    }
    return i;
}

PRIVATE char *number_to_string(
    char    *str,
    uint64_t num,
    int      base,
    int      width,
    int      precision,
    int      flag
)
{
    char        pad, sign, tmp[50];
    const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if (flag & FORMAT_SMALL)
    {
        digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    }
    if (base < 2 || base > 36) return 0;
    pad  = (flag & FORMAT_ZERO) ? '0' : ' ';
    sign = 0;
    if (flag & FORMAT_SIGN && (int64_t)num < 0)
    {
        sign = '-';
        num  = -num;
    }
    else
    {
        sign = (flag & FORMAT_PLUS) ? '+' : ((flag & FORMAT_SPACE) ? ' ' : 0);
    }
    if (sign) width--;
    if (flag & FORMAT_SPECIAL)
    {
        if (base == 16) width -= 2;
        if (base == 8) width -= 1;
    }
    int i = 0;
    if (num == 0)
    {
        tmp[i++] = '0';
    }
    else
    {
        while (num != 0)
        {
            tmp[i++] = digits[num % base];
            num /= base;
        }
    }
    if (i > precision)
    {
        precision = i;
    }
    width -= precision;
    if (!(flag & (FORMAT_ZERO | FORMAT_LEFT)))
    {
        while (width-- > 0) *str++ = ' ';
    }
    if (sign)
    {
        *str++ = sign;
    }
    if (flag & FORMAT_SPECIAL)
    {
        if (base == 8)
        {
            *str++ = '0';
        }
        if (base == 16)
        {
            *str++ = '0';
            *str++ = digits[33];
        }
    }
    if (!(flag & FORMAT_LEFT))
    {
        while (width-- > 0) *str++ = pad;
    }

    while (i < precision--) *str++ = '0';
    while (i-- > 0) *str++ = tmp[i];
    while (width-- > 0) *str++ = ' ';
    return str;
}

PRIVATE char *float_to_string(
    char  *str,
    double num,
    int    base,
    int    width,
    int    precision,
    int    flag
)
{
    char        pad, integer_part[50], fract_part[50];
    const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int         i, integer_width, fract_width;
    int         integer  = (int)num;
    double      mantissa = (num - integer);
    pad                  = (flag & FORMAT_ZERO) ? '0' : ' ';

    integer_width = 0;
    do
    {
        integer_part[integer_width++] = digits[integer % base];
        integer /= base;
    } while (integer != 0);
    width -= integer_width;

    fract_width = 0;
    if (precision == -1)
    {
        precision = 6;
    }
    if (precision > 16)
    {
        precision = 16;
    }
    if (precision != 0)
    {
        fract_part[fract_width++] = '.';
    }
    for (i = 0; i < precision; i++)
    {
        mantissa *= base;
        integer                   = (int)mantissa;
        fract_part[fract_width++] = digits[integer];
        mantissa -= integer;
    }
    width -= fract_width;

    if (!(flag & FORMAT_LEFT))
    {
        while (width-- > 0) *str++ = pad;
    }

    while (integer_width-- > 0) *str++ = integer_part[integer_width];
    while (fract_width-- > 0) *str++ = fract_part[i - fract_width];
    while (width-- > 0) *str++ = ' ';
    return str;
}

PRIVATE int get_flag(const char **fmt)
{
    int flag = 0;
repeat:
    (*fmt)++;
    /* %后可以加'-',' ','0','#' */
    switch (**fmt)
    {
        case '-':
            flag |= FORMAT_LEFT;
            goto repeat;
            break;
        case '0':
            flag |= FORMAT_ZERO;
            goto repeat;
        case ' ':
            flag |= FORMAT_SPACE;
            goto repeat;
            break;
        case '#':
            flag |= FORMAT_SPECIAL;
            goto repeat;
            break;
        default:
            break;
    }
    if (flag & FORMAT_LEFT)
    {
        flag &= ~FORMAT_ZERO;
    }
    return flag;
}

PUBLIC int vsprintf(char *buf, const char *fmt, va_list ap)
{
    char *str, *s, qualifier;
    int   flag;
    int   width;
    for (str = buf; *fmt != '\0'; fmt++)
    {
        if (*fmt != '%')
        {
            *str++ = *fmt;
            continue;
        }
        flag = get_flag(&fmt);

        width = -1;
        if (IS_DIGIT(*fmt))
        {
            width = skip_atoi(&fmt);
        }
        else if (*fmt == '*')
        {
            fmt++;
            width = va_arg(ap, int);
            if (width < 0)
            {
                width = -width;
                flag |= FORMAT_LEFT;
            }
        }

        int precision = -1;
        if (*fmt == '.')
        {
            fmt++;
            if (IS_DIGIT(*fmt))
            {
                precision = skip_atoi(&fmt);
            }
            else if (*fmt == '*')
            {
                fmt++;
                precision = va_arg(ap, int);
            }
            if (precision < 0)
            {
                precision = 0;
            }
        }

        qualifier = 0;
        while (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' || *fmt == 'Z')
        {
            qualifier = *fmt;
            fmt++;
        }
        switch (*fmt)
        {
            case '%': /* %% */
                *str = '%';
                str++;
                break;

            case 'c': /* %c */
                if (!(flag & FORMAT_LEFT))
                {
                    while (--width > 0)
                    {
                        *str++ = ' ';
                    }
                };
                *str++ = va_arg(ap, uint64_t);
                while (--width > 0)
                {
                    *str++ = ' ';
                }
                break;

            case 'd': /* %d */
            case 'i':
                flag |= FORMAT_SIGN;
            case 'u':
                if (qualifier == 'l')
                {
                    str = number_to_string(
                        str,
                        va_arg(ap, long long int),
                        10,
                        width,
                        precision,
                        flag
                    );
                }
                else
                {
                    str = number_to_string(
                        str, va_arg(ap, int), 10, width, precision, flag
                    );
                }
                break;

            case 'o': /* %o */
                if (qualifier == 'l')
                {
                    str = number_to_string(
                        str,
                        va_arg(ap, long long int),
                        8,
                        width,
                        precision,
                        flag
                    );
                }
                else
                {
                    str = number_to_string(
                        str, va_arg(ap, int), 8, width, precision, flag
                    );
                }
                break;

            case 'p':
                if (width == -1)
                {
                    width = 2 * sizeof(void *);
                    flag |= FORMAT_ZERO;
                }
                str = number_to_string(
                    str, va_arg(ap, addr_t), 16, width, precision, flag
                );
                break;

            case 's': /* %s */
                s = va_arg(ap, char *);
                strcpy(str, s);
                str += strlen(s);
                break;

            case 'x': /* %x */
                flag |= FORMAT_SMALL;
            case 'X':
                if (qualifier == 'l')
                {
                    str = number_to_string(
                        str,
                        va_arg(ap, unsigned long long int),
                        16,
                        width,
                        precision,
                        flag
                    );
                }
                else
                {
                    str = number_to_string(
                        str,
                        va_arg(ap, unsigned int),
                        16,
                        width,
                        precision,
                        flag
                    );
                }
                break;
            case 'f':
                str = float_to_string(
                    str, va_arg(ap, double), 10, width, precision, flag
                );
                break;

            default:
                *str++ = '%';
                if (*fmt)
                {
                    *str++ = *fmt;
                }
                else
                {
                    fmt--;
                }
                break;
        }
    }
    *str = '\0';
    return strlen(buf);
}

PUBLIC int sprintf(char *buf, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int len;
    len = vsprintf(buf, fmt, ap);
    va_end(ap);
    return len;
}