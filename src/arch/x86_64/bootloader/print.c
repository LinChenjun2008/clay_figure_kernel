// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#include <bootloader.h>

#define FORMAT_LEFT    (1 << 0)
#define FORMAT_SPACE   (1 << 1)
#define FORMAT_ZERO    (1 << 2)
#define FORMAT_SIGN    (1 << 3)
#define FORMAT_PLUS    (1 << 4)
#define FORMAT_SPECIAL (1 << 5)
#define FORMAT_SMALL   (1 << 6)

#define IS_DIGIT(c) ((c) >= L'0' && (c) <= L'9')

static CHAR16 *strcpy(CHAR16 *dst, const CHAR16 *src)
{
    CHAR16 *ret = dst;
    while ((*dst++ = *(CHAR16 *)src++) != L'\0');
    return ret;
}

static size_t strlen(const CHAR16 *str)
{
    const CHAR16 *end = str;
    while (*end++ != 0);
    return end - str - 1;
}

static int skip_atoi(const CHAR16 **s)
{
    int i = 0;
    while (IS_DIGIT(**s))
    {
        i = i * 10 + *((*s)++) - L'0';
    }
    return i;
}

static CHAR16 *number_to_string(
    CHAR16  *str,
    uint64_t num,
    int      base,
    int      width,
    int      precision,
    int      flag
)
{
    CHAR16        pad, sign, tmp[50];
    const CHAR16 *digits = L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if (flag & FORMAT_SMALL)
    {
        digits = L"0123456789abcdefghijklmnopqrstuvwxyz";
    }
    if (base < 2 || base > 36) return 0;
    pad  = (flag & FORMAT_ZERO) ? L'0' : L' ';
    sign = 0;
    if (flag & FORMAT_SIGN && (int64_t)num < 0)
    {
        sign = L'-';
        num  = -num;
    }
    else
    {
        sign = (flag & FORMAT_PLUS) ? L'+' : ((flag & FORMAT_SPACE) ? L' ' : 0);
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
        tmp[i++] = L'0';
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
        while (width-- > 0) *str++ = L' ';
    }
    if (sign)
    {
        *str++ = sign;
    }
    if (flag & FORMAT_SPECIAL)
    {
        if (base == 8)
        {
            *str++ = L'0';
        }
        if (base == 16)
        {
            *str++ = L'0';
            *str++ = digits[33];
        }
    }
    if (!(flag & FORMAT_LEFT))
    {
        while (width-- > 0) *str++ = pad;
    }

    while (i < precision--) *str++ = L'0';
    while (i-- > 0) *str++ = tmp[i];
    while (width-- > 0) *str++ = L' ';
    return str;
}

static CHAR16 *float_to_string(
    CHAR16 *str,
    double  num,
    int     base,
    int     width,
    int     precision,
    int     flag
)
{
    CHAR16        pad, integer_part[50], fract_part[50];
    const CHAR16 *digits = L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int           i, integer_width, fract_width;
    int           integer  = (int)num;
    double        mantissa = (num - integer);
    pad                    = (flag & FORMAT_ZERO) ? L'0' : L' ';

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
        fract_part[fract_width++] = L'.';
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
    while (width-- > 0) *str++ = L' ';
    return str;
}

static int get_flag(const CHAR16 **fmt)
{
    int flag = 0;
repeat:
    (*fmt)++;
    /* %后可以加'-',' ','0','#' */
    switch (**fmt)
    {
        case L'-':
            flag |= FORMAT_LEFT;
            goto repeat;
            break;
        case L'0':
            flag |= FORMAT_ZERO;
            goto repeat;
        case L' ':
            flag |= FORMAT_SPACE;
            goto repeat;
            break;
        case L'#':
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

int Vsprintf(CHAR16 *buf, const CHAR16 *fmt, va_list ap)
{
    CHAR16 *str, *s, qualifier;
    int     flag;
    int     width;
    for (str = buf; *fmt != L'\0'; fmt++)
    {
        if (*fmt != L'%')
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
        else if (*fmt == L'*')
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
        if (*fmt == L'.')
        {
            fmt++;
            if (IS_DIGIT(*fmt))
            {
                precision = skip_atoi(&fmt);
            }
            else if (*fmt == L'*')
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
        while (*fmt == L'h' || *fmt == L'l' || *fmt == L'L' || *fmt == L'Z')
        {
            qualifier = *fmt;
            fmt++;
        }
        switch (*fmt)
        {
            case L'%': /* %% */
                *str = L'%';
                str++;
                break;

            case L'c': /* %c */
                if (!(flag & FORMAT_LEFT))
                {
                    while (--width > 0)
                    {
                        *str++ = L' ';
                    }
                };
                *str++ = va_arg(ap, uint64_t);
                while (--width > 0)
                {
                    *str++ = L' ';
                }
                break;

            case L'd': /* %d */
            case L'i':
                flag |= FORMAT_SIGN;
            case L'u':
                if (qualifier == L'l')
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

            case L'o': /* %o */
                if (qualifier == L'l')
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

            case L'p':
                if (width == -1)
                {
                    width = 2 * sizeof(void *);
                    flag |= FORMAT_ZERO;
                }
                str = number_to_string(
                    str, va_arg(ap, uintptr_t), 16, width, precision, flag
                );
                break;

            case L's': /* %s */
                s = va_arg(ap, CHAR16 *);
                strcpy(str, s);
                str += strlen(s);
                break;

            case L'x': /* %x */
                flag |= FORMAT_SMALL;
            case L'X':
                if (qualifier == L'l')
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
            case L'f':
                str = float_to_string(
                    str, va_arg(ap, double), 10, width, precision, flag
                );
                break;

            default:
                *str++ = L'%';
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
    *str = L'\0';
    return strlen(buf);
}

int Sprintf(CHAR16 *buf, const CHAR16 *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int len;
    len = Vsprintf(buf, fmt, ap);
    va_end(ap);
    return len;
}

int Printf(const CHAR16 *fmt, ...)
{
    CHAR16  buf[256];
    va_list ap;
    va_start(ap, fmt);
    int len;
    len = Vsprintf(buf, fmt, ap);
    gST->ConOut->OutputString(gST->ConOut, buf);
    va_end(ap);
    return len;
}