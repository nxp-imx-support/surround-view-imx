/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "Events.hpp"
#include "Mesh.hpp"

#include <cinttypes>

class View : public NotifiableOnEvent
{
public:
    View(uint32_t x, uint32_t y, uint32_t width, uint32_t height, std::shared_ptr<Events> events);
    virtual ~View();

    static void Clear(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f);
    void BindFramebuffer(uint32_t id);
    void Finish(void);
    void Viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    void ViewportFull();
    void ViewportCorner(uint32_t id);
    void RenderQuad();

    uint32_t GetWidth();
    uint32_t GetHeight();

    virtual void OnEvent(const Event& inEvent) override;
    std::shared_ptr<Events> GetEvents();

protected:
    uint32_t mX;
    uint32_t mY;
    uint32_t mWidth;
    uint32_t mHeight;

    Mesh mQuad;
    std::shared_ptr<Events> mEvents;
    bool mIsActive = false;
    static uint32_t mViewCount;
};
