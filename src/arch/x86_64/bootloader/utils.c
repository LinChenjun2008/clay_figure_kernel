// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include "bootloader.h"

#define FORMAT_LEFT    (1 << 0)
#define FORMAT_SPACE   (1 << 1)
#define FORMAT_ZERO    (1 << 2)
#define FORMAT_SIGN    (1 << 3)
#define FORMAT_PLUS    (1 << 4)
#define FORMAT_SPECIAL (1 << 5)
#define FORMAT_SMALL   (1 << 6)

#define IS_DIGIT(c) ((c) >= L'0' && (c) <= L'9')

static char16_t *strcpy(char16_t *dst, const char16_t *src)
{
    char16_t *ret = dst;
    while ((*dst++ = *(char16_t *)src++) != L'\0');
    return ret;
}

static size_t strlen(const char16_t *str)
{
    const char16_t *end = str;
    while (*end++ != 0);
    return end - str - 1;
}

static int skip_atoi(const char16_t **s)
{
    int i = 0;
    while (IS_DIGIT(**s))
    {
        i = i * 10 + *((*s)++) - L'0';
    }
    return i;
}

static char16_t *number_to_string(
    char16_t *str,
    uint64_t  num,
    int       base,
    int       width,
    int       precision,
    int       flag
)
{
    char16_t        pad, sign, tmp[50];
    const char16_t *digits;
    digits = (char16_t *)L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if (flag & FORMAT_SMALL)
    {
        digits = (char16_t *)L"0123456789abcdefghijklmnopqrstuvwxyz";
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

static char16_t *float_to_string(
    char16_t *str,
    double    num,
    int       base,
    int       width,
    int       precision,
    int       flag
)
{
    char16_t        pad, integer_part[50], fract_part[50];
    const char16_t *digits;
    digits = (char16_t *)L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int    i, integer_width, fract_width;
    int    integer  = (int)num;
    double mantissa = (num - integer);
    pad             = (flag & FORMAT_ZERO) ? L'0' : L' ';

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

static int get_flag(const char16_t **fmt)
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

int vsprintf(char16_t *buf, const char16_t *fmt, va_list ap)
{
    char16_t *str, *s, qualifier;
    int       flag;
    int       width;
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
                s = va_arg(ap, char16_t *);
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

int sprintf(char16_t *buf, const char16_t *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int len;
    len = vsprintf(buf, fmt, ap);
    va_end(ap);
    return len;
}

int printf(const char16_t *fmt, ...)
{
    char16_t buf[256];
    va_list  ap;
    va_start(ap, fmt);
    int len;
    len = vsprintf(buf, fmt, ap);
    system_table->con_out->output_string(system_table->con_out, buf);
    va_end(ap);
    return len;
}

char16_t *char_to_char16(char *ch, char16_t *in_ch16)
{
    char16_t *ch16 = in_ch16;
    while (*ch != '\0')
    {
        *ch16++ = *ch++;
    }
    *ch16 = 0;
    return in_ch16;
}

int compare_guid(efi_guid_t *guid1, efi_guid_t *guid2)
{
    return (
        (guid1->data1 == guid2->data1) && (guid1->data2 == guid2->data2) &&
        (guid1->data3 == guid2->data3) &&
        (guid1->data4[0] == guid2->data4[0]) &&
        (guid1->data4[1] == guid2->data4[1]) &&
        (guid1->data4[2] == guid2->data4[2]) &&
        (guid1->data4[3] == guid2->data4[3]) &&
        (guid1->data4[4] == guid2->data4[4]) &&
        (guid1->data4[5] == guid2->data4[5]) &&
        (guid1->data4[6] == guid2->data4[6]) &&
        (guid1->data4[7] == guid2->data4[7])
    );
}