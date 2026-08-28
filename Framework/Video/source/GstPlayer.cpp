/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "GstPlayer.hpp"

#include "Log.hpp"
#include "Texture.hpp"
#include "Window.hpp"

GstLib::GstLib()
{
    gst_init(nullptr, nullptr);
}

GstLib::~GstLib()
{
    gst_deinit();
}

GstLib GstPlayer::mGst;

GstPlayer::GstPlayer(std::string source, int width, int height, std::shared_ptr<Window> window)
    : VideoStream(source, width, height)
    , mPipeline(nullptr)
    , mBus(nullptr)
    , mGstDisplay(nullptr)
    , mGlContext(nullptr)
    , mIsPrerollDone(false)
    , mInitialized(false)
    , mBufferLast(nullptr)
    , mBufferRender(nullptr)
    , mLooping(false)
    , mListener(nullptr)
{
    mPipelineCommand = source;

    // Get EGL display and context
    mGstDisplay = gst_gl_display_egl_new_with_egl_display(window->GetDisplay());
    mGlContext = gst_gl_context_new_wrapped(GST_GL_DISPLAY(mGstDisplay),
        reinterpret_cast<guintptr>(window->GetContext()),
        GST_GL_PLATFORM_EGL, GST_GL_API_GLES2);

    // Launch pipeline
    GError* error = nullptr;
    g_print("Loading GStreamer pipeline: %s\n", mPipelineCommand.data());
    mPipeline = gst_parse_launch(mPipelineCommand.data(), &error);
    if (error != nullptr) {
        g_print("gst_parse_launch fails with error: %s\n", error->message);
        g_clear_error(&error);
        throw std::runtime_error("Failed to load GStreamer pipeline");
    }

    // Watch bus
    mBus = gst_pipeline_get_bus(GST_PIPELINE(mPipeline));
    gst_bus_add_watch(mBus, GstPlayer::OnBusMessage, static_cast<gpointer>(this));

    mSink = gst_bin_get_by_name(GST_BIN(mPipeline), "sv3dsink");

    // Enable sink's signals emission.
    g_object_set(mSink, "emit-signals", TRUE, nullptr);

    // Call OnNewSample() every time the sink receives a buffer.
    mSinkCbHandlerId = g_signal_connect(G_OBJECT(mSink), "new-sample", G_CALLBACK(GstPlayer::OnNewSample),
        static_cast<gpointer>(this));

    // Get notify on state changed of sink's pad.
    GstPad* sinkPad = gst_element_get_static_pad(mSink, "sink");
    gst_pad_add_probe(sinkPad, GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM, GstPlayer::OnQuery,
        static_cast<gpointer>(this), nullptr);
    gst_pad_add_probe(sinkPad, (GstPadProbeType)(GST_PAD_PROBE_TYPE_EVENT_BOTH), GstPlayer::OnEvent,
        static_cast<gpointer>(this), nullptr);

    // Start pipeline
    GstStateChangeReturn stateReturn = gst_element_set_state(mPipeline, GST_STATE_PAUSED);
    if (stateReturn == GST_STATE_CHANGE_FAILURE) {
        throw std::runtime_error("Failed to play GStreamer pipeline");
    }

    mInitialized = true;
}

GstPlayer::~GstPlayer()
{
    Stop();

    if (mInitialized) {
        g_signal_handler_disconnect(mSink, mSinkCbHandlerId);
        GstStateChangeReturn ret = gst_element_set_state(mPipeline, GST_STATE_NULL);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            LogError("Failed to deinit GStreamer pipeline");
        }

        // Release buffers
        mBufferLock.lock();
        if (mBufferLast != nullptr && mBufferLast != mBufferRender) {
            gst_buffer_unref(mBufferLast);
        }
        mBufferLast = nullptr;
        if (mBufferRender != nullptr) {
            gst_buffer_unref(mBufferRender);
            mBufferRender = nullptr;
        }
        mBufferLock.unlock();

        gst_bus_remove_watch(mBus);
        gst_object_unref(GST_OBJECT(mBus));
        gst_object_unref(GST_OBJECT(mPipeline));
        mInitialized = false;
        mIsPrerollDone = false;
    }
}

bool GstPlayer::Start(void)
{
    if (mInitialized) {
        gst_element_set_state(mPipeline, GST_STATE_PLAYING);
        return true;
    }
    return false;
}

void GstPlayer::Stop(void)
{
    if (mInitialized) {
        gst_element_set_state(mPipeline, GST_STATE_PAUSED);
    }
}

void GstPlayer::SetListener(GstPlayerListener* listener)
{
    mListener = listener;
}

std::shared_ptr<Texture> GstPlayer::GetTexture()
{
    std::shared_ptr<Texture> texture = nullptr;

    if (mInitialized == true) {
        mBufferLock.lock();
        if (mBufferRender != nullptr && mBufferLast != mBufferRender) {
            // New buffer available, release previously rendered buffer
            gst_buffer_unref(mBufferRender);
            mBufferRender = nullptr;
        }
        mBufferRender = mBufferLast;

        if (mBufferRender != nullptr) {
            // Get OpenGL texture ID
            GstMemory* memory = gst_buffer_peek_memory(mBufferRender, 0);
            if (gst_is_gl_memory(memory) != 0) {
                auto glMemory = reinterpret_cast<GstGLMemory*>(memory);
                GLuint id = glMemory->tex_id;
                GLenum target = gst_gl_texture_target_to_gl(glMemory->tex_target);
                texture = std::make_shared<Texture>(mWidth, mHeight, id, target);
            } else {
                throw std::runtime_error(
                    "Input from appsink is not an OpenGL texture. Consider using "
                    "glupload in the pipeline.");
            }
        }
        mBufferLock.unlock();
    }

    return texture;
}

GstFlowReturn GstPlayer::OnNewSample(GstElement* appsink, gpointer data)
{
    auto* ctx = static_cast<GstPlayer*>(data);

    //  Get buffer from GStreamer
    GstSample* sample = nullptr;
    g_signal_emit_by_name(appsink, "pull-sample", &sample);

    if (sample != nullptr) {
        ctx->mBufferLock.lock();

        GstBuffer* buffer = gst_sample_get_buffer(sample);

        if (ctx->mBufferLast != nullptr && ctx->mBufferLast != ctx->mBufferRender) {
            // Previous stored buffer has not been rendered, release it.
            gst_buffer_unref(ctx->mBufferLast);
            ctx->mBufferLast = nullptr;
        }
        // Lock new buffer
        ctx->mBufferLast = gst_buffer_ref(buffer);

        gst_sample_unref(sample);

        ctx->mBufferLock.unlock();

        ctx->NotifyNewFrame();
    }

    return GST_FLOW_OK;
}

GstPadProbeReturn GstPlayer::OnQuery(GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    auto* ctx = static_cast<GstPlayer*>(data);
    GstQuery* query = GST_PAD_PROBE_INFO_QUERY(info);

    switch (GST_QUERY_TYPE(query)) {
    case GST_QUERY_CONTEXT:
        if (gst_gl_handle_context_query(ctx->mPipeline, query,
                reinterpret_cast<GstGLDisplay*>(ctx->mGstDisplay),
                nullptr, ctx->mGlContext)) {
            return GST_PAD_PROBE_HANDLED;
        }
        break;
    default:
        break;
    }

    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn GstPlayer::OnEvent(GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    auto* ctx = static_cast<GstPlayer*>(data);
    GstEvent* event = GST_PAD_PROBE_INFO_EVENT(info);

    if (GST_EVENT_TYPE(event) == GST_EVENT_CAPS) {
        GstCaps* caps;
        gst_event_parse_caps(event, &caps);

        GstStructure* properties = gst_caps_get_structure(caps, 0);
        if (gst_structure_get_int(properties, "width", &ctx->mWidth) != 0 && gst_structure_get_int(properties, "height", &ctx->mHeight) != 0) {
            return GST_PAD_PROBE_HANDLED;
        } else {
            g_print("GStreamer Error: Could not find stream dimensions\n");
        }
    }

    return GST_PAD_PROBE_OK;
}

gboolean GstPlayer::OnBusMessage(GstBus* bus, GstMessage* msg, gpointer data)
{
    auto* ctx = static_cast<GstPlayer*>(data);

    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_EOS:
        // Restart pipeline
        if (ctx->mLooping) {
            if (!gst_element_seek(ctx->mPipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                    GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
                g_print("GStreamer Error: Failed to restart pipeline\n");
            }
        }

        break;
    case GST_MESSAGE_ERROR: {
        gchar* debug = nullptr;
        GError* error = nullptr;

        gst_message_parse_error(msg, &error, &debug);

        g_print("GStreamer Error message: %s\n", error->message);
        g_error_free(error);

        if (debug != nullptr) {
            g_print("GStreamer Debug message: %s\n", debug);
            g_free(debug);
        }

        break;
    }
    case GST_MESSAGE_WARNING: {
        gchar* debug = nullptr;
        GError* error = nullptr;

        gst_message_parse_warning(msg, &error, &debug);

        g_print("GStreamer Warning message: %s\n", error->message);
        g_error_free(error);

        if (debug != nullptr) {
            g_print("GStreamer Debug message: %s\n", debug);
            g_free(debug);
        }
        break;
    }
    case GST_MESSAGE_ASYNC_DONE: {
        if (ctx->mIsPrerollDone == false) {
            ctx->mIsPrerollDone = true;
            ctx->NotifyPrerollDone();
        }
        break;
    }
    default:
        break;
    }

    return TRUE;
}

void GstPlayer::NotifyNewFrame()
{
    if (mListener != nullptr) {
        mListener->onNewFrame();
    }
}

void GstPlayer::NotifyPrerollDone()
{
    if (mListener != nullptr) {
        mListener->onPrerollDone();
    }
}
