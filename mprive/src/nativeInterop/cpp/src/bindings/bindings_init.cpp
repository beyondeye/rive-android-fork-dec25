/**
 * RiveNative JNI Bindings
 * 
 * This file implements the JNI methods for the RiveNative Kotlin object.
 * These methods provide version information and platform details.
 * 
 * Note: JNI_OnLoad and JNI_OnUnload are implemented in mprive_jni.cpp
 * to avoid duplicate symbol errors.
 */

#include <jni.h>
#include "jni_refs.hpp"
#include "jni_helpers.hpp"
#include "rive_log.hpp"
#include "platform.hpp"

extern "C" {

/**
 * nativeInit - Explicit initialization method called from Kotlin.
 * 
 * This method provides a way for Kotlin code to explicitly initialize
 * the native library and perform setup that requires a Context or other
 * Java objects.
 * 
 * Java/Kotlin signature:
 *   package app.rive.mp
 *   object RiveNative {
 *       external fun nativeInit(context: Any)
 *   }
 * 
 * @param env JNI environment
 * @param thiz The RiveNative object (or class for static method)
 * @param context Android Context or similar platform object (for future use)
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_RiveNative_nativeInit(JNIEnv* env, jobject thiz, jobject context) {
    LOGD("RiveNative.nativeInit called");
    
    // Initialize JNI class loader for cross-thread class access
    // This is critical for multi-threaded apps where we might need to access
    // Java classes from non-main threads (e.g., render threads)
    rive_mp::InitJNIClassLoader(env, context);
    
    LOGI("RiveNative initialization complete");
    
    // Future: Initialize Rive runtime if it requires explicit setup
    // Currently, Rive runtime is header-only and doesn't require initialization,
    // but this is where we would add it if needed in the future.
    
    // Future: Platform-specific initialization
    // - Android: Could extract ApplicationContext, initialize audio, etc.
    // - Desktop: Could set up resource paths, etc.
}

/**
 * nativeGetVersion - Get version information about the native library.
 * 
 * Useful for debugging and ensuring the correct native library is loaded.
 * 
 * Java/Kotlin signature:
 *   package app.rive.mp
 *   object RiveNative {
 *       external fun nativeGetVersion(): String
 *   }
 * 
 * @param env JNI environment
 * @param thiz The RiveNative object
 * @return Version string (e.g., "mprive-1.0.0-android")
 */
JNIEXPORT jstring JNICALL
Java_app_rive_mp_RiveNative_nativeGetVersion(JNIEnv* env, jobject thiz) {
    // Build version string with platform information
    const char* platform = rive_mp::GetPlatformName();
    
    // Version format: "mprive-<version>-<platform>"
    char version[128];
    snprintf(version, sizeof(version), "mprive-1.0.0-%s", platform);
    
    return rive_mp::StdStringToJString(env, version);
}

/**
 * nativeGetPlatformInfo - Get detailed platform information.
 * 
 * Returns information about the current platform, useful for debugging
 * and runtime platform detection.
 * 
 * Java/Kotlin signature:
 *   package app.rive.mp
 *   object RiveNative {
 *       external fun nativeGetPlatformInfo(): String
 *   }
 * 
 * @param env JNI environment
 * @param thiz The RiveNative object
 * @return Platform info string (e.g., "Android (JNI, Mobile)")
 */
JNIEXPORT jstring JNICALL
Java_app_rive_mp_RiveNative_nativeGetPlatformInfo(JNIEnv* env, jobject thiz) {
    char info[256];
    
    // Build detailed platform info string
    #if RIVE_PLATFORM_ANDROID
        snprintf(info, sizeof(info), "Android (JNI, Mobile)");
    #elif RIVE_PLATFORM_IOS
        snprintf(info, sizeof(info), "iOS (JNI, Mobile)");
    #elif RIVE_PLATFORM_MACOS
        snprintf(info, sizeof(info), "macOS (JNI, Desktop)");
    #elif RIVE_PLATFORM_LINUX
        snprintf(info, sizeof(info), "Linux (JNI, Desktop)");
    #elif RIVE_PLATFORM_WINDOWS
        snprintf(info, sizeof(info), "Windows (JNI, Desktop)");
    #elif RIVE_PLATFORM_WASM
        snprintf(info, sizeof(info), "WebAssembly (No JNI, Web)");
    #else
        snprintf(info, sizeof(info), "Unknown Platform");
    #endif
    
    return rive_mp::StdStringToJString(env, info);
}

} // extern "C"
