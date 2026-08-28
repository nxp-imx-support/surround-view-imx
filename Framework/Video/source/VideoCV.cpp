/*
 * Copyright 2017, 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "VideoCV.hpp"

#include "Log.hpp"

#include <unistd.h>

int VideoCV::mThreadId = 0;
int VideoCV::mThreadIdCurrent = 0;
int VideoCV::mFrameCurrent = 0;
pthread_mutex_t VideoCV::mFrameMutex = PTHREAD_MUTEX_INITIALIZER;

VideoCV::VideoCV(std::string source, int width, int height)
    : VideoStream(source, width, height)
{
    mBufferIndex = -1;

    pthread_mutex_lock(&mFrameMutex);
    mThreadId++;
    pthread_mutex_unlock(&mFrameMutex);
}

VideoCV::~VideoCV(void)
{
    Stop();
}

bool VideoCV::Start(void)
{
    for (int i = 0; i < BUFFER_NUM; i++) {
        mFrames[i] = cv::Mat(mHeight, mWidth, CV_8UC3);
        Buffer buffer;
        LogInfo("%d startCapturing framedata %p", i, mFrames[i].data);
        buffer.start = (unsigned char*)(mFrames[i].data);
        buffer.offset = 0;
        buffer.width = mWidth;
        buffer.height = mHeight;
        buffer.format = VideoFormat::RGB;
        mTextures.push_back(std::make_shared<TextureBuffer>(buffer));
    }

    mIsRunning = true;
    if (pthread_create(&mCaptureThread, NULL, VideoCV::CaptureThread, this) != 0) {
        LogError("Cannot create capture thread");
        mIsRunning = false;
    }

    return mIsRunning;
}

void VideoCV::Stop(void)
{
    mIsRunning = false;
    void* status = 0;
    if (mCaptureThread != 0U) {
        pthread_join(mCaptureThread, &status);
        if (status != 0) {
            LogError("Pthread join %s failed", mSource.c_str());
        }
    }
}

void* VideoCV::CaptureThread(void* data)
{
    auto videocv = static_cast<VideoCV*>(data);

    int frame = 0;

    pthread_mutex_lock(&mFrameMutex);
    int threadOrder = mThreadIdCurrent;
    mThreadIdCurrent++;

    cv::VideoCapture cap;
    if (!cap.open(videocv->mSource.c_str())) {
        pthread_mutex_unlock(&mFrameMutex);
        return (void*)0;
    }

    videocv->mFrameNum = int(cap.get(cv::CAP_PROP_FRAME_COUNT)) - 2;
    pthread_mutex_unlock(&mFrameMutex);

    while (videocv->mIsRunning) {
        bool readFrame = true;

        int i = videocv->mBufferIndex + 1;
        if (i >= BUFFER_NUM) {
            i = 0;
        }

        pthread_mutex_lock(&mFrameMutex);
        if (threadOrder == 0) // For master thread
        {
            if (mThreadIdCurrent == mThreadId) {
                // All slaves were updated
                if (mFrameCurrent >= videocv->mFrameNum) {
                    // Video file restart after end of the video
                    mFrameCurrent = 0;
                    if (!cap.set(cv::CAP_PROP_POS_FRAMES, 0.0)) {
                        LogError("Video file %s wasn't restarted", videocv->mSource.c_str());
                    }
                } else {
                    mFrameCurrent++;
                }
                mThreadIdCurrent = 1;
            } else {
                readFrame = false;
            }
        } else if ((mThreadIdCurrent != mThreadId) && (frame != mFrameCurrent)) {
            // All slave threads are not updated
            if (mFrameCurrent == 0) {
                // Video file restart after end of the video
                if (!cap.set(cv::CAP_PROP_POS_FRAMES, 0.0)) {
                    LogError("Video file %s wasn't restarted", videocv->mSource.c_str());
                }
            } else {
                if (frame >= videocv->mFrameNum) {
                    // Video file restart after end of the video
                    readFrame = false;
                }
            }
            frame = mFrameCurrent;
            mThreadIdCurrent++;
        } else {
            readFrame = false;
        }
        pthread_mutex_unlock(&mFrameMutex);

        if (readFrame) {
            cap >> videocv->mFrames[i];
            if (!videocv->mFrames[i].empty()) {
                cv::cvtColor(videocv->mFrames[i], videocv->mFrames[i], (int)cv::COLOR_BGR2RGB);
                videocv->mBufferIndex = i;
                videocv->mTextures[i]->OnUpdate();
            }
        }
        usleep(50000);
    }

    if (cap.isOpened()) {
        cap.release();
    }

    return (void*)0;
}

std::shared_ptr<Texture> VideoCV::GetTexture()
{
    if (mBufferIndex > -1) {
        return mTextures[mBufferIndex];
    }
    return nullptr;
}
