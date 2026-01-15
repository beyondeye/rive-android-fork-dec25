#include "bindings_commandqueue_internal.hpp"

extern "C" {

/**
 * Loads a Rive file from bytes.
 * 
 * JNI signature: cppLoadFile(ptr: Long, requestID: Long, bytes: ByteArray): Unit
 * 
 * @param env The JNI environment.
 * @param thiz The Java CommandQueue object.
 * @param ptr The native pointer to the CommandServer.
 * @param requestID The request ID for async completion.
 * @param bytes The Rive file bytes.
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppLoadFile(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jbyteArray bytes
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to load file on null CommandServer");
        return;
    }
    
    // Convert Java byte array to std::vector<uint8_t>
    jsize length = env->GetArrayLength(bytes);
    jbyte* byteElements = env->GetByteArrayElements(bytes, nullptr);
    
    std::vector<uint8_t> byteVector(
        reinterpret_cast<uint8_t*>(byteElements),
        reinterpret_cast<uint8_t*>(byteElements) + length
    );
    
    env->ReleaseByteArrayElements(bytes, byteElements, JNI_ABORT);
    
    // Enqueue the load file command
    server->loadFile(static_cast<int64_t>(requestID), byteVector);
}

/**
 * Deletes a Rive file.
 * 
 * JNI signature: cppDeleteFile(ptr: Long, requestID: Long, fileHandle: Long): Unit
 * 
 * @param env The JNI environment.
 * @param thiz The Java CommandQueue object.
 * @param ptr The native pointer to the CommandServer.
 * @param requestID The request ID for async completion.
 * @param fileHandle The handle of the file to delete.
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppDeleteFile(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong fileHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to delete file on null CommandServer");
        return;
    }
    
    // Enqueue the delete file command
    server->deleteFile(static_cast<int64_t>(requestID), static_cast<int64_t>(fileHandle));
}

/**
 * Gets artboard names from a file.
 * 
 * JNI signature: cppGetArtboardNames(ptr: Long, requestID: Long, fileHandle: Long): Unit
 * 
 * @param env The JNI environment.
 * @param thiz The Java CommandQueue object.
 * @param ptr The native pointer to the CommandServer.
 * @param requestID The request ID for async completion.
 * @param fileHandle The handle of the file to query.
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppGetArtboardNames(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong fileHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to get artboard names on null CommandServer");
        return;
    }
    
    server->getArtboardNames(static_cast<int64_t>(requestID), static_cast<int64_t>(fileHandle));
}

/**
 * Gets state machine names from an artboard.
 * 
 * JNI signature: cppGetStateMachineNames(ptr: Long, requestID: Long, artboardHandle: Long): Unit
 * 
 * @param env The JNI environment.
 * @param thiz The Java CommandQueue object.
 * @param ptr The native pointer to the CommandServer.
 * @param requestID The request ID for async completion.
 * @param artboardHandle The handle of the artboard to query.
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppGetStateMachineNames(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong artboardHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to get state machine names on null CommandServer");
        return;
    }
    
    server->getStateMachineNames(static_cast<int64_t>(requestID), static_cast<int64_t>(artboardHandle));
}

/**
 * Gets view model names from a file.
 * 
 * JNI signature: cppGetViewModelNames(ptr: Long, requestID: Long, fileHandle: Long): Unit
 * 
 * @param env The JNI environment.
 * @param thiz The Java CommandQueue object.
 * @param ptr The native pointer to the CommandServer.
 * @param requestID The request ID for async completion.
 * @param fileHandle The handle of the file to query.
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppGetViewModelNames(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong fileHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to get view model names on null CommandServer");
        return;
    }
    
    server->getViewModelNames(static_cast<int64_t>(requestID), static_cast<int64_t>(fileHandle));
}

/**
 * Creates the default artboard from a file (SYNCHRONOUS).
 * 
 * JNI signature: cppCreateDefaultArtboard(ptr: Long, requestID: Long, fileHandle: Long): Long
 * 
 * @param env The JNI environment.
 * @param thiz The Java CommandQueue object.
 * @param ptr The native pointer to the CommandServer.
 * @param requestID The request ID (unused for sync operations).
 * @param fileHandle The handle of the file to create artboard from.
 * @return The artboard handle, or 0 if creation failed.
 */
JNIEXPORT jlong JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppCreateDefaultArtboard(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong fileHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to create default artboard on null CommandServer");
        return 0;
    }
    
    return static_cast<jlong>(server->createDefaultArtboardSync(static_cast<int64_t>(fileHandle)));
}

/**
 * Creates an artboard by name from a file (SYNCHRONOUS).
 * 
 * JNI signature: cppCreateArtboardByName(ptr: Long, requestID: Long, fileHandle: Long, name: String): Long
 * 
 * @param env The JNI environment.
 * @param thiz The Java CommandQueue object.
 * @param ptr The native pointer to the CommandServer.
 * @param requestID The request ID (unused for sync operations).
 * @param fileHandle The handle of the file to create artboard from.
 * @param name The name of the artboard to create.
 * @return The artboard handle, or 0 if creation failed.
 */
JNIEXPORT jlong JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppCreateArtboardByName(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong fileHandle,
    jstring name
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to create artboard by name on null CommandServer");
        return 0;
    }
    
    // Convert Java string to C++ string
    const char* nameChars = env->GetStringUTFChars(name, nullptr);
    std::string artboardName(nameChars);
    env->ReleaseStringUTFChars(name, nameChars);
    
    return static_cast<jlong>(server->createArtboardByNameSync(static_cast<int64_t>(fileHandle), artboardName));
}

/**
 * Deletes an artboard.
 * 
 * JNI signature: cppDeleteArtboard(ptr: Long, requestID: Long, artboardHandle: Long): Unit
 * 
 * @param env The JNI environment.
 * @param thiz The Java CommandQueue object.
 * @param ptr The native pointer to the CommandServer.
 * @param requestID The request ID for async completion.
 * @param artboardHandle The handle of the artboard to delete.
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppDeleteArtboard(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong artboardHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to delete artboard on null CommandServer");
        return;
    }
    
    server->deleteArtboard(static_cast<int64_t>(requestID), static_cast<int64_t>(artboardHandle));
}

// =============================================================================
// Phase E.3: Artboard Resizing
// =============================================================================

/**
 * Resizes an artboard to match given dimensions.
 * Required for Fit.Layout mode where the artboard must match surface dimensions.
 * 
 * JNI signature: cppResizeArtboard(ptr: Long, artboardHandle: Long, width: Int, height: Int, scaleFactor: Float): Unit
 * 
 * @param env The JNI environment.
 * @param thiz The Java CommandQueue object.
 * @param ptr The native pointer to the CommandServer.
 * @param artboardHandle The handle of the artboard to resize.
 * @param width The new width in pixels.
 * @param height The new height in pixels.
 * @param scaleFactor Scale factor for high DPI displays.
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppResizeArtboard(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong artboardHandle,
    jint width,
    jint height,
    jfloat scaleFactor
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to resize artboard on null CommandServer");
        return;
    }
    
    server->resizeArtboard(
        static_cast<int64_t>(artboardHandle),
        static_cast<int32_t>(width),
        static_cast<int32_t>(height),
        static_cast<float>(scaleFactor)
    );
}

/**
 * Resets an artboard to its original dimensions defined in the Rive file.
 * 
 * JNI signature: cppResetArtboardSize(ptr: Long, artboardHandle: Long): Unit
 * 
 * @param env The JNI environment.
 * @param thiz The Java CommandQueue object.
 * @param ptr The native pointer to the CommandServer.
 * @param artboardHandle The handle of the artboard to reset.
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppResetArtboardSize(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong artboardHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to reset artboard size on null CommandServer");
        return;
    }
    
    server->resetArtboardSize(static_cast<int64_t>(artboardHandle));
}

} // extern "C"