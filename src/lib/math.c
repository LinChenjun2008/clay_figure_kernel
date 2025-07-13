// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright 2025 LinChenjun
 */

#include <kernel/global.h>

#include <lib/math.h>

#define PI       3.14159265358979323846
#define TWO_PI   (2.0 * PI)
#define HALF_PI  (PI / 2.0)
#define INFINITY 1e308
#define NAN      (1.0 / 0.0)
PUBLIC double floor(double x)
{
    return (double)((long)x);
}

PUBLIC double ceil(double x)
{
    return (long)x + ((long)x != x ? 1 : 0);
}

PUBLIC double round(double x)
{
    return x > 0 ? floor(x + 0.5) : ceil(x - 0.5);
}

extern double asm_sqrt(double);
PUBLIC double sqrt(double x)
{
    return asm_sqrt(x);
}

PUBLIC double pow(double x, double y)
{
    if (x == 0)
    {
        if (y > 0)
        {
            return 0.0;
        }
        return NAN;
    }
    return exp(y * ln(x));
}

PUBLIC double exp(double x)
{
    // x = k * ln(2) + r
    double k = round(x * 1.4426950408889634);
    double r = x - k * 0.6931471807507668;

    double tmp1 = (1.0 / 120.0 + r / 720.0);
    double tmp2 = (1.0 / 24.0 + r * tmp1);
    double tmp3 = (1.0 / 6.0 + r * tmp2);
    double tmp4 = (0.5 + r * tmp3);
    double p    = r * (1.0 + r * tmp4);

    // exp(x) = 2^k * exp(r)
    int64_t i = (int64_t)(1023 + k) << 52;
    return *(double *)&i * (1.0 + p);
}

PUBLIC double ln(double x)
{
    int64_t bits = *(int64_t *)&x;

    int64_t exponent = ((bits >> 52) & 0x7FF) - 1023;
    double  mantissa = (bits & 0x000fffffffffffff) | 0x3ff0000000000000;
    *(int64_t *)&x   = mantissa;
    mantissa         = x;

    double y  = (mantissa - 1.0) / (mantissa + 1.0);
    double y2 = y * y;
    double p  = y * (2.0 + y2 * (0.6666666666666666 + y2 * 0.4));

    // ln(x) = exponent*ln(2) + ln(mantissa)
    return exponent * 0.6931471807507668 + p;
}

PUBLIC double fmod(double x, double y)
{
    return x - (int)(x / y) * y;
}

PUBLIC double sin(double x)
{
    x         = fmod(x + PI, TWO_PI) - PI;
    double x2 = x * x;
    double x3 = x2 * x;
    double x5 = x3 * x2;
    double x7 = x5 * x2;
    return x - x3 / 6.0 + x5 / 120.0 - x7 / 5040.0;
}

PUBLIC double cos(double x)
{
    return sin(x + HALF_PI);
}

PUBLIC double asin(double x)
{
    double x2 = x * x;
    return atan(x / sqrt(1 - x2));
}

PUBLIC double acos(double x)
{
    return HALF_PI - asin(x);
}

PUBLIC double atan(double x)
{
    if (x > 1 || x < -1)
    {
        return HALF_PI - atan(1 / x);
    }
    double x2 = x * x;
    double x3 = x2 * x;
    double x5 = x3 * x2;
    double x7 = x5 * x2;
    return x - x3 / 3.0 + x5 / 5.0 - x7 / 7.0;
}

PUBLIC double atan2(double y, double x)
{
    if (x > 0)
    {
        return atan(y / x);
    }
    if (x < 0)
    {
        if (y >= 0)
        {
            return atan(y / x) + PI;
        }
        else
        {
            return atan(y / x) - PI;
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