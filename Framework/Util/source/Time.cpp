/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Time.hpp"

Time::Time()
{
    mTv.tv_sec = 0;
    mTv.tv_usec = 0;
}

Time Time::Get()
{
    Time t;
    gettimeofday(&t.mTv, nullptr);
    return t;
}

double Time::GetMs() const
{
    return mTv.tv_sec * 1000.0 + mTv.tv_usec / 1000.0;
}

double Time::GetUs() const
{
    return mTv.tv_sec * 1000000.0 + mTv.tv_usec;
}

Time Time::operator+(const Time& time) const
{
    Time result;
    result.mTv.tv_sec = mTv.tv_sec + time.mTv.tv_sec;
    result.mTv.tv_usec = mTv.tv_usec + time.mTv.tv_usec;

    // Handle microsecond overflow
    if (result.mTv.tv_usec >= 1000000) {
        result.mTv.tv_sec += result.mTv.tv_usec / 1000000;
        result.mTv.tv_usec %= 1000000;
    }

    return result;
}

Time Time::operator-(const Time& time) const
{
    Time result;
    result.mTv.tv_sec = mTv.tv_sec - time.mTv.tv_sec;
    result.mTv.tv_usec = mTv.tv_usec - time.mTv.tv_usec;

    // Handle microsecond underflow
    if (result.mTv.tv_usec < 0) {
        result.mTv.tv_sec -= 1;
        result.mTv.tv_usec += 1000000;
    }

    return result;
}
