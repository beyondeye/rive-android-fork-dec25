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
 * JNI signature matches reference: cppDeleteImage(ptr: Long, imageHandle: Long): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppDeleteImage(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong imageHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to delete image on null CommandServer");
        return;
    }

    server->deleteImage(static_cast<int64_t>(imageHandle));
}

/**
 * Registers an image asset with a name for use in Rive files.
 *
 * JNI signature matches reference: cppRegisterImage(ptr: Long, name: String, imageHandle: Long): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppRegisterImage(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
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

    server->registerImage(name, static_cast<int64_t>(imageHandle));
}

/**
 * Unregisters an image asset by name.
 *
 * JNI signature matches reference: cppUnregisterImage(ptr: Long, name: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppUnregisterImage(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
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
 * JNI signature matches reference: cppDeleteAudio(ptr: Long, audioHandle: Long): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppDeleteAudio(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong audioHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to delete audio on null CommandServer");
        return;
    }

    server->deleteAudio(static_cast<int64_t>(audioHandle));
}

/**
 * Registers an audio asset with a name for use in Rive files.
 *
 * JNI signature matches reference: cppRegisterAudio(ptr: Long, name: String, audioHandle: Long): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppRegisterAudio(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
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

    server->registerAudio(name, static_cast<int64_t>(audioHandle));
}

/**
 * Unregisters an audio asset by name.
 *
 * JNI signature matches reference: cppUnregisterAudio(ptr: Long, name: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppUnregisterAudio(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
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
 * JNI signature matches reference: cppDeleteFont(ptr: Long, fontHandle: Long): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppDeleteFont(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong fontHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to delete font on null CommandServer");
        return;
    }

    server->deleteFont(static_cast<int64_t>(fontHandle));
}

/**
 * Registers a font asset with a name for use in Rive files.
 *
 * JNI signature matches reference: cppRegisterFont(ptr: Long, name: String, fontHandle: Long): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppRegisterFont(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
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

    server->registerFont(name, static_cast<int64_t>(fontHandle));
}

/**
 * Unregisters a font asset by name.
 *
 * JNI signature matches reference: cppUnregisterFont(ptr: Long, name: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppUnregisterFont(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
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

    server->unregisterFont(name);
}

} // extern "C"