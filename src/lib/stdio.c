/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.

Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU Lesser General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 宽通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 宽通用公共许可证，了解详情。

你应该随程序获得一份 GNU 宽通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

#include <kernel/global.h>
#include <std/string.h>
#include <std/stdarg.h>

#define FORMAT_SPACE   0
#define FORMAT_LEFT    (1 << 0)
#define FORMAT_ZERO    (1 << 1)
#define FORMAT_SIGN    (1 << 2)
#define FORMAT_SPECIAL (1 << 3)
#define FORMAT_SMALL   (1 << 4)
#define FORMAT_LONG    (1 << 5)
#define FORMAT_SHORT   (1 << 6)

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

PRIVATE void signed_int_to_string(int64_t a,char* str,int base,int small)
{
    const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if (small)
    {
        digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    }
    int i;
    int is_negative;
    is_negative = a;
    if (a < 0)
    {
        a = -a;
    }
    i = 0;
    do
    {
        str[i] = digits[a % base];
        i++;
        a = a / base;
    } while (a > 0);
    if (is_negative < 0)
    {
        str[i] = '-';
        i++;
    }
    str[i] = '\0';
    char* p = str;
    char* q = str;
    char tmp;
    while (*q != '\0')
    {
        q++;
    }
    q--;
    while (q > p)
    {
        tmp = *p;
        *p = *q;
        p++;
        *q = tmp;
        q--;
    }
    return;
}

PRIVATE void unsigned_int_to_string(uint64_t a,char* str,int base,int small)
{
    const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if (small)
    {
        digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    }
    int i;
    i = 0;
    do
    {
        str[i] = digits[a % base];
        i++;
        a = a / base;
    } while (a > 0);
    str[i] = '\0';
    char* p = str;
    char* q = str;
    char tmp;
    while (*q != '\0')
    {
        q++;
    }
    q--;
    while (q > p)
    {
        tmp = *p;
        *p = *q;
        p++;
        *q = tmp;
        q--;
    }
    return;
}

PRIVATE int get_flag(const char **fmt)
{
    int flag = 0;
    repeat:
        (*fmt)++;
    /* %后可以加'-',' ','0','#' */
    switch(**fmt)
    {
    case '-':
        flag |= FORMAT_LEFT;
        goto repeat;
        break;
    case '0':
        flag |= FORMAT_ZERO;
        goto repeat;
    case ' ':
        goto repeat;
        break;
    case '#':
        flag |= FORMAT_SPECIAL;
        goto repeat;
        break;
    case 'l':
        flag |= FORMAT_LONG;
        goto repeat;
    case 'h':
        flag |= FORMAT_SHORT;
        goto repeat;
    default:
        break;
    }
    if (flag & FORMAT_LEFT) { flag &= ~FORMAT_LEFT; }
    return flag;
}

PUBLIC int vsprintf(char *buf,const char *fmt,va_list ap)
{
    char* str,*s = 0,digits[69],qualifier;
    int flag;
    int width;
    for (str = buf;*fmt != '\0';fmt++)
    {
        if (*fmt != '%')
        {
            *str++ = *fmt;
            continue;
        }
        digits[0] = 0;
        flag = get_flag(&fmt);

        width = -1;
        if (IS_DIGIT(*fmt))
        {
            width = skip_atoi(&fmt);
        }
        qualifier = 0;
        if(*fmt == 'h' || *fmt == 'l' || *fmt == 'L' || *fmt == 'Z')
        {
            qualifier = *fmt;
            fmt++;
        }
        switch(*fmt)
        {
        case '%':/* %% */
            *str = '%';
            str++;
            break;
        case 'c':/* %c */
            if (!(flag & FORMAT_LEFT))
            {
                while (--width > 0) {*str++ = ' ';}
            };
            *str++ = va_arg(ap,uint64_t);
            while (--width > 0) {*str++ = ' ';}
            break;
        case 'd': /* %d */
        case 'i':
            s = digits;
            signed_int_to_string(va_arg(ap,uint64_t),s,10,0);
            break;
        case 'o': /* %o */
            s = digits;
            if (qualifier == 'l')
            {
                unsigned_int_to_string(va_arg(ap,uint64_t),s,8,0);
            }
            else
            {
                unsigned_int_to_string(va_arg(ap,int),s,8,0);
            }
            break;
        case 'p':
            s = digits;
            unsigned_int_to_string((addr_t)va_arg(ap,addr_t),digits,16,0);
            break;
        case 's': /* %s */
            s = va_arg(ap,char*);
            strcpy(str,s);
            break;
        case 'u': /* %u */
            s = digits;
            unsigned_int_to_string(va_arg(ap,uint64_t),s,10,0);
            break;
        case 'x': /* %x */
            s = digits;
            if (qualifier == 'l')
            {
                unsigned_int_to_string(va_arg(ap,uint64_t),s,16,1);
            }
            else
            {
                unsigned_int_to_string(va_arg(ap,int),s,16,1);
            }
            break;
        case 'X':
            s = digits;
            if (qualifier == 'l')
            {
                unsigned_int_to_string(va_arg(ap,uint64_t),s,16,1);
            }
            else
            {
                unsigned_int_to_string(va_arg(ap,int),s,16,1);
            }
            unsigned_int_to_string(va_arg(ap,uint64_t),s,16,0);
            break;
        default:
            *str++ = '%';
            if(*fmt) { *str++ = *fmt; }
            else     { fmt--; }
            break;
        }
        width -= strlen(s);
        if (flag & FORMAT_SPECIAL)
        {
            width -= 2;
        }
        if ((flag & FORMAT_SPECIAL) && (flag & FORMAT_ZERO))
        {
            *str++ = '0';
            *str++ = 'x';
        }
        while (width > 0 && !(flag & FORMAT_LEFT))
        {
            *str++ = (flag & FORMAT_ZERO ? '0' : ' ');
            width--;
        }
        if ((flag & FORMAT_SPECIAL) && !(flag & FORMAT_ZERO))
        {
            *str++ = '0';
            *str++ = 'x';
        }
        strcpy(str,s);
        str += strlen(s);
        /* 左对齐的情况 */
        while (width > 0 && flag & FORMAT_LEFT)
        {
            *str = ' ';
            str++;
            width--;
        }
    }
    *str = '\0';
    return strlen(buf);
}

PUBLIC int sprintf(char* buf,const char* fmt,...)
{
    va_list ap;
    va_start(ap,fmt);
    int len;
    len = vsprintf(buf,fmt,ap);
    va_end(ap);
    return len;
}