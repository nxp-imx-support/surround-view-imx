/*
 * Copyright 2017, 2022, 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Gains.hpp"
#include "FilesName.hpp"
#include "Log.hpp"
#include "Macros.hpp"
#include "Time.hpp"

Gains::Gains(int width, int height)
{
    for (int i = 0; i < CAMERAS_NUM; i++) {
        for (int j = 0; j < CHANELS_NUM; j++) {
            mGain[i][j] = 1.0f;
        }
    }

    // Load Compensator
    mCompensator = std::make_unique<Compensator>(cv::Size(width, height));
    mCompensator->Load((const char*)COMPENSATOR_DIR);

    for (int j = 0; j < CAMERAS_NUM; j++) {
        mOverlapROI[j][0] =
            cv::Mat(mCompensator->GetFlipROI((uint)j).height, mCompensator->GetFlipROI((uint)j).width, CV_8UC(4));
        uint next = (uint)next_id(j, CAMERAS_NUM - 1);
        mOverlapROI[j][1] =
            cv::Mat(mCompensator->GetFlipROI(next).height, mCompensator->GetFlipROI(next).width, CV_8UC(4));
    }

    // Start thread
    mThreadRunning.store(true, std::memory_order_release);
    mTriggerMutex.lock();
    mThread = std::thread(&Gains::Update, this);
}

Gains::~Gains(void)
{
    mThreadRunning.store(false, std::memory_order_release);
    mTriggerMutex.unlock();
    Unlock();
    if (mThread.joinable()) {
        mThread.join();
    }
}

cv::Mat& Gains::GetOverlapROI(int cameraId, int overlapId)
{
    return mOverlapROI[cameraId][overlapId];
}

float* Gains::GetGain(int cameraId)
{
    return mGain[cameraId];
}

cv::Rect Gains::GetFlipROI(int cameraId)
{
    return mCompensator->GetFlipROI(cameraId);
}

bool Gains::TryLock()
{
    return mGainMutex.try_lock();
}

void Gains::Unlock()
{
    mGainMutex.unlock();
}

void Gains::TriggerUpdate()
{
    mTriggerMutex.unlock();
}

void Gains::Update()
{
    double gamma = 2.2;
    double gammaInv = 1.0 / gamma;

    while (mThreadRunning.load(std::memory_order_acquire)) {
        // Wait on Exposure Correction call
        mTriggerMutex.lock();

        if (mThreadRunning.load(std::memory_order_acquire)) {
            // Lock gain
            mGainMutex.lock();

            Time start = Time::Get();

            cv::Scalar accLeft[CAMERAS_NUM], accRight[CAMERAS_NUM];

            for (uint camera = 0U; camera < (uint)CAMERAS_NUM; ++camera) {
                cv::Scalar ciLeft(0, 0, 0), ciRight(0, 0, 0);

                for (int col = 0; col < mOverlapROI[camera][0].cols; col++) {
                    for (int row = 0; row < mOverlapROI[camera][0].rows; row++) {
                        // ciLeft += images(col, row) ^ gamma * mask(col, row)
                        cv::Scalar imgPow = (cv::Scalar)mOverlapROI[camera][0].at<cv::Vec4b>(cv::Point(col, row));
                        cv::pow(imgPow, gamma, imgPow);
                        cv::add(imgPow, ciLeft, ciLeft);
                    }
                }
                accLeft[camera] = ciLeft;

                for (int col = 0; col < mOverlapROI[camera][1].cols; col++) {
                    for (int row = 0; row < mOverlapROI[camera][1].rows; row++) {
                        // ciRight += images(col, row) ^ gamma * mask(col, row)
                        cv::Scalar imgPow = (cv::Scalar)mOverlapROI[camera][1].at<cv::Vec4b>(cv::Point(col, row));
                        cv::pow(imgPow, gamma, imgPow);
                        cv::add(imgPow, ciRight, ciRight);
                    }
                }
                accRight[camera] = ciRight;
            }

            cv::Scalar a[CAMERAS_NUM];  // Color correction coefficient
            a[0] = cv::Scalar(1, 1, 1); // For the first image the color correction coefficient is 1 for all channels
            for (uint i = 1U; i < (uint)CAMERAS_NUM; ++i) {
                cv::divide(accRight[i - 1U], accLeft[i], a[i]);
            }

            double ar = 0.0, ag = 0.0, ab = 0.0, ar2 = 0.0, ag2 = 0.0, ab2 = 0.0;
            for (uint i = 0U; i < (uint)CAMERAS_NUM; ++i) {
                ar += a[i].val[0];                // Color correction coefficients for channel R
                ag += a[i].val[1];                // Color correction coefficients for channel G
                ab += a[i].val[2];                // Color correction coefficients for channel B
                ar2 += a[i].val[0] * a[i].val[0]; // ar^2
                ag2 += a[i].val[1] * a[i].val[1]; // ag^2
                ab2 += a[i].val[2] * a[i].val[2]; // ab^2
            }

            cv::Scalar g(ar / ar2, ag / ag2, ab / ab2); // Global compensation coefficient
            for (uint i = 0U; i < (uint)CAMERAS_NUM; ++i) {
                cv::multiply(g, a[i], a[i]);
                cv::pow(a[i], gammaInv, a[i]);
            }

            for (uint camera = 0U; camera < (uint)CAMERAS_NUM; ++camera) {
                for (int color = 0; color < 3; color++) {
                    mGain[camera][color] = (float)a[camera][2 - color];
                }
            }

            Time end = Time::Get();
            LogDebug("Gain computation time: %f ms", (end - start).GetMs());

            mGainMutex.unlock();
        }
    }
}
