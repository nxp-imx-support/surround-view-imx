/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <cmath>

struct Point
{
    int x, y;
    Point()
        : x(0)
        , y(0) { };
    Point(int x, int y)
        : x(x)
        , y(y) { };

    static inline float Distance(const Point& a, const Point& b)
    {
        return sqrtf((float)(a.x - b.x) * (a.x - b.x) + (float)(a.y - b.y) * (a.y - b.y));
    }

    Point operator+(const Point& p) const
    {
        return Point { x + p.x, y + p.y };
    }
    Point operator-(const Point& p) const
    {
        return Point { x - p.x, y - p.y };
    }
};
