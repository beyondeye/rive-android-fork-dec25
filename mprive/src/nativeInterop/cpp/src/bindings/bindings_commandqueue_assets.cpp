#include "bindings_commandqueue_internal.hpp"

extern "C" {

// =============================================================================
// Phase E.1: Asset Management Operations
// =============================================================================

/**
 * Decodes an image from bytes.
 *
 * JNI signature: cppDecodeImage(ptr: Long, requestID: Long, bytes: ByteArray): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppDecodeImage(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jbyteArray bytes
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to decode image on null CommandServer");
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

    server->decodeImage(static_cast<int64_t>(requestID), byteVector);
}

/**
 * Deletes a decoded image.
 *
 * JNI signature: cppDeleteImage(ptr: Long, requestID: Long, imageHandle: Long): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppDeleteImage(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong imageHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to delete image on null CommandServer");
        return;
    }

    // Note: requestID is ignored - deleteImage is fire-and-forget
    server->deleteImage(static_cast<int64_t>(imageHandle));
}

/**
 * Registers an image asset with a file.
 *
 * JNI signature: cppRegisterImage(ptr: Long, requestID: Long, fileHandle: Long, assetName: String, imageHandle: Long): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppRegisterImage(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong fileHandle,
    jstring assetName,
    jlong imageHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to register image on null CommandServer");
        return;
    }

    const char* nameChars = env->GetStringUTFChars(assetName, nullptr);
    std::string name(nameChars);
    env->ReleaseStringUTFChars(assetName, nameChars);

    // Note: requestID and fileHandle are ignored - registerImage only needs name and handle
    server->registerImage(name, static_cast<int64_t>(imageHandle));
}

/**
 * Unregisters an image asset from a file.
 *
 * JNI signature: cppUnregisterImage(ptr: Long, requestID: Long, fileHandle: Long, assetName: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppUnregisterImage(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong fileHandle,
    jstring assetName
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to unregister image on null CommandServer");
        return;
    }

    const char* nameChars = env->GetStringUTFChars(assetName, nullptr);
    std::string name(nameChars);
    env->ReleaseStringUTFChars(assetName, nameChars);

    // Note: requestID and fileHandle are ignored - unregisterImage only needs name
    server->unregisterImage(name);
}

/**
 * Decodes an audio clip from bytes.
 *
 * JNI signature: cppDecodeAudio(ptr: Long, requestID: Long, bytes: ByteArray): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppDecodeAudio(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jbyteArray bytes
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to decode audio on null CommandServer");
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

    server->decodeAudio(static_cast<int64_t>(requestID), byteVector);
}

/**
 * Deletes a decoded audio clip.
 *
 * JNI signature: cppDeleteAudio(ptr: Long, requestID: Long, audioHandle: Long): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppDeleteAudio(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong audioHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to delete audio on null CommandServer");
        return;
    }

    // Note: requestID is ignored - deleteAudio is fire-and-forget
    server->deleteAudio(static_cast<int64_t>(audioHandle));
}

/**
 * Registers an audio asset with a file.
 *
 * JNI signature: cppRegisterAudio(ptr: Long, requestID: Long, fileHandle: Long, assetName: String, audioHandle: Long): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppRegisterAudio(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong fileHandle,
    jstring assetName,
    jlong audioHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to register audio on null CommandServer");
        return;
    }

    const char* nameChars = env->GetStringUTFChars(assetName, nullptr);
    std::string name(nameChars);
    env->ReleaseStringUTFChars(assetName, nameChars);

    // Note: requestID and fileHandle are ignored - registerAudio only needs name and handle
    server->registerAudio(name, static_cast<int64_t>(audioHandle));
}

/**
 * Unregisters an audio asset from a file.
 *
 * JNI signature: cppUnregisterAudio(ptr: Long, requestID: Long, fileHandle: Long, assetName: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppUnregisterAudio(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong fileHandle,
    jstring assetName
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to unregister audio on null CommandServer");
        return;
    }

    const char* nameChars = env->GetStringUTFChars(assetName, nullptr);
    std::string name(nameChars);
    env->ReleaseStringUTFChars(assetName, nameChars);

    // Note: requestID and fileHandle are ignored - unregisterAudio only needs name
    server->unregisterAudio(name);
}

/**
 * Decodes a font from bytes.
 *
 * JNI signature: cppDecodeFont(ptr: Long, requestID: Long, bytes: ByteArray): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppDecodeFont(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jbyteArray bytes
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to decode font on null CommandServer");
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

    server->decodeFont(static_cast<int64_t>(requestID), byteVector);
}

/**
 * Deletes a decoded font.
 *
 * JNI signature: cppDeleteFont(ptr: Long, requestID: Long, fontHandle: Long): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppDeleteFont(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong fontHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to delete font on null CommandServer");
        return;
    }

    // Note: requestID is ignored - deleteFont is fire-and-forget
    server->deleteFont(static_cast<int64_t>(fontHandle));
}

/**
 * Registers a font asset with a file.
 *
 * JNI signature: cppRegisterFont(ptr: Long, requestID: Long, fileHandle: Long, assetName: String, fontHandle: Long): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppRegisterFont(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong fileHandle,
    jstring assetName,
    jlong fontHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to register font on null CommandServer");
        return;
    }

    const char* nameChars = env->GetStringUTFChars(assetName, nullptr);
    std::string name(nameChars);
    env->ReleaseStringUTFChars(assetName, nameChars);

    // Note: requestID and fileHandle are ignored - registerFont only needs name and handle
    server->registerFont(name, static_cast<int64_t>(fontHandle));
}

/**
 * Unregisters a font asset from a file.
 *
 * JNI signature: cppUnregisterFont(ptr: Long, requestID: Long, fileHandle: Long, assetName: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppUnregisterFont(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong fileHandle,
    jstring assetName
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to unregister font on null CommandServer");
        return;
    }

    const char* nameChars = env->GetStringUTFChars(assetName, nullptr);
    std::string name(nameChars);
    env->ReleaseStringUTFChars(assetName, nameChars);

    // Note: requestID and fileHandle are ignored - unregisterFont only needs name
    server->unregisterFont(name);
}

} // extern "C"