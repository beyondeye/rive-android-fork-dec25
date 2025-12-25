#include <jni.h>
#include <android/log.h>

#define LOG_TAG "mprive-android"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("mprive-android JNI_OnLoad");
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved) {
    LOGI("mprive-android JNI_OnUnload");
}

// Stub implementation for nativeInit
JNIEXPORT jboolean JNICALL
Java_app_rive_mprive_RivePlatform_nativeInit(JNIEnv* env, jobject thiz) {
    LOGI("RivePlatform.nativeInit() called");
    // TODO: Initialize Rive runtime
    return JNI_TRUE;
}

} // extern "C"
