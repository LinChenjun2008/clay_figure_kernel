/*
   Copyright 2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <lib/math.h>

PUBLIC double floor(double x)
{
    return (double)((long)x);
}

PUBLIC double ceil(double x)
{
    return (long)x + ((long)x != x ? 1 : 0);
}

extern double asm_sqrt(double);
PUBLIC double sqrt(double x)
{
    return asm_sqrt(x);
}

PUBLIC double pow(double x,double y)
{
    double res = 1;
    int i = 1;
    if (y == 0)
    {
        return res;
    }
    while (i++ <= y)
    {
        res *= x;
    }
    return res;
}

PUBLIC double fmod(double x,double y)
{
    return x - (int)(x / y) * y;
}

PUBLIC double sin(double x)
{
    return x - pow(x,3)/6 + pow(x,5)/120;
}

PUBLIC double cos(double x)
{
    return 1 - pow(x,2)/2 + pow(x,4)/24 - pow(x,6)/720;
}

PUBLIC double asin(double x)
{
    return x + pow(x,3)/6 + pow(x,5) * 3/40;
}

PUBLIC double acos(double x)
{
    return 1.570795 - asin(x);
}

PUBLIC double atan(double x)
{
    if (x > 1 || x < -1)
    {
        return 1.570795 - atan(1/x);
    }
    return x - pow(x,3)/3 + pow(x,5)/5;
}

PUBLIC double atan2(double y,double x)
{
    if (x > 0)
    {
        return atan(y/x);
    }
    if (x < 0)
    {
        if (y >= 0)
        {
            return atan(y/x) + 3.1415926;
        }
        else
        {
            return atan(y/x) - 3.1415926;
        }
    }
    if (y > 0)
    {
        return 1.570795;
    }
    if (y < 0)
    {
        return -1.570795;
    }
    return 0.0 / 0.0;
}

PUBLIC double fabs(double x)
{
    return x > 0 ? x : -x;
}