/**
 * RiveFile JNI Bindings
 * 
 * This file implements JNI methods for loading and managing Rive files.
 * 
 * Memory Management:
 * - RiveFile instances are created from byte arrays
 * - Native pointers are returned as jlong handles
 * - Caller must call nativeRelease() to free resources
 */

#include <jni.h>
#include <memory>
#include <vector>
#include "jni_helpers.hpp"
#include "jni_refs.hpp"
#include "rive_log.hpp"
#include "rive/file.hpp"

extern "C" {

/**
 * Load a Rive file from a byte array.
 * 
 * Java/Kotlin signature:
 *   package app.rive.mp
 *   class RiveFile {
 *       companion object {
 *           external fun nativeLoadFile(bytes: ByteArray): Long
 *       }
 *   }
 * 
 * @param env JNI environment
 * @param clazz RiveFile class
 * @param bytes Byte array containing the .riv file data
 * @return Native pointer to rive::File (as jlong), or 0 on failure
 */
JNIEXPORT jlong JNICALL
Java_app_rive_mp_RiveFile_00024Companion_nativeLoadFile(
    JNIEnv* env,
    jobject clazz,
    jbyteArray bytes
) {
    LOGD("nativeLoadFile called");
    
    // Validate input
    if (bytes == nullptr) {
        rive_mp::ThrowRiveException(env, "File bytes cannot be null");
        return 0;
    }
    
    // Convert byte array to vector
    std::vector<uint8_t> fileData = rive_mp::JByteArrayToVector(env, bytes);
    if (fileData.empty()) {
        rive_mp::ThrowRiveException(env, "Failed to read file data");
        return 0;
    }
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Loading Rive file from byte array (size: %zu bytes)", fileData.size());
    LOGD(msg);
    
    // Import Rive file
    rive::ImportResult result;
    rive::rcp<rive::File> file = rive::File::import(
        rive::Span<const uint8_t>(fileData.data(), fileData.size()),
        nullptr,  // Factory (use default)
        &result
    );
    
    // Check for errors
    if (result != rive::ImportResult::success) {
        char errorMsg[256];
        snprintf(errorMsg, sizeof(errorMsg), 
                "Failed to import Rive file (error code: %d)", 
                static_cast<int>(result));
        LOGE(errorMsg);
        rive_mp::ThrowRiveException(env, errorMsg);
        return 0;
    }
    
    if (!file) {
        LOGE("Rive file import returned null");
        rive_mp::ThrowRiveException(env, "Failed to import Rive file: returned null");
        return 0;
    }
    
    snprintf(msg, sizeof(msg), "Rive file loaded successfully (artboards: %zu)", file->artboardCount());
    LOGI(msg);
    
    // Return pointer as jlong (file is rcp, need to increment ref count to keep alive)
    file->ref();  // Increment reference count on the File object
    return reinterpret_cast<jlong>(file.get());
}

/**
 * Get the number of artboards in a Rive file.
 * 
 * Java/Kotlin signature:
 *   package app.rive.mp
 *   class RiveFile {
 *       external fun nativeGetArtboardCount(filePtr: Long): Int
 *   }
 * 
 * @param env JNI environment
 * @param thiz RiveFile instance
 * @param filePtr Native pointer to rive::File
 * @return Number of artboards, or 0 on error
 */
JNIEXPORT jint JNICALL
Java_app_rive_mp_RiveFile_nativeGetArtboardCount(
    JNIEnv* env,
    jobject thiz,
    jlong filePtr
) {
    if (filePtr == 0) {
        rive_mp::ThrowRiveException(env, "Invalid file pointer (null)");
        return 0;
    }
    
    auto* file = reinterpret_cast<rive::File*>(filePtr);
    return static_cast<jint>(file->artboardCount());
}

/**
 * Get an artboard by index.
 * 
 * Java/Kotlin signature:
 *   package app.rive.mp
 *   class RiveFile {
 *       external fun nativeGetArtboard(filePtr: Long, index: Int): Long
 *   }
 * 
 * @param env JNI environment
 * @param thiz RiveFile instance
 * @param filePtr Native pointer to rive::File
 * @param index Artboard index (0-based)
 * @return Native pointer to rive::ArtboardInstance (as jlong), or 0 on failure
 */
JNIEXPORT jlong JNICALL
Java_app_rive_mp_RiveFile_nativeGetArtboard(
    JNIEnv* env,
    jobject thiz,
    jlong filePtr,
    jint index
) {
    char msg[256];
    snprintf(msg, sizeof(msg), "nativeGetArtboard called (index: %d)", index);
    LOGD(msg);
    
    if (filePtr == 0) {
        rive_mp::ThrowRiveException(env, "Invalid file pointer (null)");
        return 0;
    }
    
    auto* file = reinterpret_cast<rive::File*>(filePtr);
    
    // Validate index
    if (index < 0 || index >= static_cast<jint>(file->artboardCount())) {
        char errorMsg[256];
        snprintf(errorMsg, sizeof(errorMsg),
                "Artboard index out of bounds: %d (count: %zu)",
                index, file->artboardCount());
        LOGE(errorMsg);
        rive_mp::ThrowRiveException(env, errorMsg);
        return 0;
    }
    
    // Get artboard instance (returns raw pointer, need to manage lifetime)
    rive::Artboard* artboard = file->artboard(static_cast<size_t>(index));
    
    if (!artboard) {
        char errorMsg[256];
        snprintf(errorMsg, sizeof(errorMsg),
                "Failed to get artboard at index %d", index);
        LOGE(errorMsg);
        rive_mp::ThrowRiveException(env, errorMsg);
        return 0;
    }
    
    snprintf(msg, sizeof(msg), "Artboard retrieved successfully (index: %d, name: %s)", 
             index, artboard->name().c_str());
    LOGD(msg);
    
    // Return pointer as jlong (artboard is already a raw pointer)
    return reinterpret_cast<jlong>(artboard);
}

/**
 * Get an artboard by name.
 * 
 * Java/Kotlin signature:
 *   package app.rive.mp
 *   class RiveFile {
 *       external fun nativeGetArtboardByName(filePtr: Long, name: String): Long
 *   }
 * 
 * @param env JNI environment
 * @param thiz RiveFile instance
 * @param filePtr Native pointer to rive::File
 * @param name Artboard name
 * @return Native pointer to rive::ArtboardInstance (as jlong), or 0 on failure
 */
JNIEXPORT jlong JNICALL
Java_app_rive_mp_RiveFile_nativeGetArtboardByName(
    JNIEnv* env,
    jobject thiz,
    jlong filePtr,
    jstring name
) {
    if (filePtr == 0) {
        rive_mp::ThrowRiveException(env, "Invalid file pointer (null)");
        return 0;
    }
    
    if (name == nullptr) {
        rive_mp::ThrowRiveException(env, "Artboard name cannot be null");
        return 0;
    }
    
    auto* file = reinterpret_cast<rive::File*>(filePtr);
    std::string artboardName = rive_mp::JStringToStdString(env, name);
    
    char msg[256];
    snprintf(msg, sizeof(msg), "nativeGetArtboardByName called (name: %s)", artboardName.c_str());
    LOGD(msg);
    
    // Get artboard instance by name (returns raw pointer, need to manage lifetime)
    rive::Artboard* artboard = file->artboard(artboardName);
    
    if (!artboard) {
        char errorMsg[256];
        snprintf(errorMsg, sizeof(errorMsg),
                "Artboard not found: %s", artboardName.c_str());
        LOGE(errorMsg);
        rive_mp::ThrowRiveException(env, errorMsg);
        return 0;
    }
    
    snprintf(msg, sizeof(msg), "Artboard retrieved successfully (name: %s)", artboardName.c_str());
    LOGD(msg);
    
    // Return pointer as jlong (artboard is already a raw pointer)
    return reinterpret_cast<jlong>(artboard);
}

/**
 * Get the default artboard (index 0 or first artboard).
 * 
 * Java/Kotlin signature:
 *   package app.rive.mp
 *   class RiveFile {
 *       external fun nativeGetDefaultArtboard(filePtr: Long): Long
 *   }
 * 
 * @param env JNI environment
 * @param thiz RiveFile instance
 * @param filePtr Native pointer to rive::File
 * @return Native pointer to rive::ArtboardInstance (as jlong), or 0 on failure
 */
JNIEXPORT jlong JNICALL
Java_app_rive_mp_RiveFile_nativeGetDefaultArtboard(
    JNIEnv* env,
    jobject thiz,
    jlong filePtr
) {
    LOGD("nativeGetDefaultArtboard called");
    
    if (filePtr == 0) {
        rive_mp::ThrowRiveException(env, "Invalid file pointer (null)");
        return 0;
    }
    
    auto* file = reinterpret_cast<rive::File*>(filePtr);
    
    // Get default artboard (returns unique_ptr)
    std::unique_ptr<rive::ArtboardInstance> artboard = file->artboardDefault();
    
    if (!artboard) {
        LOGE("Failed to get default artboard");
        rive_mp::ThrowRiveException(env, "Failed to get default artboard");
        return 0;
    }
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Default artboard retrieved successfully (name: %s)", artboard->name().c_str());
    LOGD(msg);
    
    // Return pointer as jlong
    return reinterpret_cast<jlong>(artboard.release());
}

/**
 * Release a Rive file and free its resources.
 * 
 * Java/Kotlin signature:
 *   package app.rive.mp
 *   class RiveFile {
 *       external fun nativeRelease(filePtr: Long)
 *   }
 * 
 * @param env JNI environment
 * @param thiz RiveFile instance
 * @param filePtr Native pointer to rive::File
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_RiveFile_nativeRelease(
    JNIEnv* env,
    jobject thiz,
    jlong filePtr
) {
    LOGD("nativeRelease called");
    
    if (filePtr == 0) {
        LOGW("Attempted to release null file pointer");
        return;
    }
    
    auto* file = reinterpret_cast<rive::File*>(filePtr);
    delete file;
    
    LOGD("Rive file released successfully");
}

} // extern "C"
