#include "bindings_commandqueue_internal.hpp"

extern "C" {

// =============================================================================
// Phase D.1: ViewModelInstance Creation
// =============================================================================

/**
 * Creates a blank ViewModelInstance from a named ViewModel.
 *
 * JNI signature: cppCreateBlankVMI(ptr: Long, requestID: Long, fileHandle: Long, viewModelName: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppCreateBlankVMI(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong fileHandle,
    jstring viewModelName
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to create blank VMI on null CommandServer");
        return;
    }

    const char* nameChars = env->GetStringUTFChars(viewModelName, nullptr);
    std::string vmName(nameChars);
    env->ReleaseStringUTFChars(viewModelName, nameChars);

    server->createBlankVMI(static_cast<int64_t>(requestID), static_cast<int64_t>(fileHandle), vmName);
}

/**
 * Creates a default ViewModelInstance from a named ViewModel.
 *
 * JNI signature: cppCreateDefaultVMI(ptr: Long, requestID: Long, fileHandle: Long, viewModelName: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppCreateDefaultVMI(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong fileHandle,
    jstring viewModelName
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to create default VMI on null CommandServer");
        return;
    }

    const char* nameChars = env->GetStringUTFChars(viewModelName, nullptr);
    std::string vmName(nameChars);
    env->ReleaseStringUTFChars(viewModelName, nameChars);

    server->createDefaultVMI(static_cast<int64_t>(requestID), static_cast<int64_t>(fileHandle), vmName);
}

/**
 * Creates a named ViewModelInstance from a named ViewModel.
 *
 * JNI signature: cppCreateNamedVMI(ptr: Long, requestID: Long, fileHandle: Long, viewModelName: String, instanceName: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppCreateNamedVMI(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong fileHandle,
    jstring viewModelName,
    jstring instanceName
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to create named VMI on null CommandServer");
        return;
    }

    const char* vmNameChars = env->GetStringUTFChars(viewModelName, nullptr);
    std::string vmName(vmNameChars);
    env->ReleaseStringUTFChars(viewModelName, vmNameChars);

    const char* instNameChars = env->GetStringUTFChars(instanceName, nullptr);
    std::string instName(instNameChars);
    env->ReleaseStringUTFChars(instanceName, instNameChars);

    server->createNamedVMI(static_cast<int64_t>(requestID), static_cast<int64_t>(fileHandle), vmName, instName);
}

/**
 * Deletes a ViewModelInstance.
 *
 * JNI signature: cppDeleteVMI(ptr: Long, requestID: Long, vmiHandle: Long): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppDeleteVMI(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong vmiHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to delete VMI on null CommandServer");
        return;
    }

    server->deleteVMI(static_cast<int64_t>(requestID), static_cast<int64_t>(vmiHandle));
}

// =============================================================================
// Phase D.2: Property Operations
// =============================================================================

/**
 * Gets a number property value from a ViewModelInstance.
 *
 * JNI signature: cppGetNumberProperty(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppGetNumberProperty(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong vmiHandle,
    jstring propertyPath
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to get number property on null CommandServer");
        return;
    }

    const char* pathChars = env->GetStringUTFChars(propertyPath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(propertyPath, pathChars);

    server->getNumberProperty(static_cast<int64_t>(requestID), static_cast<int64_t>(vmiHandle), path);
}

/**
 * Sets a number property value on a ViewModelInstance.
 *
 * JNI signature matches reference: cppSetNumberProperty(ptr: Long, vmiHandle: Long, propertyPath: String, value: Float): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppSetNumberProperty(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong vmiHandle,
    jstring propertyPath,
    jfloat value
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to set number property on null CommandServer");
        return;
    }

    const char* pathChars = env->GetStringUTFChars(propertyPath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(propertyPath, pathChars);

    // Fire-and-forget - pass 0 for requestID (no async callback needed)
    server->setNumberProperty(0L, static_cast<int64_t>(vmiHandle), path, static_cast<float>(value));
}

/**
 * Gets a string property value from a ViewModelInstance.
 *
 * JNI signature: cppGetStringProperty(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppGetStringProperty(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong vmiHandle,
    jstring propertyPath
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to get string property on null CommandServer");
        return;
    }

    const char* pathChars = env->GetStringUTFChars(propertyPath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(propertyPath, pathChars);

    server->getStringProperty(static_cast<int64_t>(requestID), static_cast<int64_t>(vmiHandle), path);
}

/**
 * Sets a string property value on a ViewModelInstance.
 *
 * JNI signature matches reference: cppSetStringProperty(ptr: Long, vmiHandle: Long, propertyPath: String, value: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppSetStringProperty(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong vmiHandle,
    jstring propertyPath,
    jstring value
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to set string property on null CommandServer");
        return;
    }

    const char* pathChars = env->GetStringUTFChars(propertyPath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(propertyPath, pathChars);

    const char* valueChars = env->GetStringUTFChars(value, nullptr);
    std::string valueStr(valueChars);
    env->ReleaseStringUTFChars(value, valueChars);

    // Fire-and-forget - pass 0 for requestID (no async callback needed)
    server->setStringProperty(0L, static_cast<int64_t>(vmiHandle), path, valueStr);
}

/**
 * Gets a boolean property value from a ViewModelInstance.
 *
 * JNI signature: cppGetBooleanProperty(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppGetBooleanProperty(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong vmiHandle,
    jstring propertyPath
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to get boolean property on null CommandServer");
        return;
    }

    const char* pathChars = env->GetStringUTFChars(propertyPath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(propertyPath, pathChars);

    server->getBooleanProperty(static_cast<int64_t>(requestID), static_cast<int64_t>(vmiHandle), path);
}

/**
 * Sets a boolean property value on a ViewModelInstance.
 *
 * JNI signature matches reference: cppSetBooleanProperty(ptr: Long, vmiHandle: Long, propertyPath: String, value: Boolean): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppSetBooleanProperty(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong vmiHandle,
    jstring propertyPath,
    jboolean value
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to set boolean property on null CommandServer");
        return;
    }

    const char* pathChars = env->GetStringUTFChars(propertyPath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(propertyPath, pathChars);

    // Fire-and-forget - pass 0 for requestID (no async callback needed)
    server->setBooleanProperty(0L, static_cast<int64_t>(vmiHandle), path, static_cast<bool>(value));
}

// =============================================================================
// Phase D.3: Additional Property Types (enum, color, trigger)
// =============================================================================

/**
 * Gets an enum property value from a ViewModelInstance.
 *
 * JNI signature: cppGetEnumProperty(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppGetEnumProperty(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong vmiHandle,
    jstring propertyPath
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to get enum property on null CommandServer");
        return;
    }

    const char* pathChars = env->GetStringUTFChars(propertyPath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(propertyPath, pathChars);

    server->getEnumProperty(static_cast<int64_t>(requestID), static_cast<int64_t>(vmiHandle), path);
}

/**
 * Sets an enum property value on a ViewModelInstance.
 *
 * JNI signature matches reference: cppSetEnumProperty(ptr: Long, vmiHandle: Long, propertyPath: String, value: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppSetEnumProperty(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong vmiHandle,
    jstring propertyPath,
    jstring value
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to set enum property on null CommandServer");
        return;
    }

    const char* pathChars = env->GetStringUTFChars(propertyPath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(propertyPath, pathChars);

    const char* valueChars = env->GetStringUTFChars(value, nullptr);
    std::string valueStr(valueChars);
    env->ReleaseStringUTFChars(value, valueChars);

    // Fire-and-forget - pass 0 for requestID (no async callback needed)
    server->setEnumProperty(0L, static_cast<int64_t>(vmiHandle), path, valueStr);
}

/**
 * Gets a color property value from a ViewModelInstance.
 *
 * JNI signature: cppGetColorProperty(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppGetColorProperty(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong vmiHandle,
    jstring propertyPath
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to get color property on null CommandServer");
        return;
    }

    const char* pathChars = env->GetStringUTFChars(propertyPath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(propertyPath, pathChars);

    server->getColorProperty(static_cast<int64_t>(requestID), static_cast<int64_t>(vmiHandle), path);
}

/**
 * Sets a color property value on a ViewModelInstance.
 *
 * JNI signature matches reference: cppSetColorProperty(ptr: Long, vmiHandle: Long, propertyPath: String, value: Int): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppSetColorProperty(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong vmiHandle,
    jstring propertyPath,
    jint value
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to set color property on null CommandServer");
        return;
    }

    const char* pathChars = env->GetStringUTFChars(propertyPath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(propertyPath, pathChars);

    // Fire-and-forget - pass 0 for requestID (no async callback needed)
    server->setColorProperty(0L, static_cast<int64_t>(vmiHandle), path, static_cast<int32_t>(value));
}

/**
 * Fires a trigger property on a ViewModelInstance.
 *
 * JNI signature matches reference: cppFireTriggerProperty(ptr: Long, vmiHandle: Long, propertyPath: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppFireTriggerProperty(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong vmiHandle,
    jstring propertyPath
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to fire trigger property on null CommandServer");
        return;
    }

    const char* pathChars = env->GetStringUTFChars(propertyPath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(propertyPath, pathChars);

    // Fire-and-forget - pass 0 for requestID (no async callback needed)
    server->fireTriggerProperty(0L, static_cast<int64_t>(vmiHandle), path);
}

// =============================================================================
// Phase D.4: Property Subscriptions
// =============================================================================

/**
 * Subscribes to property updates on a ViewModelInstance.
 *
 * JNI signature: cppSubscribeToProperty(ptr: Long, vmiHandle: Long, propertyPath: String, propertyType: Int): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppSubscribeToProperty(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong vmiHandle,
    jstring propertyPath,
    jint propertyType
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to subscribe to property on null CommandServer");
        return;
    }

    const char* pathChars = env->GetStringUTFChars(propertyPath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(propertyPath, pathChars);

    server->subscribeToProperty(static_cast<int64_t>(vmiHandle), path, static_cast<int32_t>(propertyType));
}

/**
 * Unsubscribes from property updates on a ViewModelInstance.
 *
 * JNI signature: cppUnsubscribeFromProperty(ptr: Long, vmiHandle: Long, propertyPath: String, propertyType: Int): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppUnsubscribeFromProperty(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong vmiHandle,
    jstring propertyPath,
    jint propertyType
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to unsubscribe from property on null CommandServer");
        return;
    }

    const char* pathChars = env->GetStringUTFChars(propertyPath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(propertyPath, pathChars);

    server->unsubscribeFromProperty(static_cast<int64_t>(vmiHandle), path, static_cast<int32_t>(propertyType));
}

// =============================================================================
// Phase D.6: VMI Binding to State Machine
// =============================================================================

/**
 * Binds a ViewModelInstance to a StateMachine for data binding.
 *
 * JNI signature: cppBindViewModelInstance(ptr: Long, requestID: Long, smHandle: Long, vmiHandle: Long): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppBindViewModelInstance(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong smHandle,
    jlong vmiHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to bind VMI on null CommandServer");
        return;
    }

    server->bindViewModelInstance(static_cast<int64_t>(requestID), static_cast<int64_t>(smHandle), static_cast<int64_t>(vmiHandle));
}

/**
 * Gets the default ViewModelInstance for an artboard from its file.
 *
 * JNI signature: cppGetDefaultViewModelInstance(ptr: Long, requestID: Long, fileHandle: Long, artboardHandle: Long): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppGetDefaultViewModelInstance(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong fileHandle,
    jlong artboardHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to get default VMI on null CommandServer");
        return;
    }

    server->getDefaultViewModelInstance(static_cast<int64_t>(requestID), static_cast<int64_t>(fileHandle), static_cast<int64_t>(artboardHandle));
}

} // extern "C"