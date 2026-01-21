/**
 * JNI bindings for linear animation operations.
 * 
 * These bindings provide Kotlin access to linear animation functionality,
 * enabling rive-android parity where BOTH linear animations AND state machines
 * can be played.
 */

#include "bindings_commandqueue_internal.hpp"

extern "C" {

/**
 * Creates the default (first) animation from an artboard.
 *
 * JNI signature: cppCreateDefaultAnimation(ptr: Long, artboardHandle: Long): Long
 *
 * @return The animation handle, or 0 if creation failed.
 */
JNIEXPORT jlong JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppCreateDefaultAnimation(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong artboardHandle)
{
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to create default animation on null CommandServer");
        return 0;
    }
    
    return static_cast<jlong>(server->createDefaultAnimationSync(static_cast<int64_t>(artboardHandle)));
}

/**
 * Creates an animation by name from an artboard.
 *
 * JNI signature: cppCreateAnimationByName(ptr: Long, artboardHandle: Long, name: String): Long
 *
 * @return The animation handle, or 0 if creation failed.
 */
JNIEXPORT jlong JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppCreateAnimationByName(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong artboardHandle,
    jstring name)
{
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to create animation by name on null CommandServer");
        return 0;
    }
    
    const char* nameChars = env->GetStringUTFChars(name, nullptr);
    std::string animName(nameChars);
    env->ReleaseStringUTFChars(name, nameChars);
    
    return static_cast<jlong>(server->createAnimationByNameSync(static_cast<int64_t>(artboardHandle), animName));
}

/**
 * Advances and applies an animation to its artboard.
 *
 * JNI signature: cppAdvanceAndApplyAnimation(ptr: Long, animHandle: Long, artboardHandle: Long, deltaTime: Float, advanceArtboard: Boolean): Boolean
 *
 * @param advanceArtboard If true, also advances the artboard (use when no state machine is active).
 * @return true if animation is still playing, false if completed.
 */
JNIEXPORT jboolean JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppAdvanceAndApplyAnimation(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong animHandle,
    jlong artboardHandle,
    jfloat deltaTime,
    jboolean advanceArtboard)
{
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to advance animation on null CommandServer");
        return JNI_FALSE;
    }
    
    bool stillPlaying = server->advanceAndApplyAnimation(
        static_cast<int64_t>(animHandle),
        static_cast<int64_t>(artboardHandle),
        static_cast<float>(deltaTime),
        advanceArtboard == JNI_TRUE
    );
    
    return stillPlaying ? JNI_TRUE : JNI_FALSE;
}

/**
 * Deletes an animation instance.
 *
 * JNI signature: cppDeleteAnimation(ptr: Long, animHandle: Long): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppDeleteAnimation(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong animHandle)
{
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to delete animation on null CommandServer");
        return;
    }
    
    server->deleteAnimation(static_cast<int64_t>(animHandle));
}

/**
 * Sets the animation's current time position.
 *
 * JNI signature: cppSetAnimationTime(ptr: Long, animHandle: Long, time: Float): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppSetAnimationTime(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong animHandle,
    jfloat time)
{
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to set animation time on null CommandServer");
        return;
    }
    
    server->setAnimationTime(static_cast<int64_t>(animHandle), static_cast<float>(time));
}

/**
 * Sets the animation's loop mode.
 *
 * JNI signature: cppSetAnimationLoop(ptr: Long, animHandle: Long, loopMode: Int): Unit
 *
 * @param loopMode 0=oneShot, 1=loop, 2=pingPong
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppSetAnimationLoop(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong animHandle,
    jint loopMode)
{
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to set animation loop on null CommandServer");
        return;
    }
    
    server->setAnimationLoop(static_cast<int64_t>(animHandle), static_cast<int32_t>(loopMode));
}

/**
 * Sets the animation's playback direction.
 *
 * JNI signature: cppSetAnimationDirection(ptr: Long, animHandle: Long, direction: Int): Unit
 *
 * @param direction 1=forwards, -1=backwards
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppSetAnimationDirection(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong animHandle,
    jint direction)
{
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to set animation direction on null CommandServer");
        return;
    }
    
    server->setAnimationDirection(static_cast<int64_t>(animHandle), static_cast<int32_t>(direction));
}

} // extern "C"