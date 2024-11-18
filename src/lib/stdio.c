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

#define FORMAT_LEFT 0
#define FORMAT_RIGHT 1
#define FORMAT_SPACE 0
#define FORMAT_ZERO 1

PRIVATE void itoa(int64_t a,char* str,int base)
{
    static char digits[37] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
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

PRIVATE void utoa(uint64_t a,char* str,int base)
{
    static char digits[37] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
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

PUBLIC int vsprintf(char* buf,const char* fmt,va_list ap)
{
    char* str,*s = 0,digits[69];
    int repeat;
    int flage;
    int align;
    int width;
    for (str = buf;*fmt != '\0';fmt++)
    {
        if (*fmt != '%')
        {
            *str = *fmt;
            str++;
            continue;
        }
        fmt++;/* %后面的字符 */
        digits[0] = 0;
        flage = 0;
        align = FORMAT_RIGHT;
        repeat = 1;
        while (repeat)
        {
            /* %后可以加'+','-',' ','0','#' */
            switch(*fmt)
            {
            case '+':
                align = FORMAT_RIGHT;
                break;
            case '-':
                align = FORMAT_LEFT;
                break;
            case '0':
                flage |= FORMAT_ZERO;
            case ' ':
                break;
            case '#':
                break;
            default:
                fmt--; /* 与后面的fmt++抵消 */
                repeat = 0;
                break;
            }
            fmt++;
        }
        width = 0;
        while (*fmt >= '0' && *fmt <= '9')
        {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }
        if (width == 0)
        {
            align = FORMAT_LEFT;
        }
        switch(*fmt)
        {
        case '%':/* %% */
            *str = '%';
            str++;
            break;
        case 'c':/* %c */
            *str = va_arg(ap,uint64_t);
            str++;
            break;
        case 'd': /* %d */
            s = digits;
            itoa(va_arg(ap,uint64_t),s,10);
            break;
        case 'o': /* %o */
            s = digits;
            itoa(va_arg(ap,uint64_t),s,8);
            break;
        case 'p':
            s = digits;
            digits[0] = '0';
            digits[1] = 'x';
            utoa((addr_t)va_arg(ap,addr_t),digits+2,16);
            break;
        case 's': /* %s */
            s = va_arg(ap,char*);
            strcpy(str,s);
            break;
        case 'u': /* %u */
            s = digits;
            utoa(va_arg(ap,uint64_t),s,10);
            break;
        case 'x': /* %x */
            s = digits;
            utoa(va_arg(ap,uint64_t),s,16);
            break;
        }
        width -= strlen(s);
        while (width > 0 && align == FORMAT_RIGHT)
        {
            *str = (flage & FORMAT_ZERO ? '0' : ' ');
            str++;
            width--;
        }
        strcpy(str,s);
        str += strlen(s);
        /* 左对齐的情况 */
        while (width > 0 && align == FORMAT_LEFT)
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