/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Camera2.hpp"

#include "Log.hpp"
#include "Texture.hpp"
#include "Window.hpp"

// clang-format off
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
// clang-format on

#include <android/native_window_jni.h>
#include <camera/NdkCameraCaptureSession.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCaptureRequest.h>
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/eglext.h>
#include <jni.h>

struct _AndroidContext
{
    ACameraManager* mCameraManager = nullptr;
    ACameraDevice* mCameraDevice = nullptr;
    ACameraCaptureSession* mCaptureSession = nullptr;
    ACaptureRequest* mCaptureRequest = nullptr;
    ACameraOutputTarget* mOutputTarget = nullptr;
    ANativeWindow* mNativeWindow = nullptr;
    ACaptureSessionOutput* mSessionOutput = nullptr;
    ACaptureSessionOutputContainer* mOutputContainer = nullptr;

    jobject mSurfaceTexture = nullptr;
    JNIEnv* mJniEnv = nullptr;
    JavaVM* mJavaVM = nullptr;
};

// Device state callbacks
static void onCameraDisconnected(void* context, ACameraDevice* device)
{
    LogWarning("Camera device disconnected");
}

static void onCameraError(void* context, ACameraDevice* device, int error)
{
    LogError("Camera device error: %d", error);
}

static ACameraDevice_StateCallbacks deviceStateCallbacks = {
    .context = nullptr,
    .onDisconnected = onCameraDisconnected,
    .onError = onCameraError,
};

// Session state callbacks
static void onSessionActive(void* context, ACameraCaptureSession* session)
{
    LogInfo("Capture session active");
}

static void onSessionReady(void* context, ACameraCaptureSession* session)
{
    LogInfo("Capture session ready");
}

static void onSessionClosed(void* context, ACameraCaptureSession* session)
{
    LogInfo("Capture session closed");
}

static ACameraCaptureSession_stateCallbacks sessionStateCallbacks = {
    .context = nullptr,
    .onClosed = onSessionClosed,
    .onReady = onSessionReady,
    .onActive = onSessionActive,
};

// Capture callbacks
static void onCaptureCompleted(void* context, ACameraCaptureSession* session,
    ACaptureRequest* request, const ACameraMetadata* result)
{
}

static void onCaptureFailed(void* context, ACameraCaptureSession* session,
    ACaptureRequest* request, ACameraCaptureFailure* failure)
{
    LogError("Capture failed");
}

static ACameraCaptureSession_captureCallbacks captureCallbacks = {
    .context = nullptr,
    .onCaptureStarted = nullptr,
    .onCaptureProgressed = nullptr,
    .onCaptureCompleted = onCaptureCompleted,
    .onCaptureFailed = onCaptureFailed,
    .onCaptureSequenceCompleted = nullptr,
    .onCaptureSequenceAborted = nullptr,
    .onCaptureBufferLost = nullptr,
};

// JNI helper function to get SurfaceTexture from Java
static jobject getSurfaceTextureFromJava(JNIEnv* env, GLuint textureId, uint32_t width, uint32_t height)
{
    // Find the Camera2Native class
    jclass camera2NativeClass = env->FindClass("com/nxp/sv3d/SV3DRenderer");
    if (!camera2NativeClass) {
        LogError("Failed to find Camera2Native class");
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    // Get the static getSurfaceTexture method
    jmethodID getSurfaceTextureMethod = env->GetStaticMethodID(
        camera2NativeClass,
        "getSurface",
        "(III)Landroid/view/Surface;");

    if (!getSurfaceTextureMethod) {
        LogError("Failed to find getSurfaceTexture method");
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->DeleteLocalRef(camera2NativeClass);
        return nullptr;
    }

    // Call getSurfaceTexture
    jobject surfaceTexture = env->CallStaticObjectMethod(
        camera2NativeClass,
        getSurfaceTextureMethod,
        static_cast<jint>(textureId),
        static_cast<jint>(width),
        static_cast<jint>(height));

    if (env->ExceptionCheck()) {
        LogError("Exception calling getSurfaceTexture");
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->DeleteLocalRef(camera2NativeClass);
        return nullptr;
    }

    env->DeleteLocalRef(camera2NativeClass);

    if (!surfaceTexture) {
        LogError("getSurfaceTexture returned null");
        return nullptr;
    }

    return surfaceTexture;
}

Camera2::Camera2(std::string source, int width, int height, std::shared_ptr<Window> window)
    : VideoStream(source, width, height)
{
    camera_status_t status;

    mContext = std::make_unique<_AndroidContext>();

    mContext->mCameraManager = ACameraManager_create();
    if (mContext->mCameraManager == nullptr) {
        LogError("Failed to create camera manager");
        return;
    }

    // Get camera ID list
    ACameraIdList* cameraIdList = nullptr;
    status = ACameraManager_getCameraIdList(mContext->mCameraManager, &cameraIdList);
    if (status != ACAMERA_OK) {
        LogError("Failed to get camera ID list. Error code: 0x%X", status);
        return;
    }

    int sourceId = std::stoi(source);
    if (cameraIdList->numCameras < sourceId + 1) {
        LogError("Camera %d not available", sourceId);
        ACameraManager_deleteCameraIdList(cameraIdList);
        return;
    }

    const char* cameraId = cameraIdList->cameraIds[sourceId];
    LogInfo("Using camera ID: %s (total cameras: %d)", cameraId, cameraIdList->numCameras);

    // Open camera device
    deviceStateCallbacks.context = this;
    status = ACameraManager_openCamera(mContext->mCameraManager, cameraId, &deviceStateCallbacks, &mContext->mCameraDevice);
    if (status != ACAMERA_OK) {
        LogError("Failed to open camera device. Error code: 0x%X", status);
        ACameraManager_deleteCameraIdList(cameraIdList);
        return;
    }

    LogInfo("Camera device opened successfully");

    mTexture = std::make_shared<Texture>(mWidth, mHeight, GL_TEXTURE_EXTERNAL_OES);

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        LogError("OpenGL error after creating texture: 0x%X", error);
        ACameraManager_deleteCameraIdList(cameraIdList);
        return;
    }

    LogInfo("Created GL_TEXTURE_EXTERNAL_OES texture: GL ID=%u", mTexture->GetId());

    // Get JNI environment
    JNIEnv* env = nullptr;
    JavaVM* javaVM = nullptr;
    jsize javaSize;

    // Get the JavaVM from the current thread
    if (JNI_GetCreatedJavaVMs(&javaVM, 1, &javaSize) != JNI_OK || javaVM == nullptr) {
        LogError("Failed to get JavaVM");
        ACameraManager_deleteCameraIdList(cameraIdList);
        return;
    }

    mContext->mJavaVM = javaVM;

    // Attach current thread to get JNIEnv
    jint attachResult = javaVM->AttachCurrentThread(&env, nullptr);
    if (attachResult != JNI_OK || env == nullptr) {
        LogError("Failed to attach current thread to JavaVM");
        ACameraManager_deleteCameraIdList(cameraIdList);
        return;
    }

    mContext->mJniEnv = env;

    // Get SurfaceTexture from Java
    jobject surface = getSurfaceTextureFromJava(env, mTexture->GetId(), mWidth, mHeight);
    if (!surface) {
        LogError("Failed to get SurfaceTexture from Java");
        ACameraManager_deleteCameraIdList(cameraIdList);
        return;
    }

    // Get ANativeWindow from Surface
    mContext->mNativeWindow = ANativeWindow_fromSurface(env, surface);

    if (!mContext->mNativeWindow) {
        LogError("Failed to get ANativeWindow from Surface");
        ACameraManager_deleteCameraIdList(cameraIdList);
        return;
    }

    LogInfo("SurfaceTexture created and native window obtained successfully");

    ACameraManager_deleteCameraIdList(cameraIdList);

    LogInfo("Camera2 initialized with SurfaceTexture");
    if (!mContext->mCameraDevice) {
        LogError("Camera device not initialized");
        return;
    }

    if (!mContext->mNativeWindow) {
        LogError("Native window not set");
        return;
    }

    // Create output container
    ACaptureSessionOutputContainer_create(&mContext->mOutputContainer);

    // Create session output
    ACaptureSessionOutput_create(mContext->mNativeWindow, &mContext->mSessionOutput);
    ACaptureSessionOutputContainer_add(mContext->mOutputContainer, mContext->mSessionOutput);

    // Create output target
    ACameraOutputTarget_create(mContext->mNativeWindow, &mContext->mOutputTarget);

    // Create capture request
    status = ACameraDevice_createCaptureRequest(mContext->mCameraDevice, TEMPLATE_PREVIEW, &mContext->mCaptureRequest);
    if (status != ACAMERA_OK) {
        LogError("Failed to create capture request. Error code: 0x%X", status);
        return;
    }

    // Add target to request
    ACaptureRequest_addTarget(mContext->mCaptureRequest, mContext->mOutputTarget);

    sessionStateCallbacks.context = this;
    status = ACameraDevice_createCaptureSession(
        mContext->mCameraDevice,
        mContext->mOutputContainer,
        &sessionStateCallbacks,
        &mContext->mCaptureSession);

    if (status != ACAMERA_OK) {
        LogError("Failed to create capture session. Error code: 0x%X", status);
        return;
    }

    LogInfo("Capture setup completed successfully");
}

Camera2::~Camera2()
{
    Stop();

    if (mContext->mJniEnv && mContext->mSurfaceTexture) {
        mContext->mJniEnv->DeleteGlobalRef(mContext->mSurfaceTexture);
        mContext->mSurfaceTexture = nullptr;
    }

    if (mContext->mNativeWindow) {
        ANativeWindow_release(mContext->mNativeWindow);
        mContext->mNativeWindow = nullptr;
    }

    if (mContext->mCaptureRequest) {
        ACaptureRequest_free(mContext->mCaptureRequest);
    }

    if (mContext->mOutputTarget) {
        ACameraOutputTarget_free(mContext->mOutputTarget);
    }

    if (mContext->mSessionOutput) {
        ACaptureSessionOutput_free(mContext->mSessionOutput);
    }

    if (mContext->mOutputContainer) {
        ACaptureSessionOutputContainer_free(mContext->mOutputContainer);
    }

    if (mContext->mCameraDevice) {
        ACameraDevice_close(mContext->mCameraDevice);
    }

    if (mContext->mCameraManager) {
        ACameraManager_delete(mContext->mCameraManager);
        mContext->mCameraManager = nullptr;
    }
}

bool Camera2::Start(void)
{
    if (!mContext->mCaptureSession || !mContext->mCaptureRequest) {
        LogError("Capture session or request not initialized");
        return false;
    }

    captureCallbacks.context = this;
    camera_status_t status = ACameraCaptureSession_setRepeatingRequest(
        mContext->mCaptureSession,
        &captureCallbacks,
        1,
        &mContext->mCaptureRequest,
        nullptr);

    if (status != ACAMERA_OK) {
        LogError("Failed to start repeating request. Error code: 0x%X", status);
        return false;
    }

    mIsRunning = true;
    LogInfo("Camera capture started successfully");
    return true;
}

void Camera2::Stop(void)
{
    if (mContext->mCaptureSession) {
        ACameraCaptureSession_stopRepeating(mContext->mCaptureSession);
        ACameraCaptureSession_close(mContext->mCaptureSession);
        mContext->mCaptureSession = nullptr;
    }

    mIsRunning = false;
    LogInfo("Camera capture stopped");
}

std::shared_ptr<Texture> Camera2::GetTexture()
{
    if (mTexture) {
        mHasNewFrame = false;
        return mTexture;
    }

    return nullptr;
}
