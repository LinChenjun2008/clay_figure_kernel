#ifndef __STDIO_H__
#define __STDIO_H__

#include <std/stdarg.h>

int vsprintf(char* buf,const char* fmt,va_list ap);
int sprintf(char* buf,const char* fmt,...);

#endif