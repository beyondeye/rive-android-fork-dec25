/**
 * JNI entry point for mprive Android native library.
 * 
 * This file provides JNI initialization and core platform functions.
 * Additional JNI bindings are implemented in nativeInterop/cpp/src/bindings/ files.
 */

#include <jni.h>
#include "jni_refs.hpp"
#include "rive_log.hpp"

extern "C" {

/**
 * Called when the native library is loaded.
 * Stores the JVM pointer and initializes the logging system.
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    rive_mp::g_JVM = vm;
    
    // Initialize logging
    rive_mp::InitializeRiveLog();
    
    LOGI("mprive-android native library loaded");
    LOGD("JNI_OnLoad completed successfully");
    
    return JNI_VERSION_1_6;
}

/**
 * Called when the native library is unloaded.
 */
JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved) {
    LOGD("mprive-android native library unloading");
    rive_mp::g_JVM = nullptr;
}

/**
 * Initialize Rive platform.
 * Called from Kotlin to ensure native library is loaded and initialized.
 */
JNIEXPORT jboolean JNICALL
Java_app_rive_mprive_RivePlatform_nativeInit(JNIEnv* env, jobject thiz) {
    LOGI("RivePlatform.nativeInit() called");
    
    // Initialize JNI class loader if not already done
    rive_mp::InitJNIClassLoader(env, thiz);
    
    // Log platform info
    LOGI("Rive platform initialized successfully");
    
    return JNI_TRUE;
}

} // extern "C"
