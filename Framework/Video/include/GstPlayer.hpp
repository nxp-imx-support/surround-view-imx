/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "VideoStream.hpp"

#include <gst/gl/egl/gstgldisplay_egl.h>
#include <gst/gl/gl.h>
#include <gst/gst.h>
#include <mutex>
#include <string>

class GstLib
{
public:
    GstLib();
    ~GstLib();
};

class GstPlayerListener
{
public:
    virtual void onNewFrame() = 0;
    virtual void onPrerollDone() = 0;
};

class GstPlayer : public VideoStream
{
public:
    GstPlayer(std::string source, int width, int height, std::shared_ptr<Window> window);
    virtual ~GstPlayer();

    // Inherited from VideoStream
    virtual bool Start(void) override;
    virtual void Stop(void) override;
    virtual std::shared_ptr<Texture> GetTexture() override;

    void SetListener(GstPlayerListener* listener);
    bool isPrerollDone() { return mIsPrerollDone; };

protected:
    static GstFlowReturn OnNewSample(GstElement* appsink, gpointer data);
    static GstPadProbeReturn OnQuery(GstPad* pad, GstPadProbeInfo* info, gpointer data);
    static GstPadProbeReturn OnEvent(GstPad* pad, GstPadProbeInfo* info, gpointer data);
    static gboolean OnBusMessage(GstBus* bus, GstMessage* msg, gpointer data);

    void NotifyNewFrame();
    void NotifyPrerollDone();

private:
    std::string mPipelineCommand;
    GstElement* mPipeline;
    GstBus* mBus;
    GstGLDisplayEGL* mGstDisplay;
    GstGLContext* mGlContext;
    bool mIsPrerollDone;
    bool mInitialized;
    bool mLooping;

    std::mutex mBufferLock;
    GstBuffer* mBufferLast;
    GstBuffer* mBufferRender;

    static GstLib mGst;
    GstPlayerListener* mListener;

    GstElement* mSink;
    gulong mSinkCbHandlerId;
};
