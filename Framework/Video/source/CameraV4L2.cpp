/*
 * Copyright 2017, 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "CameraV4L2.hpp"

#include "Log.hpp"

#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#if USE_VIV
#include <TextureBufferVIV.hpp>
#endif

bool CameraV4L2::mIsRunning_Workaround = false;

CameraV4L2::CameraV4L2(std::string source, int width, int height, VideoFormat videoFormat)
    : VideoStream(source, width, height, videoFormat)
    , mMemoryType(V4L2_MEMORY_MMAP) // Supports only V4L2_MEMORY_MMAP
{
    for (int i = 0; i < BUFFER_NUM; i++) {
        memset(&mV4L2Buffers[i], 0, sizeof(mV4L2Buffers[i]));
    }

    mBufferIndex = -1;

    struct v4l2_capability cap;     // Query device capabilities
    struct v4l2_format fmt;         // Data format
    struct v4l2_requestbuffers req; // Parameters of the device buffers
    struct v4l2_streamparm parm;    // Streaming parameters

    if ((mFd = open(mSource.c_str(), O_RDWR, 0)) < 0) {
        LogError("Unable to open %s: %s", mSource.c_str(), strerror(errno));
        return;
    }

    // Identify kernel devices compatible with this specification
    // to obtain information about driver and hardware capabilities
    if (ioctl(mFd, VIDIOC_QUERYCAP, &cap) == -1) {
        LogError("%s is no V4L2 device", mSource.c_str());
        if (close(mFd) != 0) {
            LogWarning("%s was not closed", mSource.c_str());
        }
        return;
    }

    if ((cap.capabilities & (uint)V4L2_CAP_VIDEO_CAPTURE_MPLANE) == 0U) {
        // The device supports the single-planar API through the Video Capture interface
        LogError("%s is no video capture device", mSource.c_str());
        if (close(mFd) != 0) {
            LogWarning("%s was not closed", mSource.c_str());
        }
        return;
    }

    // Set the data format, try a format
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = (uint)V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix.width = (uint)mWidth;
    fmt.fmt.pix.height = (uint)mHeight;
    fmt.fmt.pix.pixelformat = (uint)VideoFormatToV4L2[mVideoFormat];
    fmt.fmt.pix_mp.num_planes = 1;
    if (ioctl(mFd, VIDIOC_S_FMT, &fmt) == -1) {
        // VIDIOC_S_FMT may change width and height
        LogError("%s format 0x%X not supported", mSource.c_str(), fmt.fmt.pix.pixelformat);
        if (close(mFd) != 0) {
            LogWarning("%s was not closed", mSource.c_str());
        }
        return;
    }

    // Get the data format
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = (uint)V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(mFd, VIDIOC_G_FMT, &fmt) == -1) {
        LogError("%s VIDIOC_G_FMT failed", mSource.c_str());
        if (close(mFd) != 0) {
            LogWarning("%s was not closed", mSource.c_str());
        }
        return;
    } else {
        LogInfo("Device = %s", mSource.c_str());
        LogInfo("\tWidth = %u\t Height = %u", fmt.fmt.pix.width, fmt.fmt.pix.height);
        LogInfo("\tImage size = %u", fmt.fmt.pix.sizeimage);
        LogInfo("\tPixelformat %u%u%u%u", (unsigned char)(fmt.fmt.pix.pixelformat & 0xffu),
            (unsigned char)((fmt.fmt.pix.pixelformat >> 8U) & 0xffu),
            (unsigned char)((fmt.fmt.pix.pixelformat >> 16U) & 0xffu),
            (unsigned char)((fmt.fmt.pix.pixelformat >> 24U) & 0xffu));
    }
    mWidth = (int)fmt.fmt.pix.width;
    mHeight = (int)fmt.fmt.pix.height;

    memset(&parm, 0, sizeof(parm));
    parm.type = (uint)V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(mFd, VIDIOC_G_PARM, &parm) == -1) {
        LogError("%s VIDIOC_G_PARM failed", mSource.c_str());
        parm.parm.capture.timeperframe.denominator = 30;
    }

    LogInfo("\tWxH @ fps = %ux%u@%u", fmt.fmt.pix_mp.width, fmt.fmt.pix_mp.height,
        parm.parm.capture.timeperframe.denominator);
    LogInfo("\tImage size = %u", fmt.fmt.pix_mp.plane_fmt[0].sizeimage);

    // Set read parameters to avoid unitialized value in driver
    if (ioctl(mFd, VIDIOC_S_PARM, &parm) == -1) {
        LogError("%s VIDIOC_S_PARM failed", mSource.c_str());
    }

    // Initiate Memory Mapping, User Pointer I/O or DMA buffer I/O
    memset(&req, 0, sizeof(req));
    req.count = BUFFER_NUM; // The number of buffers requested or granted
    req.type = (uint)V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.memory = (uint)mMemoryType;
    if (ioctl(mFd, VIDIOC_REQBUFS, &req) == -1) {
        LogError("%s does not support memory mapping", mSource.c_str());
        if (close(mFd) != 0) {
            LogWarning("%s was not closed", mSource.c_str());
        }
        return;
    }

    if (req.count < 2U) {
        LogError("Insufficient buffer memory on %s", mSource.c_str());
        if (close(mFd) != 0) {
            LogWarning("%s was not closed", mSource.c_str());
        }
        return;
    }
}

CameraV4L2::~CameraV4L2(void)
{
    Stop();
}

bool CameraV4L2::Start(void)
{
    StartStreaming();
    if (mIsRunning) {
        StartThread();
    }

    return mIsRunning;
}

void CameraV4L2::StartStreaming(void)
{
    LogInfo("Start capture device %s", mSource.c_str());
    if (mIsRunning == false) {
        struct v4l2_buffer buf;
        struct v4l2_plane planes = { 0 };
        enum v4l2_buf_type buf_type;
        for (int i = 0; i < BUFFER_NUM; i++) {
            memset(&buf, 0, sizeof(buf));
            memset(&planes, 0, sizeof(planes));
            buf.type = (uint)V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            buf.memory = (uint)mMemoryType;
            buf.m.planes = &planes;
            buf.length = 1;
            buf.index = (uint)i;

            if (mMemoryType == (int)V4L2_MEMORY_USERPTR) {
                buf.length = mBuffers[i].bufferLength;
                buf.m.userptr = (unsigned long)mBuffers[i].offset;
            }

            // Query the status of a buffer at any time after buffers have been allocated with the VIDIOC_REQBUFS ioctl
            if (ioctl(mFd, VIDIOC_QUERYBUF, &buf) == -1) {
                LogError("%s VIDIOC_QUERYBUF error", mSource.c_str());
                return;
            }
            if (mMemoryType == (int)V4L2_MEMORY_MMAP) {
                mBuffers[i].bufferLength = buf.m.planes->length;
                mBuffers[i].offset = (size_t)buf.m.planes->m.mem_offset;
                mBuffers[i].start = (unsigned char*)mmap(NULL, mBuffers[i].bufferLength, PROT_READ, MAP_SHARED, mFd,
                    (off_t)mBuffers[i].offset);
                if (mBuffers[i].start == MAP_FAILED) {
                    LogError("%s mmap failed for buffer %d: %s", mSource.c_str(), i, strerror(errno));
                    mBuffers[i].start = nullptr;
                    // Unmap buffers that were already mapped before this failure
                    for (int j = 0; j < i; j++) {
                        if (mBuffers[j].start != nullptr) {
                            if (munmap(mBuffers[j].start, mBuffers[j].bufferLength) == -1) {
                                LogError("%s munmap failed for buffer %d: %s", mSource.c_str(), j, strerror(errno));
                            }
                            mBuffers[j].start = nullptr;
                        }
                    }
                    mTextures.clear();
                    return;
                }
                // memset(mBuffers[i].start, 0xFF, mBuffers[i].bufferLength);
            }

            mBuffers[i].width = mWidth;
            mBuffers[i].height = mHeight;
            mBuffers[i].format = mVideoFormat;

#if USE_VIV
            mTextures.push_back(std::make_shared<TextureBufferVIV>(mBuffers[i]));
#else
            mTextures.push_back(std::make_shared<TextureBuffer>(mBuffers[i]));
#endif
        }

        for (int i = 0; i < BUFFER_NUM; i++) {
            memset(&buf, 0, sizeof(buf));
            memset(&planes, 0, sizeof(planes));
            buf.type = (uint)V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            buf.memory = (uint)mMemoryType;
            buf.m.planes = &planes;
            buf.index = (uint)i;
            buf.m.planes->length = mBuffers[i].bufferLength;
            buf.length = 1;
            if (mMemoryType == (int)V4L2_MEMORY_USERPTR) {
                buf.m.planes->m.userptr = (unsigned long)mBuffers[i].start;
            } else {
                buf.m.planes->m.mem_offset = (unsigned int)mBuffers[i].offset;
            }

            // Enqueue an empty buffer in the driver's incoming queue
            if (ioctl(mFd, VIDIOC_QBUF, &buf) == -1) {
                LogError("%s VIDIOC_QBUF error", mSource.c_str());
                return;
            }
        }

        // Start streaming I/O
        buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        if (ioctl(mFd, VIDIOC_STREAMON, &buf_type) == -1) {
            LogError("%s VIDIOC_STREAMON error", mSource.c_str());
            return;
        }
        mIsRunning = true;
        mIsRunning_Workaround = true;
    }
}

void CameraV4L2::Stop(void)
{
    LogInfo("Stop capture device %s", mSource.c_str());

    // Stop all camera capture threads before stop any capture stream with VIDIOC_STREAMOFF
    // And wait for threads to terminates
    if (mIsRunning_Workaround) {
        mIsRunning_Workaround = false;
        sleep(1);
    }

    if (mIsRunning) {
        mIsRunning = false;
        void* status = 0;
        if (mCaptureThread != 0U) {
            pthread_join(mCaptureThread, &status);
            if (status != 0) {
                LogError("Pthread join %s failed", mSource.c_str());
            }
            mCaptureThread = 0;
        }

        // Stop streaming
        enum v4l2_buf_type buf_type;
        buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        if (ioctl(mFd, VIDIOC_STREAMOFF, &buf_type) == -1) {
            LogError("%s VIDIOC_STREAMOFF error", mSource.c_str());
        }

        if (mFd >= 0) {
            for (int i = 0; i < BUFFER_NUM; i++) {
                if (-1 == munmap(mBuffers[i].start, mBuffers[i].bufferLength)) {
                    LogError("munmap failed");
                }
            }
            // Release GL textures mapped to video buffers
            mTextures.clear();
            // Force GPU driver to schedule surface deallocation before closing the stream
            glFinish();

            if (close(mFd) != 0) {
                LogError("%s was not closed", mSource.c_str());
            }
        }
    }
}

void* CameraV4L2::CaptureThread(void* data)
{
    auto camerav4l2 = static_cast<CameraV4L2*>(data);

    struct v4l2_plane planes = { 0 };
    int i = 0;

    // while (camerav4l2->mIsRunning)
    while (mIsRunning_Workaround) {
        if (camerav4l2->mBufferIndex != -1) {
            i = camerav4l2->mBufferIndex + 1;
            if (i == BUFFER_NUM) {
                i = 0;
            }
        } else {
            i = 0;
        }

        memset((void*)&(camerav4l2->mV4L2Buffers[i]), 0, sizeof(mV4L2Buffers[i]));
        memset(&planes, 0, sizeof(planes));
        camerav4l2->mV4L2Buffers[i].type = (uint)V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        camerav4l2->mV4L2Buffers[i].memory = (uint)V4L2_MEMORY_MMAP;
        camerav4l2->mV4L2Buffers[i].m.planes = &planes;
        camerav4l2->mV4L2Buffers[i].length = 1;

        if (ioctl(camerav4l2->mFd, VIDIOC_DQBUF, &camerav4l2->mV4L2Buffers[i]) == -1) {
            LogError("VIDIOC_DQBUF failed for %s", camerav4l2->mSource.c_str());
            continue;
        }
        camerav4l2->mBufferIndex = i;
        camerav4l2->mTextures[i]->OnUpdate();

        if (ioctl(camerav4l2->mFd, VIDIOC_QBUF, &camerav4l2->mV4L2Buffers[i]) == -1) {
            LogError("VIDIOC_QBUF failed for %s", camerav4l2->mSource.c_str());
            continue;
        }
    }
    pthread_exit((void*)0);
}

void CameraV4L2::StartThread(void)
{
    if (pthread_create(&mCaptureThread, NULL, CameraV4L2::CaptureThread, this) != 0) {
        LogError("Cannot create capture thread");
    }
}

std::shared_ptr<Texture> CameraV4L2::GetTexture()
{
    if (mBufferIndex > -1) {
        return mTextures[mBufferIndex];
    }
    return nullptr;
}
