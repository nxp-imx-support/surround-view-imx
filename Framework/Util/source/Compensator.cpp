/*
 * Copyright 2017, 2022 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Compensator.hpp"

#include "AssetManager.hpp"
#include "FilesName.hpp"
#include "Log.hpp"
#include "Macros.hpp"

#include <fstream>
#include <iostream>
#include <sys/stat.h>

Compensator::Compensator(cv::Size maskSize)
{
    mMask = cv::Mat(maskSize, CV_8UC1, cv::Scalar(0));
}

cv::Rect Compensator::GetFlipROI(uint index)
{
    if (index < mROI.size()) {
        float x1 = (mROI[index].x + 1.0f) * (float)mMask.cols / 2.0f;
        float y1 = (-mROI[index].y + 1.0f) * (float)mMask.rows / 2.0f;
        float x2 = (mROI[index].x + mROI[index].width + 1.0f) * (float)mMask.cols / 2.0f;
        float y2 = (-mROI[index].y - mROI[index].height + 1.0f) * (float)mMask.rows / 2.0f;
        return cv::Rect(cv::Point((int)x1, (int)y1), cv::Point((int)x2, (int)y2));
    } else {
        return cv::Rect(0, 0, 0, 0);
    }
}

void Compensator::Feed(uint camerasCount, std::vector<std::vector<cv::Point3f>>& seamPoints)
{
    for (uint i = 0; i < camerasCount; i++) {
        if (seamPoints[i].size() < 8) {
            LogError("Exposure compensator was not generated. Seam %u doesn't contain 8 points", i);
            return;
        }
    }

    mRadius = sqrt(pow(seamPoints[0][1].x, 2) + pow(seamPoints[0][1].y, 2));

    double height = (double)mMask.rows / (double)mRadius / 2.0;
    double xGain = 2.0 * mRadius;
    double yGain = 2.0 * mRadius * (double)mMask.rows / (double)mMask.cols;

    for (size_t i = 0; i < camerasCount; i += 2U) {
        std::vector<cv::Point> pLeft, pRight;
        double sg = (double)pow(-1.0f, (int)(i >> 1U));
        int iNext = next_id((int)i, (int)camerasCount - 1);     // Index of the previous camera
        int iPrev = previous_id((int)i, (int)camerasCount - 1); // Index of the next camera

        pLeft.push_back(cv::Point2f((float)((sg * (double)seamPoints[i][0].x + mRadius) * height),
            (float)((sg * (double)seamPoints[i][0].y + mRadius) * height)));
        pLeft.push_back(cv::Point2f((float)((sg * (double)seamPoints[i][1].x + mRadius) * height),
            (float)((sg * (double)seamPoints[i][1].y + mRadius) * height)));
        pLeft.push_back(cv::Point2f((float)((sg * (double)seamPoints[iPrev][6].y + mRadius) * height),
            (float)((-sg * (double)seamPoints[iPrev][6].x + mRadius) * height)));
        pLeft.push_back(cv::Point2f((float)((sg * (double)seamPoints[iPrev][7].y + mRadius) * height),
            (float)((-sg * (double)seamPoints[iPrev][7].x + mRadius) * height)));

        pRight.push_back(cv::Point2f((float)((-sg * seamPoints[iNext][0].y + mRadius) * height),
            (float)((sg * seamPoints[iNext][0].x + mRadius) * height)));
        pRight.push_back(cv::Point2f((float)((-sg * seamPoints[iNext][1].y + mRadius) * height),
            (float)((sg * seamPoints[iNext][1].x + mRadius) * height)));
        pRight.push_back(cv::Point2f((float)((sg * seamPoints[i][6].x + mRadius) * height),
            (float)((sg * seamPoints[i][6].y + mRadius) * height)));
        pRight.push_back(cv::Point2f((float)((sg * seamPoints[i][7].x + mRadius) * height),
            (float)((sg * seamPoints[i][7].y + mRadius) * height)));

        mROI.push_back(cv::Rect2f(cv::Point2f((float)(sg * (double)seamPoints[i][1].x / xGain),
                                      (float)(-sg * (double)seamPoints[iPrev][6].x / yGain)),
            cv::Point2f((float)(sg * (double)seamPoints[iPrev][7].y / xGain),
                (float)(sg * (double)seamPoints[i][0].y / yGain))));
        mROI.push_back(cv::Rect2f(cv::Point2f((float)(-sg * (double)seamPoints[iNext][0].y / xGain),
                                      (float)(sg * (double)seamPoints[i][7].y / yGain)),
            cv::Point2f((float)(sg * (double)seamPoints[i][6].x / xGain),
                (float)(sg * (double)seamPoints[iNext][1].x / yGain))));

        // Draws a filled convex polygon using all seam points
        cv::fillConvexPoly(mMask, pLeft, cv::Scalar(255));
        cv::fillConvexPoly(mMask, pRight, cv::Scalar(255));
    }
}

int Compensator::Save(const char* path)
{
    double xGain = 2.0 * mRadius;
    double yGain = 2.0 * mRadius * (double)mMask.rows / (double)mMask.cols;
    double height = (double)mMask.rows / (double)mRadius / 2.0;

    if (AssetManager::PathExists(std::string(path)) == false) {
        LogError("Directory %s does not exist", path);
        return (-1);
    }

    for (uint i = 0; i < mROI.size(); i++) {
        std::string fileName = AssetManager::GetPath(VERTICES_DIR ARRAY_PREFIX + std::to_string(i + 1U));
        std::string fileNameROI = AssetManager::GetPath(std::string(path) + ARRAY_PREFIX + std::to_string(i + 1U));

        std::ifstream ifsRef(fileName.c_str());
        if (ifsRef) {
            std::ofstream outC;
            outC.open(fileNameROI.c_str(), std::ofstream::out | std::ofstream::trunc);

            float vx[3], vy[3], vz[3], tx[3], ty[3];
            while (ifsRef >> vx[0] >> vy[0] >> vz[0] >> tx[0] >> ty[0] >> vx[1] >> vy[1] >> vz[1] >> tx[1] >> ty[1] >> vx[2] >> vy[2] >> vz[2] >> tx[2] >> ty[2]) // Read triangle
            {
                if ((vz[0] == 0.0f) && (vz[1] == 0.0f) && (vz[2] == 0.0f)) {
                    double x0 = (vx[0] + mRadius) * height;
                    double y0 = (-vy[0] + mRadius) * height;

                    double x1 = (vx[1] + mRadius) * height;
                    double y1 = (-vy[1] + mRadius) * height;

                    double x2 = (vx[2] + mRadius) * height;
                    double y2 = (-vy[2] + mRadius) * height;

                    if ((x0 >= (double)0.0) && (x0 < (double)mMask.cols) && (y0 >= (double)0.0) && (y0 < (double)mMask.rows) && (x1 >= (double)0.0) && (x1 < (double)mMask.cols) && (y1 >= (double)0.0) && (y1 < (double)mMask.rows) && (x2 >= (double)0.0) && (x2 < (double)mMask.cols) && (y2 >= (double)0.0) && (y2 < (double)mMask.rows)) {
                        uint verticesSum = (uint)mMask.at<uchar>(cv::Point(x0, y0)) + (uint)mMask.at<uchar>(cv::Point(x1, y1)) + (uint)mMask.at<uchar>(cv::Point(x2, y2));
                        if (verticesSum == 765U) { // 3 * 255
                            outC << vx[0] / xGain << " " << vy[0] / yGain << " " << vz[0] << " " << tx[0] << " "
                                 << ty[0] << std::endl;
                            outC << vx[1] / xGain << " " << vy[1] / yGain << " " << vz[1] << " " << tx[1] << " "
                                 << ty[1] << std::endl;
                            outC << vx[2] / xGain << " " << vy[2] / yGain << " " << vz[2] << " " << tx[2] << " "
                                 << ty[2] << std::endl;
                        }
                    }
                }
            }
            outC.close();
        } else {
            LogError("Compensator grids have not been saved. File %s not found", fileName.c_str());
            return (-1);
        }
    }

    std::string fileName = AssetManager::GetPath(std::string(path) + COMPENSATOR_FILE);
    std::ofstream outC;
    outC.open(fileName.c_str(), std::ofstream::out | std::ofstream::trunc);
    for (uint i = 0; i < mROI.size(); i++) {
        outC << mROI[i].x << " " << mROI[i].y << " ";
        outC << mROI[i].x + mROI[i].width << " " << mROI[i].y + mROI[i].height << std::endl;
    }
    outC.close();

    return (0);
}

int Compensator::Load(const char* path)
{
    std::string fileName = AssetManager::GetPath(std::string(path) + COMPENSATOR_FILE);
    std::ifstream ifsRef(fileName.c_str());
    if (ifsRef) // The file exists, and is open for input
    {
        mROI.clear();
        float p1, p2, p3, p4;
        while (ifsRef >> p1 >> p2 >> p3 >> p4) // Read triangle
        {
            mROI.push_back(cv::Rect2f(cv::Point2f(p1, p2), cv::Point2f(p3, p4)));

            std::vector<cv::Point> p;
            p.push_back(cv::Point2f((p1 + 1.0f) * (float)mMask.cols / 2.0f,
                (p2 + 1.0f) * (float)mMask.rows / 2.0f));
            p.push_back(cv::Point2f((p3 + 1.0f) * (float)mMask.cols / 2.0f,
                (p2 + 1.0f) * (float)mMask.rows / 2.0f));
            p.push_back(cv::Point2f((p3 + 1.0f) * (float)mMask.cols / 2.0f,
                (p4 + 1.0f) * (float)mMask.rows / 2.0f));
            p.push_back(cv::Point2f((p1 + 1.0f) * (float)mMask.cols / 2.0f,
                (p4 + 1.0f) * (float)mMask.rows / 2.0f));
            cv::fillConvexPoly(mMask, p, cv::Scalar(255)); // Draws a filled convex polygon using all seam points
        }
    } else {
        LogError("Compensator has not been loaded. File %s not found", fileName.c_str());
        return (-1);
    }
    return (0);
}
