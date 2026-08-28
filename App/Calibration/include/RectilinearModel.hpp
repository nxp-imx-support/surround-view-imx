/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "CameraModel.hpp"

class RectilinearModel : public CameraModel
{
public:
    RectilinearModel(uint32_t width, uint32_t height);
    virtual ~RectilinearModel() = default;

    void UpdateLUT();
};
