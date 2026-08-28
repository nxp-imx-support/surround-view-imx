// Copyright 2026 NXP

#include "View.hpp"

#include "Graphics.hpp"

uint32_t View::mViewCount = 0;

View::View(uint32_t x, uint32_t y, uint32_t width, uint32_t height, std::shared_ptr<Events> events)
    : NotifiableOnEvent(events)
    , mX(x)
    , mY(y)
    , mWidth(width)
    , mHeight(height)
{
    // Set focus on first view created
    mIsActive = (mViewCount++ == 0);
    mQuad.LoadQuad();

    glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ViewportFull();

    mEvents = std::make_shared<Events>();
}

View::~View()
{
    --mViewCount;
}

void View::BindFramebuffer(uint32_t id)
{
    glBindFramebuffer(GL_FRAMEBUFFER, id);
}

void View::Clear(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void View::Finish(void)
{
    glFinish();
}

void View::Viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    glViewport(x, y, width, height);
}

void View::ViewportFull()
{
    Viewport(mX, mY, mWidth, mHeight);
}

void View::ViewportCorner(uint32_t id)
{
    Viewport(mX + (id & 1U) * mWidth / 2, mY + ((id >> 1U) & 1U) * mHeight / 2, mWidth / 2, mHeight / 2);
}

void View::RenderQuad()
{
    mQuad.Render();
}

uint32_t View::GetWidth()
{
    return mWidth;
}

uint32_t View::GetHeight()
{
    return mHeight;
}

void View::OnEvent(const Event& event)
{
    // Forward events if mouse is over view
    if (event.Type == EventType::MousePress || event.Type == EventType::MouseScroll) {
        mIsActive = (event.MousePos.x >= mX && event.MousePos.x < (mX + mWidth) && event.MousePos.y >= mY && event.MousePos.y < (mY + mHeight));
    }
    if (mIsActive) {
        mEvents->PostEvent(event);
    }
}

std::shared_ptr<Events> View::GetEvents()
{
    return mEvents;
}
