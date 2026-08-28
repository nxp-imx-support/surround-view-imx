/*
 * Copyright 2017, 2022 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

static inline int previous_id(int x, int max_val)
{
    int return_value = max_val;
    if (x > 0) {
        return_value = x - 1;
    }
    return return_value;
}

static inline int next_id(int x, int max_val)
{
    int return_value = 0;
    if (x < max_val) {
        return_value = x + 1;
    }
    return return_value;
}
