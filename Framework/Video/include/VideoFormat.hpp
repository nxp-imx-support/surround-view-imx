/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <map>

enum class VideoFormat
{
    RGB,
    YUYV,
};

#include <linux/videodev2.h>
static std::map<VideoFormat, __u32> VideoFormatToV4L2 = {
    { VideoFormat::RGB, V4L2_PIX_FMT_RGB24 },
    { VideoFormat::YUYV, V4L2_PIX_FMT_YUYV }
};

#include "Graphics.hpp"
static std::map<VideoFormat, GLenum> VideoFormatToGL = {
    { VideoFormat::RGB, GL_RGB },
#if defined GL_VIV_YUY2
    { VideoFormat::YUYV, GL_VIV_YUY2 }
#endif
};

#include <string>
static std::map<std::string, VideoFormat> StringToVideoFormat = {
    { "RGB", VideoFormat::RGB },
    { "YUYV", VideoFormat::YUYV }
};

#include <opencv2/imgproc/types_c.h>
static std::map<VideoFormat, __u32> VideoFormatToCVcvt2RGB = {
    { VideoFormat::YUYV, CV_YUV2RGB_YUYV }
};
static std::map<VideoFormat, __u32> VideoFormatToCVtype = {
    { VideoFormat::RGB, CV_8UC3 }, { VideoFormat::YUYV, CV_8UC2 }
};
