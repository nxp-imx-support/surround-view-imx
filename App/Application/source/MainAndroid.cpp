/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Android.hpp"
#include "AssetManager.hpp"
#include "FilesName.hpp"
#include "Log.hpp"
#include "VideoStreams.hpp"
#include "Window.hpp"

#include "Graphics.hpp"

#include "CalibrationApp.hpp"
#include "CapturingApp.hpp"
#include "RenderApp.hpp"

#include <android/asset_manager.h>
#include <jni.h>
#include <string>

std::shared_ptr<Settings> gSettings = nullptr;
std::shared_ptr<Events> gEvents = nullptr;
std::shared_ptr<Android> gWindowNative = nullptr;
std::shared_ptr<Window> gWindow = nullptr;
std::shared_ptr<View> gView = nullptr;
std::shared_ptr<VideoStreams> gVideoStreams = nullptr;
std::unique_ptr<Application> gApplication = nullptr;
bool gCameraPermissionGranted = false;
static JavaVM* gJavaVM = nullptr;

#if USE_VIV
PFNGLTEXDIRECTVIVMAPPROC pFNglTexDirectVIVMap = nullptr;
PFNGLTEXDIRECTINVALIDATEVIVPROC pFNglTexDirectInvalidateVIV = nullptr;

void InitGlViv()
{
    LogDebug("InitGlViv");
    if (pFNglTexDirectVIVMap == nullptr) {
        pFNglTexDirectVIVMap = (PFNGLTEXDIRECTVIVMAPPROC)eglGetProcAddress("glTexDirectVIVMap");
        if (pFNglTexDirectVIVMap == nullptr) {
            LogError("Extension glTexDirectVIVMap not supported");
        }
    }

    if (pFNglTexDirectInvalidateVIV == nullptr) {
        pFNglTexDirectInvalidateVIV = (PFNGLTEXDIRECTINVALIDATEVIVPROC)eglGetProcAddress("glTexDirectInvalidateVIV");
        if (pFNglTexDirectInvalidateVIV == nullptr) {
            LogError("Extension glTexDirectInvalidateVIV not supported");
        }
    }
}
#endif

void Java_com_nxp_sv3d_native_init(JNIEnv* env, jclass unused, jobject assetManager, jstring filesDirPath)
{
    (void)unused;
    LogDebug("Java_com_nxp_sv3d_native_init");

    const char* str = env->GetStringUTFChars(filesDirPath, 0);
    AssetManager::SetAndroidAssetManager(AAssetManager_fromJava(env, assetManager), std::string(str));
    env->ReleaseStringUTFChars(filesDirPath, str);

    gSettings = std::make_shared<Settings>();
    if (gSettings->ReadXML(SETTINGS_PATH_FILE) == -1) {
        LogError("Reading XML %s file failed", SETTINGS_PATH_FILE);
        return;
    }
    gEvents = std::make_shared<Events>();
    gWindowNative = std::make_shared<Android>(gSettings, gEvents);
    gView = std::make_shared<View>(0, 0, gSettings->displayWidth, gSettings->displayHeight, gEvents);

    LogInfo("Initialization complete, waiting for camera permission");

#if USE_VIV
    InitGlViv();
#endif
}

void Java_com_nxp_sv3d_native_update(JNIEnv* env)
{
    (void)env;

    // Create VideoStreams on first update if permission is granted and not yet created
    if (gCameraPermissionGranted && !gVideoStreams && gSettings && gWindowNative) {
        LogInfo("Creating VideoStreams (permission granted)");
        gVideoStreams = std::make_shared<VideoStreams>(gSettings, nullptr);
    }

    if (gApplication && gVideoStreams) {
        View::Clear();
        gApplication->Update(gVideoStreams->GetTextures());
    }
}

void Java_com_nxp_sv3d_native_set_app(JNIEnv* env, jclass unused, jobject appEnum)
{
    (void)env;
    (void)unused;

    // Deninit previous app
    gApplication.reset();

    jclass sv3dappClass = env->GetObjectClass(appEnum);
    jmethodID enumGetValueMethod = env->GetMethodID(sv3dappClass, "ordinal", "()I");
    jint appEnumValue = env->CallIntMethod(appEnum, enumGetValueMethod);

#define JavaSV3DAppField(app)                                                                                         \
    env->CallIntMethod(                                                                                               \
        env->GetStaticObjectField(sv3dappClass, env->GetStaticFieldID(sv3dappClass, #app, "Lcom/nxp/sv3d/SV3DApp;")), \
        enumGetValueMethod)

    if (appEnumValue == JavaSV3DAppField(CAPTURING)) {
        LogInfo("Starting Capturing Application");
        gApplication = std::make_unique<CapturingApp>(gSettings, gView);
    } else if (appEnumValue == JavaSV3DAppField(CALIBRATION)) {
        LogInfo("Starting Calibration Application");
        gApplication = std::make_unique<CalibrationApp>(gSettings, gView);
    } else if (appEnumValue == JavaSV3DAppField(RENDER)) {
        LogInfo("Starting Render Application");
        gApplication = std::make_unique<RenderApp>(gSettings, gView, nullptr);
    } else {
        LogError("Unknown Application ID: %d", appEnumValue);
    }
}

void Java_com_nxp_sv3d_native_deinit(JNIEnv* env)
{
    LogDebug("Java_com_nxp_sv3d_native_deinit");
    (void)env;
    gApplication.reset();
    gVideoStreams.reset();
}

void Java_com_nxp_sv3d_reload_xml_file(JNIEnv* env)
{
    (void)env;
    gSettings->ReadXML(SETTINGS_PATH_FILE);
}

void Java_com_nxp_sv3d_native_ontouchevent(JNIEnv* env, jclass unused, jobject event)
{
    (void)unused;
    gWindowNative->OnTouchEvent(env, event);
}

jboolean Java_com_nxp_sv3d_native_onkeyup(JNIEnv* env, jclass unused, jint keycode)
{
    (void)unused;
    return gWindowNative->OnKeyUp(env, keycode);
}

jboolean Java_com_nxp_sv3d_native_onkeydown(JNIEnv* env, jclass unused, jint keycode)
{
    (void)unused;
    return gWindowNative->OnKeyDown(env, keycode);
}

void Java_com_nxp_sv3d_native_oncamerapermissiongranted(JNIEnv* env, jclass unused)
{
    (void)env;
    (void)unused;
    LogDebug("Java_com_nxp_sv3d_native_oncamerapermissiongranted");
    gCameraPermissionGranted = true;
}

static JNINativeMethod sv3d_native_methods[] = {
    { "Init", "(Landroid/content/res/AssetManager;Ljava/lang/String;)V", reinterpret_cast<void*>(Java_com_nxp_sv3d_native_init) },
    { "Update", "()V", reinterpret_cast<void*>(Java_com_nxp_sv3d_native_update) },
    { "SetApp", "(Lcom/nxp/sv3d/SV3DApp;)V", reinterpret_cast<void*>(Java_com_nxp_sv3d_native_set_app) },
    { "DeInit", "()V", reinterpret_cast<void*>(Java_com_nxp_sv3d_native_deinit) },
    { "ReloadXmlFile", "()V", reinterpret_cast<void*>(Java_com_nxp_sv3d_reload_xml_file) },
    { "OnTouchEvent", "(Landroid/view/MotionEvent;)V", reinterpret_cast<void*>(Java_com_nxp_sv3d_native_ontouchevent) },
    { "OnKeyUp", "(I)Z", reinterpret_cast<void*>(Java_com_nxp_sv3d_native_onkeyup) },
    { "OnKeyDown", "(I)Z", reinterpret_cast<void*>(Java_com_nxp_sv3d_native_onkeydown) },
    { "OnCameraPermissionGranted", "()V", reinterpret_cast<void*>(Java_com_nxp_sv3d_native_oncamerapermissiongranted) },
};
unsigned int sv3d_native_methods_size = sizeof(sv3d_native_methods) / sizeof(sv3d_native_methods[0]);

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    (void)reserved;

    JNIEnv* envJni = NULL;
    jint version = JNI_VERSION_1_6;

    if (vm->GetEnv((void**)(&envJni), version) != JNI_OK) {
        LogError("JNI version 1.6 not supported");
        return -1;
    }

    // Store JavaVM for later use
    if (gJavaVM == nullptr) {
        envJni->GetJavaVM(&gJavaVM);
    }

    const char* const className = "com/nxp/sv3d/SV3DNative";
    jclass classJni = envJni->FindClass(className);
    if (classJni == NULL) {
        LogError("JNI class %s not found", className);
        return -1;
    }

    if (envJni->RegisterNatives(classJni, sv3d_native_methods, sv3d_native_methods_size) < 0) {
        LogError("Registering native methods failed for class %s", className);
        return -1;
    }

    return version;
}
