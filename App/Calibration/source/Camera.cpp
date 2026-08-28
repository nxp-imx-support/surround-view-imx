/*
 * Copyright 2017, 2022, 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Camera.hpp"

#include "AssetManager.hpp"
#include "Log.hpp"

#include <opencv2/calib3d/calib3d_c.h>
#include <opencv2/imgcodecs/legacy/constants_c.h>

Camera::Camera(std::shared_ptr<CameraModel> model, int index)
    : mModel(model)
    , mIndex(index)
{
    SetRoi(50);
}

cv::Mat Camera::GetK()
{
    cv::Mat M;
    mParams.K.copyTo(M);
    return M;
}

cv::Mat Camera::GetDistCoeffs()
{
    cv::Mat M;
    mParams.distCoeffs.copyTo(M);
    return M;
}

cv::Mat Camera::GetRvec(void)
{
    cv::Mat M;
    mParams.rvec.copyTo(M);
    return M;
}

cv::Mat Camera::GetTvec(void)
{
    cv::Mat M;
    mParams.tvec.copyTo(M);
    return M;
}

std::shared_ptr<CameraModel> Camera::GetModel()
{
    return mModel;
}

int Camera::GetIndex()
{
    return mIndex;
}

const CameraTemplate& Camera::GetTemplate()
{
    return mTemplate;
}

void Camera::UpdateLUT(float scale_factor)
{
    mModel->SetScale(scale_factor);
}

void Camera::SetRoi(int roi)
{
    if ((uint)roi > 100U) {
        mRoi = 1.0f;
    } else {
        mRoi = (float)roi / 100.0f;
    }
}

void Camera::SetContourMinSize(int size)
{
    mContourMinSize = size;
}

int Camera::SetTemplate(std::string filename, cv::Size templateSize)
{
    mTemplate.filename = AssetManager::GetPath(filename);

    int x, y;
    int max_x = 0;
    mTemplate.pt_count = 0;
    std::ifstream ifs_ref(mTemplate.filename);
    while (ifs_ref >> x >> y) {
        if (x > max_x) {
            // Get width of reference pattern
            max_x = x;
        }
        // Set number of reference points
        mTemplate.pt_count++;
    }
    if (max_x != templateSize.width) {
        mTemplate.size = cv::Size(max_x, templateSize.width);
    } else {
        mTemplate.size = cv::Size(max_x, templateSize.height);
    }

    ifs_ref.close();
    mPosterSize = templateSize;

    return 0;
}

int Camera::SetIntrinsic(std::string filepath, std::string filename, int imageCount, cv::Size patternSize)
{
    // 1.Load chessboard image
    std::vector<std::vector<cv::Point3f>> object_points;
    std::vector<std::vector<cv::Point2f>> image_points;

    // Define chessboard corners in 3D spase
    std::vector<cv::Point3f> obj;
    for (int j = 0; j < patternSize.width * patternSize.height; j++) {
        obj.push_back(cv::Point3f(j / patternSize.width, j % patternSize.height, 0.0f));
    }

    for (int i = 0; i < imageCount; i++) {
        std::string img_name = filepath + filename + std::to_string(i) + ".jpg";
        std::string filePath = AssetManager::GetPath(img_name);
        cv::Mat chessboard_img = cv::imread(filePath.c_str(), cv::IMREAD_COLOR);
        if (chessboard_img.empty()) {
            LogError("Image %s not found", filePath.c_str());
            return -1;
        }

        // 2. Calculate maps for fisheye undistortion using Scarramuza calibrating data
        // Remove fisheye distortion
        mModel->Remap(chessboard_img, chessboard_img);

        // 3. Calculate camera intrinsic parameters
        // Convert the image into a grayscale image
        cv::Mat chessboard_gray;
        cvtColor(chessboard_img, chessboard_gray, CV_BGR2GRAY);

        // Find chessboard corners
        std::vector<cv::Point2f> corners; // Chessboard corners in 2D camera frame
        if (findChessboardCorners(chessboard_img, patternSize, corners,
                CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FILTER_QUADS)) {
            // Store points results into the lists
            image_points.push_back(corners);
            object_points.push_back(obj);
        }
    }

    // Calculate intrinsic parameters
    if (object_points.size() > 0) {
        mParams.K = initCameraMatrix2D(object_points, image_points, mModel->Size(), 0.0);
        mParams.distCoeffs = cv::Mat(4, 1, CV_32F, cv::Scalar(0));
        std::cout << "K: \n"
                  << mParams.K << std::endl;
    } else {
        LogError("Problem with corner detection");
        return -1;
    }
    return 0;
}

int Camera::SetExtrinsic(const cv::Mat& img)
{
    int x, y;

    // 1. Apply camera model (Defisheye)
    cv::Mat und_img;
    mModel->Remap(img, und_img);

    // 2. Get 3D reference points
    mTemplate.ref_points.clear();
    std::ifstream ifs_ref(mTemplate.filename);
    while (ifs_ref >> x >> y) {
        mTemplate.ref_points.push_back(cv::Point3f(x, y, 0));
    }

    // Normalization into [-1, 1] according template size
    for (uint j = 0; j < mTemplate.ref_points.size(); j++) {
        mTemplate.ref_points[j].x = (2.0f * mTemplate.ref_points[j].x - (float)mTemplate.size.width) / (float)mPosterSize.width;
        mTemplate.ref_points[j].y = (2.0f * mTemplate.ref_points[j].y - (float)mTemplate.size.height) / (float)mPosterSize.width;
    }

    // 3. Get points from distorted image
    mImagePoints.clear();
    if (GetImagePoints(und_img, mTemplate.pt_count, mImagePoints) != 0) {
        return -1;
    }

    // 4. Find an object pose from 3D-2D point correspondences.
    std::vector<cv::Point2f> image_points;
    std::vector<cv::Point3f> object_points;
    for (uint j = 0; j < mImagePoints.size(); j++) {
        image_points.push_back(mImagePoints[j]);
        object_points.push_back(cv::Point3f(mTemplate.ref_points[j].x, mTemplate.ref_points[j].y, 0.0));
    }

    cv::Mat matImgPoints(image_points);
    cv::Mat matObjPoints(object_points);
    if (!cv::solvePnP(matObjPoints, matImgPoints, mParams.K, mParams.distCoeffs, mParams.rvec, mParams.tvec)) {
        return -1;
    }

#if 0
    // 5. Calculate camera world position.
    // Translation and rotation vectors from solvePnP are telling where is the object in camera's coordinates.
    // To determine world coordinates of a camera we need to get an inverse transform.
    cv::Mat R;
    Rodrigues(mParams.rvec, R);
    cv::Mat cameraRotationVector;
    Rodrigues(R.t(), cameraRotationVector);
    cv::Mat cameraTranslationVector = -R.t() * mParams.tvec;
    std::cout << "Camera translation  " <<  cameraTranslationVector << std::endl;
    std::cout << "Camera rotation  " <<  cameraRotationVector << std::endl;
#endif

    mRadius = sqrt((double)pow(mTemplate.ref_points[0].y, 2) + (double)pow(mTemplate.ref_points[0].x, 2));

    return 0;
}

int Camera::GetBowlHeight(double radius, double stepX)
{
    if (mParams.rvec.empty() || mParams.tvec.empty() || mParams.K.empty()) {
        return 0;
    }

    int num = 1;
    bool nextPoint = true;

    // Get mask for defisheye transformation
    cv::Mat transformMask(mModel->Size(), CV_8U, cv::Scalar(255));
    mModel->Remap(transformMask, transformMask);

    // Get 3D points projection into 2D image for bowl side with x = 0 (z = (y - radius)^2)
    while ((nextPoint) && (num < 100)) {
        std::vector<cv::Point3f> p3d;
        std::vector<cv::Point2f> p2d;

        double newPoint = (double)num * stepX;
        p3d.push_back(cv::Point3f(0, -radius - newPoint, -newPoint * newPoint));
        // Project the point into 2D image
        projectPoints(p3d, mParams.rvec, mParams.tvec, mParams.K, mParams.distCoeffs, p2d);

        if ((p2d[0].y >= 0.0) && (p2d[0].y < (float)transformMask.rows) && (p2d[0].x >= 0.0) && (p2d[0].x < (float)transformMask.cols)) {
            if ((transformMask.data != NULL) && (transformMask.at<uchar>((int)round(p2d[0].y), (int)round(p2d[0].x)) != 255U)) {
                nextPoint = false;
            }
        } else {
            nextPoint = false;
        }
        num++;
    }
    return (MAX(0, (num - 2)));
}

int Camera::GetImagePoints(cv::Mat& undistImg, uint num, std::vector<cv::Point2f>& img_points)
{
    // 1. Contour detections
    // Convert to grayscale
    cv::Mat undistImgGray;
    cvtColor(undistImg, undistImgGray, CV_RGB2GRAY);

    // Get Roi
    cv::Rect rect(0, static_cast<int>((float)undistImgGray.rows * (1.0f - mRoi)) - 10, undistImgGray.cols,
        static_cast<int>((float)undistImgGray.rows * mRoi));
    cv::Mat temp;
    undistImgGray(rect).copyTo(temp);

    cv::Ptr<CvMemStorage> memStorage;
    memStorage = cvCreateMemStorage(0);
    CvSeq* root = cvCreateSeq(0, sizeof(CvSeq), sizeof(CvSeq*), memStorage);

    // Get contours
    int contoursCount = GetContours(temp, &root, memStorage, mContourMinSize);

    // If number of contours not equal to CONTOURS_NUM, then complete the calibration process
    if (contoursCount < CONTOURS_NUM) {
        SequenceToVector(&root, img_points, cv::Point2f(0.0f, ((float)undistImg.rows * (1.0f - mRoi)) - 10.0f));
        if (contoursCount == 0) {
            LogError("Camera %d. No contours were found. Change the calibration image", mIndex);
            return -1;
        }
        LogError("Camera %d. The number of contours is fewer than 4. Change the calibration image", mIndex);
        return -1;
    } else {
        if (contoursCount > CONTOURS_NUM) {
            SequenceToVector(&root, img_points, cv::Point2f(0.0f, ((float)undistImg.rows * (1.0f - mRoi)) - 10.0f));
            LogError("Camera %d. The number of contours is bigger than 4. Change the calibration image", mIndex);
            return -1;
        }
    }

    // 2. Sort contours from left to right
    SortContours(&root);

    // 3. Get contours points
    cv::Point2f shift = cv::Point2f(0.0f, ((float)undistImg.rows * (1.0f - mRoi)) - 10.0f);
    // Sort contour points clockwise (start from the top left point)
    GetFeaturePoints(&root, img_points, shift);

    // Draw contours
    for (int i = 0; i < (int)img_points.size() - 1; i++) {
        line(undistImg, img_points[i], img_points[i + 1], cvScalar(255.0, 0.0, 0.0), 1, CV_AA, 0);
    }

    // Check points count
    if (img_points.size() != num) {
        LogError("Too few points were found");
        return -1;
    }
    return 0;
}

int Camera::CreateContours(float** lines)
{
    float xNorm = 1.0f / (float)mModel->Size().width;
    float yNorm = 1.0f / (float)mModel->Size().height;

    (*lines) = (float*)malloc(3 * 2 * mImagePoints.size() * sizeof(float));
    if ((*lines) == NULL) {
        LogError("Memory allocation did not complete successfully");
        return 0;
    }

    for (uint i = 0U; i < mImagePoints.size(); i++) {
        uint nextI = 4U * (i / 4U) + ((i + 1U) % 4U);

        (*lines)[6U * i] = ToClipSpaceX(mImagePoints[i].x * xNorm);
        (*lines)[6U * i + 1U] = ToClipSpaceY(mImagePoints[i].y * yNorm);
        (*lines)[6U * i + 2U] = 0.0f;

        (*lines)[6U * i + 3U] = ToClipSpaceX(mImagePoints[nextI].x * xNorm);
        (*lines)[6U * i + 4U] = ToClipSpaceY(mImagePoints[nextI].y * yNorm);
        (*lines)[6U * i + 5U] = 0.0f;
    }

    return (6 * (int)mImagePoints.size());
}

int Camera::CreateVertices(float** vertices, int density, std::string filename)
{
    cv::Size mapSize = mModel->Size();
    if ((mapSize.height == 0) || (mapSize.width == 0) || (mapSize.height == 0) || (mapSize.width == 0)) {
        LogError("Defisheye mesh not generated. LUTs are empty");
        return 0;
    }

    int rows = mapSize.height / density;
    int cols = mapSize.width / density;

    int count = 6 * rows * cols;
    *vertices = (float*)calloc(count * 5, sizeof(float));
    float* ptr = *vertices;

    if (ptr == NULL) {
        LogError("Memory allocation did not complete successfully");
        return 0;
    }

    float xNorm = 1.0f / (float)mapSize.width;
    float yNorm = 1.0f / (float)mapSize.height;

    int k = 0;
    for (int row = 1; row < rows; row++) {
        for (int col = 1; col < cols; col++) {
            // Get triangles (v4-v1-v2) and (v4-v2-v3)
            // v3 _  v2
            //   | /|
            //   |/_|
            // v4    v1

            // Vertices
            cv::Point2f v1 = cv::Point2f(col * density, row * density);
            cv::Point2f v2 = cv::Point2f(col * density, (row - 1) * density);
            cv::Point2f v3 = cv::Point2f((col - 1) * density, (row - 1) * density);
            cv::Point2f v4 = cv::Point2f((col - 1) * density, row * density);

            // Texels
            cv::Point2f p1 = cv::Point2f(mModel->XMapAt(cv::Point2i(v1)), mModel->YMapAt(cv::Point2i(v1)));
            cv::Point2f p2 = cv::Point2f(mModel->XMapAt(cv::Point2i(v2)), mModel->YMapAt(cv::Point2i(v2)));
            cv::Point2f p3 = cv::Point2f(mModel->XMapAt(cv::Point2i(v3)), mModel->YMapAt(cv::Point2i(v3)));
            cv::Point2f p4 = cv::Point2f(mModel->XMapAt(cv::Point2i(v4)), mModel->YMapAt(cv::Point2i(v4)));

            // Check if p2 and p4 belong to the input frame
            if (mModel->MapHas(p2) && mModel->MapHas(p4)) {
                // Save triangle (v4-v1-v2)
                // Check if p1 belongs to the input frame
                if (mModel->MapHas(p1)) {
                    ptr[k] = ToClipSpaceX(v1.x * xNorm);
                    ptr[k + 1] = ToClipSpaceY(v1.y * yNorm);
                    ptr[k + 2] = 0.0f;
                    ptr[k + 3] = p1.x * xNorm;
                    ptr[k + 4] = p1.y * yNorm;

                    ptr[k + 5] = ToClipSpaceX(v2.x * xNorm);
                    ptr[k + 6] = ToClipSpaceY(v2.y * yNorm);
                    ptr[k + 7] = 0.0f;
                    ptr[k + 8] = p2.x * xNorm;
                    ptr[k + 9] = p2.y * yNorm;

                    ptr[k + 10] = ToClipSpaceX(v4.x * xNorm);
                    ptr[k + 11] = ToClipSpaceY(v4.y * yNorm);
                    ptr[k + 12] = 0.0f;
                    ptr[k + 13] = p4.x * xNorm;
                    ptr[k + 14] = p4.y * yNorm;

                    k += 15;
                }

                // Save triangle (v4-v2-v3)
                // Check if p3 belongs to the input frame
                if (mModel->MapHas(p3)) {
                    ptr[k] = ToClipSpaceX(v4.x * xNorm);
                    ptr[k + 1] = ToClipSpaceY(v4.y * yNorm);
                    ptr[k + 2] = 0.0f;
                    ptr[k + 3] = p4.x * xNorm;
                    ptr[k + 4] = p4.y * yNorm;

                    ptr[k + 5] = ToClipSpaceX(v2.x * xNorm);
                    ptr[k + 6] = ToClipSpaceY(v2.y * yNorm);
                    ptr[k + 7] = 0.0f;
                    ptr[k + 8] = p2.x * xNorm;
                    ptr[k + 9] = p2.y * yNorm;

                    ptr[k + 10] = ToClipSpaceX(v3.x * xNorm);
                    ptr[k + 11] = ToClipSpaceY(v3.y * yNorm);
                    ptr[k + 12] = 0.0f;
                    ptr[k + 13] = p3.x * xNorm;
                    ptr[k + 14] = p3.y * yNorm;

                    k += 15;
                }
            }
        }
    }

    // Write ptr into file for debug purpose
    if (filename.empty() == false) {
        std::ofstream outstream;
        std::string filePath = AssetManager::GetPath(filename);
        LogInfo("Writing mesh to file %s", filePath.c_str());
        outstream.open(filePath.c_str(), std::ofstream::out | std::ofstream::trunc);
        for (int i = 0; i < k; i += 5) {
            for (int j = 0; j < 4; j++) {
                outstream << ptr[i + j] << " ";
            }
            outstream << ptr[i + 4] << std::endl;
        }
    }

    return count;
}
