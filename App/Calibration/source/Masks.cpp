/*
 * Copyright 2017, 2022, 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Masks.hpp"

#include "AssetManager.hpp"
#include "FilesName.hpp"
#include "Log.hpp"
#include "Macros.hpp"

#define SEAM_STEP 0.001

int Masks::GetSeam(Seam* left, Seam* right, uint i)
{
    if ((i < mSeamLeft.size()) && (i < mSeamRight.size())) {
        memcpy(left, &mSeamLeft[i], sizeof(Seam));
        memcpy(right, &mSeamRight[i], sizeof(Seam));
        return (1);
    }
    return (0);
}

void Masks::CreateMasks(std::vector<std::shared_ptr<Camera>>& cameras, std::vector<std::vector<cv::Point3f>>& seamPoints,
    float smoothing)
{
    for (uint i = 0; i < cameras.size(); i++) {
        if (seamPoints[i].size() < 8) {
            LogError("Masks was not generated. Seam %u doesn't contain 8 points", i);
            return;
        }
    }

    // Get grids intersection points
    for (uint i = 0; i < cameras.size(); i++) {
        Seam seamLeft, seamRight;
        // intersection with next grid
        GetSeamPoints(seamPoints[i], seamPoints[next_id((int)i, (int)cameras.size() - 1)], seamRight, 1);
        mSeamRight.push_back(seamRight);
        // intersection with previous grid
        GetSeamPoints(seamPoints[i], seamPoints[previous_id((int)i, (int)cameras.size() - 1)], seamLeft, -1);
        mSeamLeft.push_back(seamLeft);
    }

    // Calculate seams
    for (uint i = 0; i < cameras.size(); i++) {
        auto model = cameras[i]->GetModel();
        std::vector<cv::Point3f> seam3d;
        double radius = sqrt(pow(seamPoints[i][1].x, 2) + pow(seamPoints[i][1].y, 2));
        // Left edge
        mSeamLeft[i].p2.x = static_cast<float>(
            (-mSeamLeft[i].line.alpha * mSeamLeft[i].line.beta - sqrt(pow(radius, 2) * (1.0 + pow(mSeamLeft[i].line.alpha, 2)) - pow(mSeamLeft[i].line.beta, 2))) / (1.0 + pow(mSeamLeft[i].line.alpha, 2)));
        mSeamLeft[i].p2.y = (float)sqrt(pow(radius, 2) - pow(mSeamLeft[i].p2.x, 2));
        double cos_l = mSeamLeft[i].p2.x / radius;
        // Right edge
        mSeamRight[i].p2.x = static_cast<float>(
            (-mSeamRight[i].line.alpha * mSeamRight[i].line.beta + sqrt(pow(radius, 2) * (1.0 + pow(mSeamRight[i].line.alpha, 2)) - pow(mSeamRight[i].line.beta, 2))) / (1.0 + pow(mSeamRight[i].line.alpha, 2)));
        mSeamRight[i].p2.y = (float)sqrt(pow(radius, 2) - pow(mSeamRight[i].p2.x, 2));
        double cos_r = mSeamRight[i].p2.x / radius;

        // Left horizontal seam
        for (double xx = mSeamLeft[i].p1.x; xx >= mSeamLeft[i].p2.x; xx -= SEAM_STEP) {
            seam3d.push_back(cv::Point3f(xx, xx * mSeamLeft[i].line.alpha + mSeamLeft[i].line.beta, 0.0));
        }

        // Left vertical seam
        for (double zz = 0.0; zz < abs(seamPoints[i][3].z); zz += SEAM_STEP) {
            double newX = (radius + sqrt(zz)) * cos_l;
            double newY = -newX * tan(acos(cos_l));
            seam3d.push_back(cv::Point3f(newX, newY, -zz));
        }

        // Top seam
        for (double aa = cos_l; aa < cos_r; aa += SEAM_STEP) {
            double newX = (radius + sqrt(abs(seamPoints[i][3].z))) * aa;
            double newY = -newX * tan(acos(aa));
            seam3d.push_back(cv::Point3f(newX, newY, seamPoints[i][3].z));
        }

        // Right vertical seam
        for (double zz = abs(seamPoints[i][4].z); zz > 0.0; zz -= SEAM_STEP) {
            double newX = (radius + sqrt(zz)) * cos_r;
            double newY = -newX * tan(acos(cos_r));
            seam3d.push_back(cv::Point3f(newX, newY, -zz));
        }

        // Right horizontal seam
        for (double xx = mSeamRight[i].p2.x; xx >= mSeamRight[i].p1.x; xx -= SEAM_STEP) {
            seam3d.push_back(cv::Point3f(xx, xx * mSeamRight[i].line.alpha + mSeamRight[i].line.beta, 0.0));
        }

        // Bottom seam
        Line bottom = GetAlphaBeta(mSeamRight[i].p1, mSeamLeft[i].p1);
        for (double xx = mSeamRight[i].p1.x; xx > mSeamLeft[i].p1.x; xx -= SEAM_STEP) {
            seam3d.push_back(cv::Point3f(xx, bottom.alpha * xx + bottom.beta, 0));
        }

        // Projects 3D seam points to an image plane
        std::vector<cv::Point2f> seam2d;
        cv::projectPoints(seam3d, cameras[i]->GetRvec(), cameras[i]->GetTvec(), cameras[i]->GetK(), cameras[i]->GetDistCoeffs(), seam2d);

        // Get seam for fisheye image
        std::vector<cv::Point> seam;
        for (uint j = 0; j < seam2d.size(); j++) {
            if (model->MapHas(seam2d[j])) {
                seam.push_back(cv::Point2f(model->XMapAt(seam2d[j]), model->YMapAt(seam2d[j])));
            }
        }
        seam.push_back(seam[0]);

        // Create masks which are limited to seams.
        cv::Mat camMask(model->Size(), CV_8UC1, cv::Scalar(0));
        mMasks.push_back(camMask);
        cv::fillConvexPoly(mMasks[i], seam, cv::Scalar(255)); // Draws a filled convex polygon using all seam points

        // Define left edge of smoothing
        if ((smoothing > 0.5f) || (smoothing < 0.0f)) {
            smoothing = 0.5f;
        }
        double smoothingAngle = atan(mSeamLeft[i].line.alpha);
        std::vector<cv::Point3f> left_points;
        left_points.push_back(cv::Point3f(mSeamLeft[i].p1.x, mSeamLeft[i].p1.y, 0));
        left_points.push_back(cv::Point3f(mSeamLeft[i].p2.x, sqrt(pow(radius, 2) - pow(mSeamLeft[i].p2.x, 2)), 0));
        left_points.push_back(seamPoints[i][3]);
        SmoothMaskEdge(mMasks[i], cv::Vec2b(0, 255), cv::Vec2d(smoothingAngle - smoothing, smoothingAngle + smoothing),
            0.0039, left_points, cameras[i]);

        // Define right edge of smoothing
        smoothingAngle = atan(mSeamRight[i].line.alpha);
        std::vector<cv::Point3f> right_points;
        right_points.push_back(cv::Point3f(mSeamRight[i].p1.x, mSeamRight[i].p1.y, 0));
        right_points.push_back(cv::Point3f(mSeamRight[i].p2.x, sqrt(pow(radius, 2) - pow(mSeamRight[i].p2.x, 2)), 0));
        right_points.push_back(seamPoints[i][4]);
        SmoothMaskEdge(mMasks[i], cv::Vec2b(255, 0), cv::Vec2d(smoothingAngle - smoothing, smoothingAngle + smoothing),
            0.0039, right_points, cameras[i]);

        // Blurs an image using a Gaussian filter
        for (int row = 0; row < mMasks[i].rows; row++) {
            cv::Mat roi = camMask(cv::Rect(0, row, mMasks[i].cols, 1));
            GaussianBlur(roi, roi, cv::Size(2 * ((mMasks[i].rows - row) / 2) + 1, 1), 10.0, 1.0);
        }
        GaussianBlur(mMasks[i], mMasks[i], cv::Size(3, 3), 10.0);

        // Save masks
        AssetManager::MakeDirectory(GENERATED_DIR);
        AssetManager::MakeDirectory(MASKS_DIR);
        std::string mask_name = MASKS_DIR MASK_PREFIX + std::to_string(i) + ".jpg";
        std::string filePath = AssetManager::GetPath(mask_name);
        cv::Mat outMask;
        cv::resize(mMasks[i], outMask, cv::Size(), 0.5f, 0.5f, cv::INTER_CUBIC);
        if (cv::imwrite(filePath.c_str(), outMask) == false) {
            LogError("Failed to save %s", filePath.c_str());
        }
    }
}

int Masks::SplitGrids(void)
{
    float vx[3], vy[3], vz[3], tx[3], ty[3];

    for (uint i = 0; i < mMasks.size(); i++) {
        int arrayi = i + 1U;
        // Input grid
        std::string fileName = AssetManager::GetPath(VERTICES_DIR ARRAY_PREFIX + std::to_string(arrayi));
        // Overlap grid
        std::string fileName1 = AssetManager::GetPath(VERTICES_DIR ARRAY_PREFIX + std::to_string(arrayi) + "1");
        // Non-Overlap grid
        std::string fileName2 = AssetManager::GetPath(VERTICES_DIR ARRAY_PREFIX + std::to_string(arrayi) + "2");

        std::ifstream ifs_ref(fileName.c_str());
        if (ifs_ref) {
            std::ofstream outC1, outC2;
            outC1.open(fileName1.c_str(), std::ofstream::out | std::ofstream::trunc);
            outC2.open(fileName2.c_str(), std::ofstream::out | std::ofstream::trunc);

            while (ifs_ref >> vx[0] >> vy[0] >> vz[0] >> tx[0] >> ty[0] >> vx[1] >> vy[1] >> vz[1] >> tx[1] >> ty[1] >> vx[2] >> vy[2] >> vz[2] >> tx[2] >> ty[2]) {
                uint pixels_sum = 0U; // Sum of pixels of triangle vertices
                // For each vertex of the triangle
                for (int j = 0; j < 3; j++) {
                    cv::Point idx1 = cv::Point(static_cast<int>(tx[j] * (float)mMasks[i].cols) - 40,
                        static_cast<int>(ty[j] * (float)mMasks[i].rows) - 40);
                    cv::Point idx2 = cv::Point(static_cast<int>(tx[j] * (float)mMasks[i].cols) + 40,
                        static_cast<int>(ty[j] * (float)mMasks[i].rows) + 40);

                    if ((idx1.x < mMasks[i].cols) && (idx1.y < mMasks[i].rows) && (idx1.x > 0) && (idx1.y > 0)) {
                        pixels_sum += mMasks[i].at<uchar>(idx1); // Add pixel value
                        if ((idx2.x < mMasks[i].cols) && (idx2.y < mMasks[i].rows) && (idx2.x > 0) && (idx2.y > 0)) {
                            pixels_sum += mMasks[i].at<uchar>(cv::Point(idx2.x, idx2.y)); // Add pixel value
                            pixels_sum += mMasks[i].at<uchar>(cv::Point(idx1.x, idx2.y)); // Add pixel value
                            pixels_sum += mMasks[i].at<uchar>(cv::Point(idx2.x, idx1.y)); // Add pixel value
                        }
                    } else {
                        if ((idx2.x < mMasks[i].cols) && (idx2.y < mMasks[i].rows) && (idx2.x > 0) && (idx2.y > 0)) {
                            pixels_sum += mMasks[i].at<uchar>(idx2); // Add pixel value
                        }
                    }
                }

                if (pixels_sum == 4U * 765U) {
                    // 3 vertices of triangles are white (3 * 255 = 765) -> non-overlap region
                    outC2 << vx[0] << " " << vy[0] << " " << vz[0] << " " << tx[0] << " " << ty[0] << std::endl;
                    outC2 << vx[1] << " " << vy[1] << " " << vz[1] << " " << tx[1] << " " << ty[1] << std::endl;
                    outC2 << vx[2] << " " << vy[2] << " " << vz[2] << " " << tx[2] << " " << ty[2] << std::endl;
                } else {
                    if (pixels_sum != 0U) {
                        // Overlap region
                        outC1 << vx[0] << " " << vy[0] << " " << vz[0] << " " << tx[0] << " " << ty[0] << std::endl;
                        outC1 << vx[1] << " " << vy[1] << " " << vz[1] << " " << tx[1] << " " << ty[1] << std::endl;
                        outC1 << vx[2] << " " << vy[2] << " " << vz[2] << " " << tx[2] << " " << ty[2] << std::endl;
                    }
                }
            }
            outC1.close();
            outC2.close();
        } else {
            LogError("Texels/vertices grids have not been split. File %s not found", fileName.c_str());
            return (-1);
        }
    }
    return (0);
}

void Masks::SmoothMaskEdge(cv::Mat& img, cv::Vec2b colors, cv::Vec2d angles, double angleStep,
    std::vector<cv::Point3f> edgePoints, std::shared_ptr<Camera> camera)
{
    auto model = camera->GetModel();
    double color = (double)colors[0];
    double colorStep = static_cast<double>(colors[1] - colors[0]) * angleStep / (angles[1] - angles[0]);
    double radius = sqrt(pow(edgePoints[1].x, 2) + pow(edgePoints[1].y, 2));

    double xStart, xEnd, angleCos;

    for (double ang = angles[0]; ang < angles[1]; ang += angleStep) {
        double alpha = tan(ang);
        double beta = edgePoints[0].y - alpha * edgePoints[0].x;

        if (alpha > 0.0) {
            xStart = (-alpha * beta - sqrt(pow(radius, 2) * (1 + pow(alpha, 2)) - pow(beta, 2))) / (1 + pow(alpha, 2));
            xEnd = edgePoints[0].x;
            angleCos = xStart / radius;
        } else {
            xStart = edgePoints[0].x;
            xEnd = (-alpha * beta + sqrt(pow(radius, 2) * (1 + pow(alpha, 2)) - pow(beta, 2))) / (1 + pow(alpha, 2));
            angleCos = xEnd / radius;
        }

        std::vector<cv::Point3f> seam3d;
        std::vector<cv::Point2f> seam2d, seam;

        // Horizontal part of seam
        for (double xx = xStart; xx <= xEnd; xx += SEAM_STEP) {
            seam3d.push_back(cv::Point3f(xx, xx * alpha + beta, 0.0));
        }

        // Vertical part of seam
        for (double zz = 0.0; zz < abs(edgePoints[2].z); zz += SEAM_STEP) {
            double newX = (radius + sqrt(zz)) * angleCos;
            double newY = -newX * tan(acos(angleCos));
            seam3d.push_back(cv::Point3f(newX, newY, -zz));
        }

        cv::projectPoints(seam3d, camera->GetRvec(), camera->GetTvec(), camera->GetK(), camera->GetDistCoeffs(), seam2d);

        for (uint j = 0; j < seam2d.size(); j++) {
            if (model->MapHas(seam2d[j])) {
                seam.push_back(cv::Point2f(model->XMapAt(seam2d[j]), model->YMapAt(seam2d[j])));
                circle(img, seam[j], 1, color, 1);
            }
        }
        color += colorStep;
    }
}

void Masks::GetSeamPoints(std::vector<cv::Point3f>& polygon1, std::vector<cv::Point3f>& polygon2, Seam& seam, int rotation)
{
    bool intersection = false;

    // Search for the first intersection point of input polygons
    int pnt_1 = (int)polygon1.size() - 2; // 7th seam point
    // pnt_1 can be only 6 and 7 (7th and 8th polygon points)
    while ((pnt_1 < (int)polygon1.size()) && (!intersection)) {
        int pnt_2 = 1;
        // pnt_2 can be only 1 and 0 (1st and 2nd polygon points)
        while ((pnt_2 >= 0) && (!intersection)) {
            cv::Point2f p1_1, p1_2, p2_1, p2_2;
            if (rotation < 0) {
                // Rotate polygon2 points to the left
                // 1st polygon, side 2-1 or 1-8
                p1_1 = cv::Point2f(polygon1[pnt_2].x, polygon1[pnt_2].y);
                p1_2 = cv::Point2f(polygon1[previous_id(pnt_2, (int)polygon2.size() - 1)].x,
                    polygon1[previous_id(pnt_2, (int)polygon2.size() - 1)].y);
                // 2nd polygon, side 7-8 or 8-1
                p2_1 = cv::Point2f(polygon2[pnt_1].y, -polygon2[pnt_1].x);
                p2_2 = cv::Point2f(polygon2[next_id(pnt_1, (int)polygon2.size() - 1)].y,
                    -polygon2[next_id(pnt_1, (int)polygon2.size() - 1)].x);
            } else {
                // Rotate polygon2 points to the right
                // 1st polygon, side 7-8 or 8-1
                p1_1 = cv::Point2f(polygon1[pnt_1].x, polygon1[pnt_1].y);
                p1_2 = cv::Point2f(polygon1[next_id(pnt_1, (int)polygon1.size() - 1)].x,
                    polygon1[next_id(pnt_1, (int)polygon1.size() - 1)].y);
                // 2nd polygon, side 1-0 or 0-8
                p2_1 = cv::Point2f(-polygon2[pnt_2].y, polygon2[pnt_2].x);
                p2_2 = cv::Point2f(-polygon2[previous_id(pnt_2, (int)polygon2.size() - 1)].y,
                    polygon2[previous_id(pnt_2, (int)polygon2.size() - 1)].x);
            }

            // Calculate polygon sides intersection
            Line line1 = GetAlphaBeta(p1_1, p1_2);
            Line line2 = GetAlphaBeta(p2_1, p2_2);
            seam.p1 = GetIntersection(line1, line2);

            // Check if intersection point lays on a polygon side
            if ((seam.p1.x > MIN(p2_1.x, p2_2.x)) && (seam.p1.y > MIN(p2_1.y, p2_2.y)) && (seam.p1.x < MAX(p2_1.x, p2_2.x)) && (seam.p1.y < MAX(p2_1.y, p2_2.y)) && (seam.p1.x > MIN(p1_1.x, p1_2.x)) && (seam.p1.y > MIN(p1_1.y, p1_2.y)) && (seam.p1.x < MAX(p2_1.x, p2_2.x)) && (seam.p1.y < MAX(p2_1.y, p2_2.y))) {
                intersection = true; // Terminate searching process
                double radius;
                if (rotation < 0) // left
                {
                    radius = MAX(pow(polygon1[0].y, 2) + pow(polygon1[0].x, 2),
                        pow(polygon2[polygon2.size() - 1].y, 2) + pow(polygon2[polygon2.size() - 1].x, 2));
                } else // right
                {
                    radius = MAX(pow(polygon1[polygon1.size() - 1].y, 2) + pow(polygon1[polygon1.size() - 1].x, 2),
                        pow(polygon2[0].y, 2) + pow(polygon2[0].x, 2));
                }
                // Check if intersection point belongs to the 1st and 2nd grid which don't cover whole polygons area
                if (radius > ((double)seam.p1.x * seam.p1.x + (double)seam.p1.y * seam.p1.y)) {
                    double alpha = (double)seam.p1.y / seam.p1.x;
                    if (seam.p1.x < 0.0) {
                        seam.p1.x = -1.0f * (float)sqrt(radius / (1.0 + (alpha * alpha)));
                    } else {
                        seam.p1.x = (float)sqrt(radius / (1.0 + (alpha * alpha)));
                    }
                    seam.p1.y = (float)alpha * seam.p1.x;
                }
            }
            pnt_2--; // 1st seam point
        }
        pnt_1++; // 8th seam point
    }

    // Search for the second intersection point of input polygons
    cv::Point2f p1_4, p1_5, p2_4, p2_5;
    // 1st polygon, side 4-5
    p1_4 = cv::Point2f(polygon1[3].x, polygon1[3].y);
    p1_5 = cv::Point2f(polygon1[4].x, polygon1[4].y);

    // 2st polygon, side 4-5
    if (rotation < 0) // Rotate polygon2 points to the left
    {
        p2_4 = cv::Point2f(polygon1[3].y, -polygon1[3].x);
        p2_5 = cv::Point2f(polygon1[4].y, -polygon1[4].x);
    } else // Rotate polygon2 points to the right
    {
        p2_4 = cv::Point2f(-polygon2[3].y, polygon2[3].x);
        p2_5 = cv::Point2f(-polygon2[4].y, polygon2[4].x);
    }

    // Calculate polygon sides intersection
    Line line1 = GetAlphaBeta(p1_4, p1_5);
    Line line2 = GetAlphaBeta(p2_4, p2_5);
    seam.p2 = GetIntersection(line1, line2);

    seam.line = GetAlphaBeta(seam.p1, seam.p2);
}
