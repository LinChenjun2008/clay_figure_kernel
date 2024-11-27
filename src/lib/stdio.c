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

PRIVATE char * number_to_string(char * str,uint64_t num,int base,int width,int precision,int flag)
{
	char pad,sign,tmp[50];
	const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	if (flag & FORMAT_SMALL)
    {
        digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    }
	if (base < 2 || base > 36)
		return 0;
	pad = (flag & FORMAT_ZERO) ? '0' : ' ' ;
	sign = 0;
	if (flag & FORMAT_SIGN && (int64_t)num < 0)
    {
		sign = '-';
		num = -num;
	} 
    else
	{
        sign=(flag & FORMAT_PLUS) ? '+' : ((flag & FORMAT_SPACE) ? ' ' : 0);
    }
	if (sign) width--;
	if (flag & FORMAT_SPECIAL)
	{
        if (base == 16) width -= 2;
		if (base ==  8) width -= 1;
    }
	int i = 0;
	if (num == 0) { tmp[i++]='0'; }
	else 
    {
        while (num != 0)
        {
            tmp[i++]=digits[num % base];
            num /= base;
        }
    }
	if (i > precision) { precision=i; }
	width -= precision;
	if (!(flag & (FORMAT_ZERO | FORMAT_LEFT)))
	{
        while(width-- > 0) *str++ = ' ';
    }
	if (sign) { *str++ = sign; }
	if (flag & FORMAT_SPECIAL)
	{
        if (base == 8) { *str++ = '0'; }
		if (base==16) 
		{
			*str++ = '0';
			*str++ = digits[33];
		}
    }
	if (!(flag & FORMAT_LEFT))
    {
        while(width-- > 0) *str++ = pad;
    }

	while(i < precision--) *str++ = '0';
	while(i-- > 0) *str++ = tmp[i];
	while(width-- > 0) *str++ = ' ';
	return str;
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
    if (flag & FORMAT_LEFT) { flag &= ~FORMAT_ZERO; }
    return flag;
}

PUBLIC int vsprintf(char *buf,const char *fmt,va_list ap)
{
    char* str,*s,qualifier;
    int flag;
    int width;
    for (str = buf;*fmt != '\0';fmt++)
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
            flag |= FORMAT_SIGN;
        case 'u':
            if (qualifier == 'l')
            {
                str = number_to_string(str,va_arg(ap,long long int),10,width,-1,flag);
            }
            else
            {
                str = number_to_string(str,va_arg(ap,int),10,width,-1,flag);
            }
            break;

        case 'o': /* %o */
            if (qualifier == 'l')
            {
                str = number_to_string(str,va_arg(ap,long long int),8,width,-1,flag);
            }
            else
            {
                str = number_to_string(str,va_arg(ap,int),8,width,-1,flag);
            }
            break;

        case 'p':
            width = 2 * sizeof(void*);
            flag |= FORMAT_ZERO;
            str = number_to_string(str,va_arg(ap,addr_t),16,width,16,flag);
            break;

        case 's': /* %s */
            s = va_arg(ap,char*);
            strcpy(str,s);
            str += strlen(s);
            break;

        case 'x': /* %x */
            flag |= FORMAT_SMALL;
        case 'X':
            if (qualifier == 'l')
            {
                str = number_to_string(str,va_arg(ap,long long int),16,width,-1,flag);
            }
            else
            {
                str = number_to_string(str,va_arg(ap,int),16,width,-1,flag);
            }
            break;

        default:
            *str++ = '%';
            if(*fmt) { *str++ = *fmt; }
            else     { fmt--; }
            break;
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