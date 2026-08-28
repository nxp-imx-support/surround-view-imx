/*
 * Copyright 2017, 2022, 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "FisheyeModel.hpp"

#include "AssetManager.hpp"
#include "Log.hpp"

FisheyeModel::FisheyeModel(std::string filename, float scaleFactor)
{
    struct stat st;
    std::string filePath = AssetManager::GetPath(filename);
    if (stat(filePath.c_str(), &st) != 0) {
        LogError("Camera model file %s not found for FisheyeModel", filePath.c_str());
    } else {
        std::string line;
        std::ifstream ifs_ref(filePath.c_str());
        int line_num = 0;
        while (getline(ifs_ref, line)) {
            if ((line.length() > 1) && (line.at(0) != '#')) {
                std::stringstream strStream(line);
                double value;

                switch (line_num) {
                case 0: // Polynomial coefficients
                    strStream >> value;
                    while (strStream >> value) {
                        mPolCoefs.push_back(value);
                    }
                    break;
                case 1: // Polynomial coefficients for the inverse mapping function
                    strStream >> value;
                    while (strStream >> value) {
                        mInvPolCoefs.push_back(value);
                    }
                    break;
                case 2: // Center
                    strStream >> mCenter.x;
                    strStream >> mCenter.y;
                    break;
                case 3: // Affine parameters
                    strStream >> mAffine(0, 0);
                    strStream >> mAffine(0, 1);
                    strStream >> mAffine(1, 0);
                    mAffine(1, 1) = 1.0;
                    break;
                case 4: // Image size
                    strStream >> mImgSize.height;
                    strStream >> mImgSize.width;
                    break;
                default:
                    break;
                }
                line_num++;
            }
        }
        ifs_ref.close();
    }

    mScale = scaleFactor;
    UpdateLUT();
}

void FisheyeModel::UpdateLUT()
{
    cv::Point3d p3D;

    mXmap.create(mImgSize.height, mImgSize.width, CV_32FC1);
    mYmap.create(mImgSize.height, mImgSize.width, CV_32FC1);

    float xc_norm = (float)mImgSize.width / 2.0f;
    float yc_norm = (float)mImgSize.height / 2.0f;
    p3D.z = (double)(-mImgSize.width) / mScale; // Z

    for (int col = 0; col < mImgSize.width; col++) {
        for (int row = 0; row < mImgSize.height; row++) {
            p3D.x = (double)row - (double)yc_norm; // X
            p3D.y = (double)col - (double)xc_norm; // Y

            // norm = sqrt(X^2 + Y^2)
            double norm = sqrt(p3D.x * p3D.x + p3D.y * p3D.y);
            if (norm == 0.0) {
                mXmap.at<float>(row, col) = (float)mCenter.y;
                mYmap.at<float>(row, col) = (float)mCenter.x;
                continue;
            }

            // t = atan(Z/sqrt(X^2 + Y^2))
            double t = atan(p3D.z / norm);

            // r = a0 + a1 * t + a2 * t^2 + a3 * t^3 + ...
            double t_pow = t;
            double r = mInvPolCoefs[0];

            for (uint i = 1; i < mInvPolCoefs.size(); i++) {
                r += t_pow * mInvPolCoefs[i];
                t_pow *= t;
            }

            // | u | = r * | X | / sqrt(X^2 + Y^2);
            // | v |       | Y |
            double u = r * p3D.x / norm;
            double v = r * p3D.y / norm;

            // | x | = | sx  shy | * | u | + | xc |
            // | y |   | shx  1  |   | v |   | yc |
            mYmap.at<float>(row, col) = (float)((mAffine(0, 0) * u + mAffine(0, 1) * v + mCenter.x));
            mXmap.at<float>(row, col) = (float)((mAffine(1, 0) * u + mAffine(1, 1) * v + mCenter.y));
        }
    }
}

void FisheyeModel::Cam2World(cv::Point3d* p3d, cv::Point2d p2d)
{
    // 1/det(A), where A = [c,d;e,1] as in the Matlab file
    double invdet = 1.0 / (mAffine(0, 0) - mAffine(0, 1) * mAffine(1, 0));

    double xp = invdet * ((p2d.x - mCenter.x) - mAffine(0, 1) * (p2d.y - mCenter.y));
    double yp = invdet * (-mAffine(1, 0) * (p2d.x - mCenter.x) + mAffine(0, 0) * (p2d.y - mCenter.y));

    // Distance [pixels] of the point from the image center
    double r = sqrt(xp * xp + yp * yp);
    double zp = mPolCoefs[0];
    double r_i = 1.0;

    for (uint i = 1; i < mPolCoefs.size(); i++) {
        r_i *= r;
        zp += r_i * mPolCoefs[i];
    }

    // Normalize to unit norm
    double invnorm = 1.0 / sqrt(xp * xp + yp * yp + zp * zp);

    p3d->x = invnorm * xp;
    p3d->y = invnorm * yp;
    p3d->z = invnorm * zp;
}
