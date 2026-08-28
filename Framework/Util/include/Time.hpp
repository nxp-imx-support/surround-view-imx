/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <sys/time.h>

class Time
{
public:
    Time();
    ~Time() = default;

    static Time Get();
    double GetMs() const;
    double GetUs() const;

    Time operator+(const Time& time) const;
    Time operator-(const Time& time) const;

private:
    struct timeval mTv;
};
