#include "bindings_commandqueue_internal.hpp"

extern "C" {

/**
 * Creates the default state machine from an artboard.
 * 
 * JNI signature: cppCreateDefaultStateMachine(ptr: Long, requestID: Long, artboardHandle: Long): Unit
 * 
 * @param env The JNI environment.
 * @param thiz The Java CommandQueue object.
 * @param ptr The native pointer to the CommandServer.
 * @param requestID The request ID for async completion.
 * @param artboardHandle The handle of the artboard to create state machine from.
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppCreateDefaultStateMachine(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong artboardHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to create default state machine on null CommandServer");
        return;
    }
    
    server->createDefaultStateMachine(static_cast<int64_t>(requestID), static_cast<int64_t>(artboardHandle));
}

/**
 * Creates a state machine by name from an artboard.
 * 
 * JNI signature: cppCreateStateMachineByName(ptr: Long, requestID: Long, artboardHandle: Long, name: String): Unit
 * 
 * @param env The JNI environment.
 * @param thiz The Java CommandQueue object.
 * @param ptr The native pointer to the CommandServer.
 * @param requestID The request ID for async completion.
 * @param artboardHandle The handle of the artboard to create state machine from.
 * @param name The name of the state machine to create.
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppCreateStateMachineByName(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong artboardHandle,
    jstring name
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to create state machine by name on null CommandServer");
        return;
    }
    
    // Convert Java string to C++ string
    const char* nameChars = env->GetStringUTFChars(name, nullptr);
    std::string smName(nameChars);
    env->ReleaseStringUTFChars(name, nameChars);
    
    server->createStateMachineByName(static_cast<int64_t>(requestID), static_cast<int64_t>(artboardHandle), smName);
}

/**
 * Advances a state machine by a time delta.
 * 
 * JNI signature: cppAdvanceStateMachine(ptr: Long, smHandle: Long, deltaTimeNs: Long): Unit
 * 
 * This matches the kotlin/src/main reference implementation.
 * 
 * @param env The JNI environment.
 * @param thiz The Java CommandQueue object.
 * @param ptr The native pointer to the CommandServer.
 * @param smHandle The handle of the state machine to advance.
 * @param deltaTimeNs The time delta in nanoseconds.
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppAdvanceStateMachine(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong smHandle,
    jlong deltaTimeNs
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to advance state machine on null CommandServer");
        return;
    }
    
    // Convert nanoseconds to seconds for the server
    float deltaTimeSeconds = static_cast<float>(deltaTimeNs) / 1000000000.0f;
    server->advanceStateMachine(static_cast<int64_t>(smHandle), deltaTimeSeconds);
}

/**
 * Deletes a state machine.
 *
 * JNI signature: cppDeleteStateMachine(ptr: Long, requestID: Long, smHandle: Long): Unit
 *
 * @param env The JNI environment.
 * @param thiz The Java CommandQueue object.
 * @param ptr The native pointer to the CommandServer.
 * @param requestID The request ID for async completion.
 * @param smHandle The handle of the state machine to delete.
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppDeleteStateMachine(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong smHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to delete state machine on null CommandServer");
        return;
    }

    server->deleteStateMachine(static_cast<int64_t>(requestID), static_cast<int64_t>(smHandle));
}

// =============================================================================
// Phase C.4: State Machine Input Operations
// =============================================================================

/**
 * Gets the input count from a state machine.
 *
 * JNI signature: cppGetInputCount(ptr: Long, requestID: Long, smHandle: Long): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppGetInputCount(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong smHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to get input count on null CommandServer");
        return;
    }

    server->getInputCount(static_cast<int64_t>(requestID), static_cast<int64_t>(smHandle));
}

/**
 * Gets the input names from a state machine.
 *
 * JNI signature: cppGetInputNames(ptr: Long, requestID: Long, smHandle: Long): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppGetInputNames(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong smHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to get input names on null CommandServer");
        return;
    }

    server->getInputNames(static_cast<int64_t>(requestID), static_cast<int64_t>(smHandle));
}

/**
 * Gets input info (name and type) by index from a state machine.
 *
 * JNI signature: cppGetInputInfo(ptr: Long, requestID: Long, smHandle: Long, inputIndex: Int): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppGetInputInfo(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong smHandle,
    jint inputIndex
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to get input info on null CommandServer");
        return;
    }

    server->getInputInfo(static_cast<int64_t>(requestID), static_cast<int64_t>(smHandle), static_cast<int32_t>(inputIndex));
}

/**
 * Gets the value of a number input.
 *
 * JNI signature: cppGetNumberInput(ptr: Long, requestID: Long, smHandle: Long, inputName: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppGetNumberInput(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong smHandle,
    jstring inputName
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to get number input on null CommandServer");
        return;
    }

    const char* nameChars = env->GetStringUTFChars(inputName, nullptr);
    std::string name(nameChars);
    env->ReleaseStringUTFChars(inputName, nameChars);

    server->getNumberInput(static_cast<int64_t>(requestID), static_cast<int64_t>(smHandle), name);
}

/**
 * Sets the value of a number input.
 *
 * JNI signature: cppSetNumberInput(ptr: Long, requestID: Long, smHandle: Long, inputName: String, value: Float): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppSetNumberInput(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong smHandle,
    jstring inputName,
    jfloat value
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to set number input on null CommandServer");
        return;
    }

    const char* nameChars = env->GetStringUTFChars(inputName, nullptr);
    std::string name(nameChars);
    env->ReleaseStringUTFChars(inputName, nameChars);

    server->setNumberInput(static_cast<int64_t>(requestID), static_cast<int64_t>(smHandle), name, static_cast<float>(value));
}

/**
 * Gets the value of a boolean input.
 *
 * JNI signature: cppGetBooleanInput(ptr: Long, requestID: Long, smHandle: Long, inputName: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppGetBooleanInput(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong smHandle,
    jstring inputName
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to get boolean input on null CommandServer");
        return;
    }

    const char* nameChars = env->GetStringUTFChars(inputName, nullptr);
    std::string name(nameChars);
    env->ReleaseStringUTFChars(inputName, nameChars);

    server->getBooleanInput(static_cast<int64_t>(requestID), static_cast<int64_t>(smHandle), name);
}

/**
 * Sets the value of a boolean input.
 *
 * JNI signature: cppSetBooleanInput(ptr: Long, requestID: Long, smHandle: Long, inputName: String, value: Boolean): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppSetBooleanInput(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong smHandle,
    jstring inputName,
    jboolean value
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to set boolean input on null CommandServer");
        return;
    }

    const char* nameChars = env->GetStringUTFChars(inputName, nullptr);
    std::string name(nameChars);
    env->ReleaseStringUTFChars(inputName, nameChars);

    server->setBooleanInput(static_cast<int64_t>(requestID), static_cast<int64_t>(smHandle), name, static_cast<bool>(value));
}

/**
 * Fires a trigger input.
 *
 * JNI signature: cppFireTrigger(ptr: Long, requestID: Long, smHandle: Long, inputName: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppFireTrigger(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong smHandle,
    jstring inputName
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to fire trigger on null CommandServer");
        return;
    }

    const char* nameChars = env->GetStringUTFChars(inputName, nullptr);
    std::string name(nameChars);
    env->ReleaseStringUTFChars(inputName, nameChars);

    server->fireTrigger(static_cast<int64_t>(requestID), static_cast<int64_t>(smHandle), name);
}

// =============================================================================
// Phase 0.3: State Machine Input Manipulation (SMI - fire-and-forget)
// =============================================================================

/**
 * Sets a number input on a state machine (fire-and-forget).
 * This is a simplified API for RiveSprite that doesn't wait for confirmation.
 *
 * JNI signature: cppSetStateMachineNumberInput(ptr: Long, smHandle: Long, inputName: String, value: Float): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppSetStateMachineNumberInput(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong smHandle,
    jstring inputName,
    jfloat value
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to set SM number input on null CommandServer");
        return;
    }

    const char* nameChars = env->GetStringUTFChars(inputName, nullptr);
    std::string name(nameChars);
    env->ReleaseStringUTFChars(inputName, nameChars);

    // Fire-and-forget: use 0 as requestID
    server->setNumberInput(0L, static_cast<int64_t>(smHandle), name, static_cast<float>(value));
}

/**
 * Sets a boolean input on a state machine (fire-and-forget).
 * This is a simplified API for RiveSprite that doesn't wait for confirmation.
 *
 * JNI signature: cppSetStateMachineBooleanInput(ptr: Long, smHandle: Long, inputName: String, value: Boolean): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppSetStateMachineBooleanInput(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong smHandle,
    jstring inputName,
    jboolean value
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to set SM boolean input on null CommandServer");
        return;
    }

    const char* nameChars = env->GetStringUTFChars(inputName, nullptr);
    std::string name(nameChars);
    env->ReleaseStringUTFChars(inputName, nameChars);

    // Fire-and-forget: use 0 as requestID
    server->setBooleanInput(0L, static_cast<int64_t>(smHandle), name, static_cast<bool>(value));
}

/**
 * Fires a trigger input on a state machine (fire-and-forget).
 * This is a simplified API for RiveSprite that doesn't wait for confirmation.
 *
 * JNI signature: cppFireStateMachineTrigger(ptr: Long, smHandle: Long, inputName: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppFireStateMachineTrigger(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong smHandle,
    jstring inputName
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to fire SM trigger on null CommandServer");
        return;
    }

    const char* nameChars = env->GetStringUTFChars(inputName, nullptr);
    std::string name(nameChars);
    env->ReleaseStringUTFChars(inputName, nameChars);

    // Fire-and-forget: use 0 as requestID
    server->fireTrigger(0L, static_cast<int64_t>(smHandle), name);
}

} // extern "C"