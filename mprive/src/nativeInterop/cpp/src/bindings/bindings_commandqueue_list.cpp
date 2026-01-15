#include "bindings_commandqueue_internal.hpp"

extern "C" {

// =============================================================================
// Phase D.5: List Operations JNI Bindings
// =============================================================================

/**
 * Gets the size of a list property.
 *
 * JNI signature: cppGetListSize(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppGetListSize(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong vmiHandle,
    jstring propertyPath
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to get list size on null CommandServer");
        return;
    }

    const char* pathChars = env->GetStringUTFChars(propertyPath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(propertyPath, pathChars);

    server->getListSize(static_cast<int64_t>(requestID), static_cast<int64_t>(vmiHandle), path);
}

/**
 * Gets an item from a list property by index.
 *
 * JNI signature: cppGetListItem(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String, index: Int): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppGetListItem(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong vmiHandle,
    jstring propertyPath,
    jint index
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to get list item on null CommandServer");
        return;
    }

    const char* pathChars = env->GetStringUTFChars(propertyPath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(propertyPath, pathChars);

    server->getListItem(static_cast<int64_t>(requestID), static_cast<int64_t>(vmiHandle), path, static_cast<int32_t>(index));
}

/**
 * Adds an item to the end of a list property.
 *
 * JNI signature: cppAddListItem(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String, itemHandle: Long): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppAddListItem(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong vmiHandle,
    jstring propertyPath,
    jlong itemHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to add list item on null CommandServer");
        return;
    }

    const char* pathChars = env->GetStringUTFChars(propertyPath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(propertyPath, pathChars);

    server->addListItem(static_cast<int64_t>(requestID), static_cast<int64_t>(vmiHandle), path, static_cast<int64_t>(itemHandle));
}

/**
 * Adds an item at a specific index in a list property.
 *
 * JNI signature: cppAddListItemAt(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String, index: Int, itemHandle: Long): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppAddListItemAt(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong vmiHandle,
    jstring propertyPath,
    jint index,
    jlong itemHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to add list item at index on null CommandServer");
        return;
    }

    const char* pathChars = env->GetStringUTFChars(propertyPath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(propertyPath, pathChars);

    server->addListItemAt(static_cast<int64_t>(requestID), static_cast<int64_t>(vmiHandle), path, static_cast<int32_t>(index), static_cast<int64_t>(itemHandle));
}

/**
 * Removes an item from a list property by its handle.
 *
 * JNI signature: cppRemoveListItem(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String, itemHandle: Long): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppRemoveListItem(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong vmiHandle,
    jstring propertyPath,
    jlong itemHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to remove list item on null CommandServer");
        return;
    }

    const char* pathChars = env->GetStringUTFChars(propertyPath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(propertyPath, pathChars);

    server->removeListItem(static_cast<int64_t>(requestID), static_cast<int64_t>(vmiHandle), path, static_cast<int64_t>(itemHandle));
}

/**
 * Removes an item from a list property by index.
 *
 * JNI signature: cppRemoveListItemAt(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String, index: Int): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppRemoveListItemAt(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong vmiHandle,
    jstring propertyPath,
    jint index
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to remove list item at index on null CommandServer");
        return;
    }

    const char* pathChars = env->GetStringUTFChars(propertyPath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(propertyPath, pathChars);

    server->removeListItemAt(static_cast<int64_t>(requestID), static_cast<int64_t>(vmiHandle), path, static_cast<int32_t>(index));
}

/**
 * Swaps two items in a list property.
 *
 * JNI signature: cppSwapListItems(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String, indexA: Int, indexB: Int): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppSwapListItems(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong vmiHandle,
    jstring propertyPath,
    jint indexA,
    jint indexB
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to swap list items on null CommandServer");
        return;
    }

    const char* pathChars = env->GetStringUTFChars(propertyPath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(propertyPath, pathChars);

    server->swapListItems(static_cast<int64_t>(requestID), static_cast<int64_t>(vmiHandle), path, static_cast<int32_t>(indexA), static_cast<int32_t>(indexB));
}

// =============================================================================
// Phase D.5: Nested VMI Operations JNI Bindings
// =============================================================================

/**
 * Gets a nested ViewModelInstance property.
 *
 * JNI signature: cppGetInstanceProperty(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppGetInstanceProperty(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong vmiHandle,
    jstring propertyPath
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to get instance property on null CommandServer");
        return;
    }

    const char* pathChars = env->GetStringUTFChars(propertyPath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(propertyPath, pathChars);

    server->getInstanceProperty(static_cast<int64_t>(requestID), static_cast<int64_t>(vmiHandle), path);
}

/**
 * Sets a nested ViewModelInstance property.
 *
 * JNI signature: cppSetInstanceProperty(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String, nestedHandle: Long): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppSetInstanceProperty(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong vmiHandle,
    jstring propertyPath,
    jlong nestedHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to set instance property on null CommandServer");
        return;
    }

    const char* pathChars = env->GetStringUTFChars(propertyPath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(propertyPath, pathChars);

    server->setInstanceProperty(static_cast<int64_t>(requestID), static_cast<int64_t>(vmiHandle), path, static_cast<int64_t>(nestedHandle));
}

// =============================================================================
// Phase D.5: Asset Property Operations JNI Bindings
// =============================================================================

/**
 * Sets an image property on a ViewModelInstance.
 *
 * JNI signature: cppSetImageProperty(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String, imageHandle: Long): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppSetImageProperty(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong vmiHandle,
    jstring propertyPath,
    jlong imageHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to set image property on null CommandServer");
        return;
    }

    const char* pathChars = env->GetStringUTFChars(propertyPath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(propertyPath, pathChars);

    server->setImageProperty(static_cast<int64_t>(requestID), static_cast<int64_t>(vmiHandle), path, static_cast<int64_t>(imageHandle));
}

/**
 * Sets an artboard property on a ViewModelInstance.
 *
 * JNI signature: cppSetArtboardProperty(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String, fileHandle: Long, artboardHandle: Long): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppSetArtboardProperty(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong vmiHandle,
    jstring propertyPath,
    jlong fileHandle,
    jlong artboardHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to set artboard property on null CommandServer");
        return;
    }

    const char* pathChars = env->GetStringUTFChars(propertyPath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(propertyPath, pathChars);

    server->setArtboardProperty(static_cast<int64_t>(requestID), static_cast<int64_t>(vmiHandle), path, static_cast<int64_t>(fileHandle), static_cast<int64_t>(artboardHandle));
}

} // extern "C"