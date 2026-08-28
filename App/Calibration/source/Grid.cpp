/*
 * Copyright 2017, 2022, 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Grid.hpp"

#include "AssetManager.hpp"
#include "FilesName.hpp"
#include "Log.hpp"

void Grid::SetAngles(uint val)
{
    mParams.angles = val;
}

void Grid::SetStartAngle(uint val)
{
    mParams.startAngle = val;
}

void Grid::SetNopZ(uint val)
{
    mParams.zPointsCount = val;
}

void Grid::SetStepX(double val)
{
    mParams.stepX = val;
}

void Grid::GetSeamPoints(std::vector<cv::Point3f>& seamPoints)
{
    seamPoints.clear();
    for (uint i = 0; i < mSeamPoints.size(); i++) {
        seamPoints.push_back(mSeamPoints[i]);
    }
}

int Grid::GetGrid(float** points)
{
    int size = 0;

    if (mP2D.size() == 0) {
        return (0);
    }

    float xNorm = 1.0f / (float)mCamInfo.width;
    float yNorm = 1.0f / (float)mCamInfo.height;

    (*points) = (float*)malloc(6 * mP2D.size() * sizeof(float));
    if ((*points) == NULL) {
        LogError("Memory allocation did not complete successfully");
        return (0);
    }

    for (uint i = 0; i < mP2D.size(); i++) {
        if ((mP2D[i].x < (float)mCamInfo.width) && (mP2D[i].y < (float)mCamInfo.height) && (mP2D[i].x > 0.0) && (mP2D[i].y > 0.0)) {
            (*points)[size] = Camera::ToClipSpaceX(mP2D[i].x * xNorm);
            (*points)[size + 1] = Camera::ToClipSpaceY(mP2D[i].y * yNorm);
            (*points)[size + 2] = 0.0f;

            size += 3;
        }
    }

    return size;
}

cv::Point3f Grid::RotatePoint(int index, cv::Point3f point)
{
    cv::Point3f result;
    switch (index) {
    case (0): // Without rotation
        result.x = point.x;
        result.y = -point.y;
        result.z = -point.z;
        break;
    case (1): // 90 degree clockwise rotation
        result.x = -point.y;
        result.y = -point.x;
        result.z = -point.z;
        break;
    case (2): // 180 degree clockwise rotation
        result.x = -point.x;
        result.y = point.y;
        result.z = -point.z;
        break;
    default: // case(3) 270 degree clockwise rotation
        result.x = point.y;
        result.y = point.x;
        result.z = -point.z;
        break;
    }
    return (result);
}

CurvilinearGrid::CurvilinearGrid(uint angles, uint startAngle, uint zPointsCount, double stepX)
{
    mParams.angles = angles;
    mParams.startAngle = startAngle;
    mParams.zPointsCount = zPointsCount;
    mParams.stepX = stepX;
    mCamInfo.height = 0;
    mCamInfo.width = 0;
    mCamInfo.index = -1;
    mPointsCount = 0;
}

void CurvilinearGrid::CreateGrid(std::shared_ptr<Camera> camera, double radius)
{
    mCamInfo.height = camera->GetModel()->Size().height;
    mCamInfo.width = camera->GetModel()->Size().width;
    mCamInfo.index = camera->GetIndex();

    mP3D.clear();
    mP2D.clear();

    if (((camera->GetRvec()).empty()) || ((camera->GetTvec()).empty()) || (radius <= 0.0)) {
        LogError("Grid %d was not generated", camera->GetIndex());
        return;
    }

    // Number of grid rows of flat bowl bottom
    int pnum = static_cast<int>(radius / mParams.stepX);
    // The first row of grid
    int startpnum = static_cast<int>((float)camera->GetTemplate().ref_points[0].y / (float)camera->GetTemplate().ref_points[0].x / (float)mParams.stepX / 3.0f);
    // Number of grid points for one grid sector (angle)
    mPointsCount = 2 * ((pnum - startpnum) + (int)mParams.zPointsCount);

    for (uint ang = mParams.startAngle + 1U; ang <= (uint)(mParams.angles - mParams.startAngle); ang++) {
        double angle_start = (double)((double)ang - 1.0) * (M_PI / (double)mParams.angles);
        double angle_end = (double)ang * (M_PI / (double)mParams.angles);

        // Flat bottom points
        for (int i = startpnum; i < pnum; i++) {
            double hptn = (double)i * mParams.stepX;
            mP3D.push_back(cv::Point3f(hptn * cos(angle_end), -hptn * sin(angle_end), 0.0));
            mP3D.push_back(cv::Point3f(hptn * cos(angle_start), -hptn * sin(angle_start), 0.0));
        }

        // Points on bowl side
        for (uint i = 1; i <= mParams.zPointsCount; i++) {
            double hptn = radius + (double)i * mParams.stepX;
            mP3D.push_back(
                cv::Point3f(hptn * cos(angle_end), -hptn * sin(angle_end), -pow((double)i * mParams.stepX, 2)));
            mP3D.push_back(
                cv::Point3f(hptn * cos(angle_start), -hptn * sin(angle_start), -pow((double)i * mParams.stepX, 2)));
        }
    }

    // Projects 3D points to an image plane
    cv::projectPoints(mP3D, camera->GetRvec(), camera->GetTvec(), camera->GetK(), camera->GetDistCoeffs(), mP2D);

    // Reorganize grid to clean middle part of view
    ReorgGrid(radius, camera);

    // Find seam points
    FindSeam(camera, mSeamPoints);
}

void CurvilinearGrid::ReorgGrid(double radius, std::shared_ptr<Camera> camera)
{
    double gridStep = radius / 4.0;

    // 3D coordinates of the closest point for the camera
    std::vector<cv::Point3f> point3D;
    point3D.push_back(cv::Point3f(0.0, (float)(-gridStep), 0.0)); // Start value

    // Mask for defisheye image
    cv::Mat distortionMask(camera->GetModel()->Size(), CV_8U, cv::Scalar(255, 255, 255));
    camera->GetModel()->Remap(distortionMask, distortionMask);

    // Get closest point for the camera which exists on camera frame
    for (int i = 0; i < 10; i++) // 10 iteration is enough
    {
        // 2D coordinates of the closest point for the camera on defisheye image
        std::vector<cv::Point2f> point2D;
        cv::projectPoints(point3D, camera->GetRvec(), camera->GetTvec(), camera->GetK(), camera->GetDistCoeffs(),
            point2D);
        gridStep = gridStep / 2.0; // Increase step
        cv::Point p = point2D[0];
        if ((p.x < distortionMask.cols) && (p.y < distortionMask.rows) && (p.x >= 0) && (p.y >= 0)) {
            if (distortionMask.at<uchar>(p) == 0U) // Check mask value
            {
                point3D[0].y -= gridStep; // Go down
            } else {
                point3D[0].y += gridStep; // Go up
            }
        } else {
            point3D[0].y -= gridStep; // Go down
        }
    }

    // Recalculate 3D and 2D grid points
    for (uint i = 0; i < mP3D.size(); i++) {
        if (mP3D[i].y > point3D[0].y) {
            // If grid point is lays in masking region
            std::vector<cv::Point2f> p2tmp;
            std::vector<cv::Point3f> p3tmp;
            mP3D[i].y = point3D[0].y;
            p3tmp.push_back(mP3D[i]);
            cv::projectPoints(p3tmp, camera->GetRvec(), camera->GetTvec(), camera->GetK(), camera->GetDistCoeffs(), p2tmp);
            mP2D[i] = p2tmp[0];
        }
    }
}

void CurvilinearGrid::FindSeam(std::shared_ptr<Camera> camera, std::vector<cv::Point3f>& seamPoints)
{
    bool isfound = false;
    // index of the middle angle
    int middleAngle = ((int)mParams.angles - 2 * (int)mParams.startAngle) / 2;
    seamPoints.clear();

    // Get mask for defisheye transformation
    std::shared_ptr<CameraModel> model = camera->GetModel();
    cv::Mat defisheye_mask(model->Size(), CV_8U, cv::Scalar(255));
    model->Remap(defisheye_mask, defisheye_mask);

    // p1 - 1st point is located on flat circle base (z = 0).
    // It is the leftmost point with minimum value of radial coordinate in polar coordinate system.
    int pointCount = 1;
    while ((!isfound) && (pointCount < (mPointsCount - 2 * (int)mParams.zPointsCount))) {
        for (int ang = (int)mParams.angles - 2 * (int)mParams.startAngle - 1; ang >= middleAngle; ang--) {
            int idx = ang * mPointsCount + pointCount;
            cv::Point p((int)round(mP2D[idx].x), (int)round(mP2D[idx].y));
            if (model->MapHas(p)) {
                if ((defisheye_mask.data != NULL) && (defisheye_mask.at<uchar>(p) != 0U)) {
                    seamPoints.push_back(mP3D[idx]);
                    isfound = true;
                    break;
                }
            }
        }
        pointCount += 2;
    }

    // p2 - 2nd point is located on flat circle base (z = 0).
    // It is the leftmost point of grid which lies on base circle edge.
    for (int ang = (int)mParams.angles - 2 * (int)mParams.startAngle; ang >= middleAngle; ang--) {
        int idx = mPointsCount * ang - 2 * (int)mParams.zPointsCount - 2;
        if (model->MapHas(mP2D[idx])) {
            seamPoints.push_back(mP3D[idx]);
            break;
        }
    }

    // p3 - 3rd point is located on bowl edge.
    // It is the last point in first grid column with (z != 0).
    int angleCount = (int)mParams.angles - 2 * (int)mParams.startAngle;
    isfound = false;
    while ((!isfound) && (angleCount >= middleAngle)) {
        for (int idx = mPointsCount * angleCount - 2; idx >= mPointsCount * angleCount - 2 * (int)mParams.zPointsCount; idx -= 2) {
            if (model->MapHas(mP2D[idx])) {
                seamPoints.push_back(mP3D[idx]);
                isfound = true;
                break;
            }
        }
        angleCount--;
    }

    // p4 - 4th point is located on bowl edge.
    // It is the leftmost point with maximum value of z coordinate.
    angleCount = (int)mParams.angles - 2 * (int)mParams.startAngle;
    isfound = false;
    while ((!isfound) && (angleCount >= middleAngle)) {
        int idx = mPointsCount * angleCount - 2;
        if (model->MapHas(mP2D[idx])) {
            seamPoints.push_back(mP3D[idx]);
            isfound = true;
        }
        angleCount--;
    }

    // p5 - 5th point is located on bowl edge.
    // It is the rightmost point with maximum value of z coordinate.
    angleCount = 1;
    isfound = false;
    while ((!isfound) && (angleCount <= (int)middleAngle)) {
        int idx = mPointsCount * angleCount - 1;
        if (model->MapHas(mP2D[idx])) {
            seamPoints.push_back(mP3D[idx]);
            isfound = true;
        }
        angleCount++;
    }

    // p6 - 6th point is located on bowl edge.
    // It is the last point in last grid column with (z != 0).
    angleCount = 1;
    isfound = false;
    while ((!isfound) && (angleCount <= (int)middleAngle)) {
        for (int idx = mPointsCount * angleCount - 1; idx >= mPointsCount * angleCount - 2 * (int)mParams.zPointsCount; idx -= 2) {
            if (model->MapHas(mP2D[idx])) {
                seamPoints.push_back(mP3D[idx]);
                isfound = true;
                break;
            }
        }
        angleCount++;
    }

    // p7 - 7th point is located on flat circle base (z = 0).
    // It is the rightmost point of grid which lies on base circle edge.
    for (int ang = 0; ang <= middleAngle; ang++) {
        int idx = mPointsCount * (ang + 1) - 2 * (int)mParams.zPointsCount - 1;
        if (model->MapHas(mP2D[idx])) {
            seamPoints.push_back(mP3D[idx]);
            break;
        }
    }

    // p8 - 8th point is located on flat circle base (z = 0).
    // It is the rightmost point with minimum value of radial coordinate in polar coordinate system.
    pointCount = 1;
    isfound = false;
    while ((!isfound) && (pointCount < mPointsCount - 2 * (int)mParams.zPointsCount)) {
        for (int ang = 0; ang < (int)middleAngle; ang++) {
            int idx = ang * mPointsCount + pointCount;
            cv::Point p((int)round(mP2D[idx].x), (int)round(mP2D[idx].y));
            if (model->MapHas(p)) {
                if ((defisheye_mask.data != NULL) && (defisheye_mask.at<uchar>(p) != 0U)) {
                    seamPoints.push_back(mP3D[idx]);
                    isfound = true;
                    break;
                }
            }
        }
        pointCount += 2;
    }
}

void CurvilinearGrid::SaveGrid(std::shared_ptr<Camera> camera)
{
    auto model = camera->GetModel();

    // 2D grid size (texels)
    float height = (float)model->Size().height;
    float width = (float)model->Size().width;

    // Generate output array
    AssetManager::MakeDirectory(GENERATED_DIR);
    AssetManager::MakeDirectory(VERTICES_DIR);
    std::string file_name = AssetManager::GetPath(VERTICES_DIR ARRAY_PREFIX + std::to_string(camera->GetIndex() + 1));

    std::ofstream outC;
    outC.open(file_name.c_str(), std::ofstream::out | std::ofstream::trunc);
    for (uint ang = 0U; ang < mParams.angles - 2U * mParams.startAngle; ang++) {
        for (int i = 0; i < mPointsCount - 2; i += 2) {
            int p = (int)ang * mPointsCount + i;

            // Get triangles for I quadrant of template
            // 1st triangle (p - p+1 - p+2)
            // 2nd triangle (p+1 - p+2 - p+3)
            // p  _  p+2
            //   | /|
            //   |/_|
            // p+1    p+3

            if ((round(mP2D[p + 1].x) < width) && (round(mP2D[p + 1].y) < height) && (mP2D[p + 1].x >= 0.0) && (mP2D[p + 1].y >= 0.0) && (round(mP2D[p + 2].x) < width) && (round(mP2D[p + 2].y) < height) && (mP2D[p + 2].x >= 0.0) && (mP2D[p + 2].y >= 0.0)) {
                // Recalculate point for fisheye image
                cv::Point2f t2 = cv::Point2f(model->XMapAt(mP2D[p + 1]) / width, model->YMapAt(mP2D[p + 1]) / height);
                cv::Point2f t3 = cv::Point2f(model->XMapAt(mP2D[p + 2]) / width, model->YMapAt(mP2D[p + 2]) / height);

                // Rotate grid point according to the template
                cv::Point3f v2 = RotatePoint(camera->GetIndex(), mP3D[p + 1]);
                cv::Point3f v3 = RotatePoint(camera->GetIndex(), mP3D[p + 2]);

                // 1st triangle (p - p+1 - p+2)
                if ((round(mP2D[p].x) < width) && (round(mP2D[p].y) < height) && (mP2D[p].x >= 0.0) && (mP2D[p].y >= 0.0)) {
                    cv::Point3f v1 = RotatePoint(camera->GetIndex(), mP3D[p]);
                    cv::Point2f t1 = cv::Point2f(model->XMapAt(mP2D[p]) / width, model->YMapAt(mP2D[p]) / height);

                    // Save triangle points to the output file
                    outC << v1.x << " " << v1.y << " " << v1.z << " " << t1.x << " " << t1.y << std::endl;
                    outC << v2.x << " " << v2.y << " " << v2.z << " " << t2.x << " " << t2.y << std::endl;
                    outC << v3.x << " " << v3.y << " " << v3.z << " " << t3.x << " " << t3.y << std::endl;
                }

                // 2nd triangle (p+1 - p+2 - p+3)
                if ((round(mP2D[p + 3].x) < width) && (round(mP2D[p + 3].y) < height) && (mP2D[p + 3].x >= 0.0) && (mP2D[p + 3].y >= 0.0)) {
                    cv::Point3f v4 = RotatePoint(camera->GetIndex(), mP3D[p + 3]);
                    cv::Point2f t4 = cv::Point2f(model->XMapAt(mP2D[p + 3]) / width, model->YMapAt(mP2D[p + 3]) / height);

                    // Save triangle points to the output file
                    outC << v2.x << " " << v2.y << " " << v2.z << " " << t2.x << " " << t2.y << std::endl;
                    outC << v4.x << " " << v4.y << " " << v4.z << " " << t4.x << " " << t4.y << std::endl;
                    outC << v3.x << " " << v3.y << " " << v3.z << " " << t3.x << " " << t3.y << std::endl;
                }
            }
        }
    }
    outC.close();
}

RectilinearGrid::RectilinearGrid(uint angles, uint startAngle, uint zPointsCount, double stepX)
{
    mParams.angles = angles;
    mParams.startAngle = startAngle;
    mParams.zPointsCount = zPointsCount;
    mParams.stepX = stepX;
    mCamInfo.height = 0;
    mCamInfo.width = 0;
    mCamInfo.index = -1;
}

void RectilinearGrid::CreateGrid(std::shared_ptr<Camera> camera, double radius)
{
    mCamInfo.height = camera->GetModel()->Size().height;
    mCamInfo.width = camera->GetModel()->Size().width;
    mCamInfo.index = camera->GetIndex();

    mP3D.clear();
    mP2D.clear();
    mPointsCount.clear();

    if (((camera->GetRvec()).empty()) || ((camera->GetTvec()).empty()) || (radius <= 0.0)) {
        LogError("Grid %d was not generated", camera->GetIndex());
        return;
    }

    double step_x_2 = mParams.stepX * mParams.stepX;

    for (uint i = 0U; i < 2U * (mParams.angles - mParams.startAngle); i++) {
        mPointsCount.push_back(0);
    }

    // X and Y coordinates for circle base
    std::vector<double> xxx; // Array of x coordinates of grid (in row z = 0, y = 0)
    std::vector<double> yyy; // Array of y coordinates of grid (in column z = 0, x = 0)

    // The circle is divided into arcs of same values
    // The intersection points of arcs define X and Y coordinates of grid corners
    for (uint j = 0; j <= mParams.angles; j++) {
        double ang = (double)j * (M_PI_2 / (double)mParams.angles); // Arcs are defined by angles
        xxx.push_back(-radius * cos(ang));
        yyy.push_back(-radius * sin(ang));
    }

    // For each column of grid (II quadrant)
    for (uint j = mParams.startAngle; j < xxx.size(); j++) {
        // Add points of circle base (z = 0)
        for (uint k = mParams.startAngle; k <= j; k++) {
            mP3D.push_back(cv::Point3f(xxx[j], yyy[k], 0));
        }
        // Add 3D part of grid (bowl edge)
        for (double zz = 1.0; zz <= (double)mParams.zPointsCount; zz++) {
            double new_z = zz * zz * step_x_2;
            double new_x = xxx[j] * (1.0 + sqrt(new_z) / radius);
            double new_y = yyy[j] * (1.0 + sqrt(new_z) / radius);
            mP3D.push_back(cv::Point3f(new_x, new_y, -new_z));
        }
        // Set the number of grid point in the column
        mPointsCount[j - mParams.startAngle] = (int)j - (int)mParams.startAngle + 1 + (int)mParams.zPointsCount;
    }

    // For each column of grid (I quadrant)
    for (int j = (int)xxx.size() - 2; j >= (int)mParams.startAngle; j--) {
        // Add points of circle base (z = 0)
        for (int k = (int)mParams.startAngle; k <= j; k++) {
            mP3D.push_back(cv::Point3f(-xxx[j], yyy[k], 0));
        }
        // Add 3D part of grid (bowl edge)
        for (double zz = 1.0; zz <= (double)mParams.zPointsCount; zz++) {
            double new_z = zz * zz * step_x_2;
            double new_x = xxx[j] * (-1.0 - sqrt(new_z) / radius);
            double new_y = yyy[j] * (1.0 + sqrt(new_z) / radius);
            mP3D.push_back(cv::Point3f(new_x, new_y, -new_z));
        }
        // Set the number of grid point in the column
        mPointsCount[2 * (int)xxx.size() - j - (int)mParams.startAngle - 2] =
            j - (int)mParams.startAngle + 1 + (int)mParams.zPointsCount;
    }

    // Projects 3D points to an image plane
    cv::projectPoints(mP3D, camera->GetRvec(), camera->GetTvec(), camera->GetK(), camera->GetDistCoeffs(), mP2D);

    // Find seam points
    FindSeam(camera, mSeamPoints);
}

void RectilinearGrid::FindSeam(std::shared_ptr<Camera> camera, std::vector<cv::Point3f>& seamPoints)
{
    auto model = camera->GetModel();
    cv::Point3f p;
    int offset = 0;
    double min_y = -100.0;
    double min_z = 0.0;

    int pointCount = 0;
    for (uint i = 0; i < mPointsCount.size(); i++) {
        pointCount += mPointsCount[i];
    }

    seamPoints.clear(); // Clear output std::vector

    // p1 - 1st point is located on flat circle base (z = 0).
    // It is the leftmost point with minimum value of y coordinate.
    // For all column in II quadrant
    for (uint i = 0; i < mPointsCount.size() / 2; i++) {
        // Check only flat base circle
        for (uint j = 0; j < (mPointsCount[i] - mParams.zPointsCount); j++) {
            uint index = offset + j;
            // Check if point belongs to grid
            if (model->MapHas(mP2D[index])) {
                if (min_y < mP3D[index].y) {
                    min_y = mP3D[index].y; // Get minimum y
                    p = mP3D[index];
                }
            }
        }
        offset += mPointsCount[i]; // Add number of point in i column
    }
    seamPoints.push_back(p); // Push p1 to the output std::vector

    // p2 - 2nd point is located on flat circle base (z = 0).
    // It is the leftmost point of grid which lies on base circle edge.
    offset = 0;
    // For all column in II quadrant
    for (uint i = 0; i < mPointsCount.size() / 2; i++) {
        // Get index of point which lies on base circle edge in i column
        int index = offset + mPointsCount[i] - (int)mParams.zPointsCount - 1;
        // Check if point belongs to grid
        if (model->MapHas(mP2D[index])) {
            seamPoints.push_back(mP3D[index]); // Push p2 to the output std::vector

            // p3 - 3rd point is located on bowl edge.
            // It is the last point in first grid column with (z != 0).
            for (int k = index + 1; k < offset + mPointsCount[i]; k++) {
                // Check if point does not belong to grid
                if (model->MapHas(mP2D[k]) == false) {
                    seamPoints.push_back(mP3D[k - 1]);
                    break;
                } else {
                    // Check if k point is the last in the i column
                    if (k == offset + mPointsCount[i] - 1) {
                        seamPoints.push_back(mP3D[k]);
                    }
                }
            }
            break;
        }
        offset += mPointsCount[i]; // Add number of point in i column
    }

    // p4 - 4th point is located on bowl edge.
    // It is the leftmost point with maximum value of z coordinate.
    offset = 0;
    // For all column in II quadrant
    for (uint i = 0; i < mPointsCount.size() / 2; i++) {
        // Check only bowl edge with z != 0
        for (int j = mPointsCount[i] - (int)mParams.zPointsCount; j < mPointsCount[i]; j++) {
            uint index = offset + j;
            // Check if point belongs to grid
            if (model->MapHas(mP2D[index])) {
                if (min_z > mP3D[index].z) {
                    min_z = mP3D[index].z; // Get minimum z
                    p = mP3D[index];
                }
            }
        }
        offset += mPointsCount[i]; // Add number of point in i column
    }
    seamPoints.push_back(p); // Push p4 to the output std::vector

    // p5 - 5th point is located on bowl edge.
    // It is the rightmost point with maximum value of z coordinate.
    offset = pointCount; // Set offset
    min_z = 0.0;         // Reset minimum z
    // For all columns in I quadrant
    for (uint i = (uint)mPointsCount.size() - 1U; i > (uint)mPointsCount.size() / 2U; i--) {
        // Check only bowl edge with z != 0
        for (uint j = 0; j < mParams.zPointsCount; j++) {
            // Check if point belongs to grid
            uint index = offset - j;
            if (model->MapHas(mP2D[index])) {
                if (min_z > mP3D[index].z) {
                    min_z = mP3D[index].z; // Get minimum z
                    p = mP3D[index];
                }
            }
        }
        offset -= mPointsCount[i]; // Subtract number of point in i column
    }
    seamPoints.push_back(p); // Push p5 to the output std::vector

    // p7 - 7th point is located on flat circle base (z = 0).
    // It is the rightmost point of grid which lies on base circle edge.
    offset = pointCount; // Set offset
    // For all columns in I quadrant
    for (uint i = (uint)mPointsCount.size() - 1U; i > (uint)mPointsCount.size() / 2U; i--) {
        // Get index of point which lies on base circle edge in i column
        uint index = offset - (int)mParams.zPointsCount - 1;
        // Check if point belongs to grid
        if (model->MapHas(mP2D[index])) {
            // p6 - 6th point is located on bowl edge.
            // It is the last point in last grid column with (z != 0).
            for (int k = index + 1; k < offset; k++) {
                // Check if point does not belong to grid
                if (model->MapHas(mP2D[k]) == false) {
                    seamPoints.push_back(mP3D[k - 1]); // Push p6 to the output std::vector
                    seamPoints.push_back(mP3D[index]); // Push p7 to the output std::vector
                    break;
                } else {
                    // Check if k point is the last in the i column
                    if (k == offset - 1) {
                        seamPoints.push_back(mP3D[k]);     // Push p6 to the output std::vector
                        seamPoints.push_back(mP3D[index]); // Push p7 to the output std::vector
                    }
                }
            }
            break;
        }
        offset -= mPointsCount[i]; // Subtract number of point in i column
    }

    // p8 - 8th point is located on flat circle base (z = 0).
    // It is the rightmost point with minimum value of y coordinate
    offset = pointCount; // Set offset
    min_y = -100.0;      // Reset minimum y
    // For all column in I quadrant
    for (int i = (int)mPointsCount.size() / 2 - 1; i > 0; i--) {
        // Check only flat base circle
        for (int j = mPointsCount[i] - (int)mParams.zPointsCount - 1; j > 0; j--) {
            uint index = offset - j;
            // Check if point belongs to grid
            if (model->MapHas(mP2D[index])) {
                if ((min_y < mP3D[index].y) && (mP3D[index].z == 0.0)) {
                    min_y = mP3D[index].y; // Get minimum y
                    p = mP3D[index];
                }
            }
        }
        offset -= mPointsCount[i]; // Subtract number of point in i column
    }
    seamPoints.push_back(p); // Push p8 to the output std::vector
}

void RectilinearGrid::SaveGrid(std::shared_ptr<Camera> camera)
{
    auto model = camera->GetModel();

    // 2D grid size (texels)
    float height = (float)model->Size().height;
    float width = (float)model->Size().width;

    // Generate output array
    char file_name[50];
    sprintf(file_name, "./array%d", camera->GetIndex() + 1);

    std::ofstream outC;
    outC.open(file_name, std::ofstream::out | std::ofstream::trunc);

    int offset = mPointsCount[0]; // Set offset of point in 3D grid (vertices)

    // Get triangles for II quadrant of template
    // 1st triangle (p4-p1-p2)
    // 2nd triangle (p4-p2-p3)
    // p3 _  p2
    //   | /|
    //   |/_|
    // p4   p1

    // Repeat for each column of 3D grid
    for (uint xx = 1U; xx <= mParams.angles - mParams.startAngle; xx++) {
        // Repeat for each row of the xx column
        for (int yy = 0; yy < mPointsCount[xx]; yy++) {
            int p1 = offset + yy;                               // Point with current offset (x, y)
            int p2 = offset + (yy + 1);                         // Next point in the same column (x, y + 1)
            int p3 = offset + (yy + 1) - mPointsCount[xx - 1U]; // Neighbor point from previous column (x - 1, y + 1)
            int p4 = offset + yy - mPointsCount[xx - 1U];       // Neighbor point from previous column (x - 1, y)

            // Check if points p2 and p4 are exist in the 3D vertices grid
            if ((p4 < offset) && (p2 < offset + mPointsCount[xx])) {
                // 1st triangle (p4-p1-p2)
                if ((round(mP2D[p2].x) < width) && (round(mP2D[p2].y) < height) && (mP2D[p2].x >= 0.0) && (mP2D[p2].y >= 0.0) && (round(mP2D[p4].x) < width) && (round(mP2D[p4].y) < height) && (mP2D[p4].x >= 0.0) && (mP2D[p4].y >= 0.0)) {
                    if (model->MapHas(mP2D[p1])) {
                        // Recalculate point for fisheye image
                        cv::Point2f p4_new = cv::Point2f(model->XMapAt(mP2D[p4]) / width, model->YMapAt(mP2D[p4]) / height);
                        cv::Point2f p1_new = cv::Point2f(model->XMapAt(mP2D[p1]) / width, model->YMapAt(mP2D[p1]) / height);
                        cv::Point2f p2_new = cv::Point2f(model->XMapAt(mP2D[p2]) / width, model->YMapAt(mP2D[p2]) / height);

                        // Rotate grid point according to the template
                        cv::Point3f vertex4 = RotatePoint(camera->GetIndex(), mP3D[p4]);
                        cv::Point3f vertex1 = RotatePoint(camera->GetIndex(), mP3D[p1]);
                        cv::Point3f vertex2 = RotatePoint(camera->GetIndex(), mP3D[p2]);

                        // Save triangle points to the output file
                        outC << vertex4.x << " " << vertex4.y << " " << vertex4.z << " " << p4_new.x << " " << p4_new.y
                             << std::endl;
                        outC << vertex1.x << " " << vertex1.y << " " << vertex1.z << " " << p1_new.x << " " << p1_new.y
                             << std::endl;
                        outC << vertex2.x << " " << vertex2.y << " " << vertex2.z << " " << p2_new.x << " " << p2_new.y
                             << std::endl;
                    }

                    // 2nd triangle (p4-p2-p3)
                    if ((p3 < offset) && (round(mP2D[p3].x) < width) && (round(mP2D[p3].y) < height) && (mP2D[p3].x >= 0.0) && (mP2D[p3].y >= 0.0)) {
                        // Recalculate point for fisheye image
                        cv::Point2f p4_new = cv::Point2f(model->XMapAt(mP2D[p4]) / width, model->YMapAt(mP2D[p4]) / height);
                        cv::Point2f p2_new = cv::Point2f(model->XMapAt(mP2D[p2]) / width, model->YMapAt(mP2D[p2]) / height);
                        cv::Point2f p3_new = cv::Point2f(model->XMapAt(mP2D[p3]) / width, model->YMapAt(mP2D[p3]) / height);

                        // Rotate grid point according to the template
                        cv::Point3f vertex4 = RotatePoint(camera->GetIndex(), mP3D[p4]);
                        cv::Point3f vertex2 = RotatePoint(camera->GetIndex(), mP3D[p2]);
                        cv::Point3f vertex3 = RotatePoint(camera->GetIndex(), mP3D[p3]);

                        // Save triangle points to the output file
                        outC << vertex4.x << " " << vertex4.y << " " << vertex4.z << " " << p4_new.x << " " << p4_new.y
                             << std::endl;
                        outC << vertex2.x << " " << vertex2.y << " " << vertex2.z << " " << p2_new.x << " " << p2_new.y
                             << std::endl;
                        outC << vertex3.x << " " << vertex3.y << " " << vertex3.z << " " << p3_new.x << " " << p3_new.y
                             << std::endl;
                    }
                }
            }
        }
        offset += mPointsCount[xx];
    }

    // Get triangles for I quadrant of template
    // 1st triangle (p4-p1-p3)
    // 2nd triangle (p1-p2-p3)
    // p3 _  p2
    //  |\ |
    //  |_\|
    // p4   p1
    for (uint xx = mParams.angles + 1U - mParams.startAngle;
        xx < 2U * (mParams.angles - mParams.startAngle) + 1U; xx++) {
        for (int yy = 0; yy < mPointsCount[xx]; yy++) {
            int p1 = offset + yy;                               // Point with current offset (x, y)
            int p2 = offset + (yy + 1);                         // Next point in the same column (x, y + 1)
            int p3 = offset + (yy + 1) - mPointsCount[xx - 1U]; // Neighbor point from previous column (x - 1, y + 1)
            int p4 = offset + yy - mPointsCount[xx - 1U];       // Neighbor point from previous column (x - 1, y)

            if (p3 < offset) // Check if point p3 is exist in the 3D vertices grid
            {
                // 1st triangle (p4-p1-p3)
                if ((round(mP2D[p1].x) < width) && (round(mP2D[p1].y) < height) && (mP2D[p1].x >= 0.0) && (mP2D[p1].y >= 0.0) && (round(mP2D[p3].x) < width) && (round(mP2D[p3].y) < height) && (mP2D[p3].x >= 0.0) && (mP2D[p3].y >= 0.0)) {
                    if ((round(mP2D[p4].x) < width) && (round(mP2D[p4].y) < height) && (mP2D[p4].x >= 0.0) && (mP2D[p4].y >= 0.0)) {
                        // Recalculate point for fisheye image
                        cv::Point2f p4_new = cv::Point2f(model->XMapAt(mP2D[p4]) / width, model->YMapAt(mP2D[p4]) / height);
                        cv::Point2f p1_new = cv::Point2f(model->XMapAt(mP2D[p1]) / width, model->YMapAt(mP2D[p1]) / height);
                        cv::Point2f p3_new = cv::Point2f(model->XMapAt(mP2D[p3]) / width, model->YMapAt(mP2D[p3]) / height);

                        // Rotate grid point according to the template
                        cv::Point3f vertex4 = RotatePoint(camera->GetIndex(), mP3D[p4]);
                        cv::Point3f vertex1 = RotatePoint(camera->GetIndex(), mP3D[p1]);
                        cv::Point3f vertex3 = RotatePoint(camera->GetIndex(), mP3D[p3]);

                        // Save triangle points to the output file
                        outC << vertex4.x << " " << vertex4.y << " " << vertex4.z << " " << p4_new.x << " " << p4_new.y
                             << std::endl;
                        outC << vertex1.x << " " << vertex1.y << " " << vertex1.z << " " << p1_new.x << " " << p1_new.y
                             << std::endl;
                        outC << vertex3.x << " " << vertex3.y << " " << vertex3.z << " " << p3_new.x << " " << p3_new.y
                             << std::endl;
                    }

                    // 2nd triangle (p1-p2-p3)
                    if ((p2 < offset + mPointsCount[xx]) && (round(mP2D[p2].x) < width) && (round(mP2D[p2].y) < height) && (mP2D[p2].x >= 0.0) && (mP2D[p2].y >= 0.0)) {
                        // Recalculate point for fisheye image
                        cv::Point2f p1_new = cv::Point2f(model->XMapAt(mP2D[p1]) / width, model->YMapAt(mP2D[p1]) / height);
                        cv::Point2f p2_new = cv::Point2f(model->XMapAt(mP2D[p2]) / width, model->YMapAt(mP2D[p2]) / height);
                        cv::Point2f p3_new = cv::Point2f(model->XMapAt(mP2D[p3]) / width, model->YMapAt(mP2D[p3]) / height);

                        // Rotate grid point according to the template
                        cv::Point3f vertex1 = RotatePoint(camera->GetIndex(), mP3D[p1]);
                        cv::Point3f vertex2 = RotatePoint(camera->GetIndex(), mP3D[p2]);
                        cv::Point3f vertex3 = RotatePoint(camera->GetIndex(), mP3D[p3]);

                        // Save triangle points to the output file
                        outC << vertex1.x << " " << vertex1.y << " " << vertex1.z << " " << p1_new.x << " " << p1_new.y
                             << std::endl;
                        outC << vertex2.x << " " << vertex2.y << " " << vertex2.z << " " << p2_new.x << " " << p2_new.y
                             << std::endl;
                        outC << vertex3.x << " " << vertex3.y << " " << vertex3.z << " " << p3_new.x << " " << p3_new.y
                             << std::endl;
                    }
                }
            }
        }
        offset += mPointsCount[xx];
    }
    outC.close();
}
