/*
   Copyright 2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <lib/math.h>

#define PI 3.14159265358979323846
#define TWO_PI (2.0 * PI)
#define HALF_PI (PI / 2.0)
#define INFINITY 1e308

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
    x = fmod(x + PI, TWO_PI) - PI;
    double x2 = x * x;
    double x3 = x2 * x;
    double x5 = x3 * x2;
    double x7 = x5 * x2;
    return x - x3/6 + x5/120 - x7/5040;
}

PUBLIC double cos(double x)
{
    return sin(x + HALF_PI);
}

PUBLIC double asin(double x)
{
    return atan(x / sqrt(1 - x*x));
}

PUBLIC double acos(double x)
{
    return HALF_PI - asin(x);
}

PUBLIC double atan(double x)
{
    if (x > 1 || x < -1)
    {
        return HALF_PI - atan(1/x);
    }
    double x2 = x * x;
    double x3 = x2 * x;
    double x5 = x3 * x2;
    double x7 = x5 * x2;
    return x - x3/3 + x5/5 - x7/7;
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
            return atan(y/x) + PI;
        }
        else
        {
            return atan(y/x) - PI;
        }
    }
    if (y > 0)
    {
        return HALF_PI;
    }
    if (y < 0)
    {
        return -HALF_PI;
    }
    return INFINITY;
}

PUBLIC double fabs(double x)
{
    return x > 0 ? x : -x;
}