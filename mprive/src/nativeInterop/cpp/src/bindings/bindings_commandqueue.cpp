#include <jni.h>
#include "command_server.hpp"
#include "jni_helpers.hpp"
#include "rive_log.hpp"

using namespace rive_android;
using namespace rive_mp;

extern "C" {

/**
 * Constructs a native CommandServer and returns its pointer.
 * 
 * JNI signature: cppConstructor(renderContextPtr: Long): Long
 * 
 * @param env The JNI environment.
 * @param thiz The Java CommandQueue object.
 * @param renderContextPtr The native pointer to the RenderContext.
 * @return The native pointer to the created CommandServer.
 */
JNIEXPORT jlong JNICALL
Java_app_rive_mp_CommandQueue_cppConstructor(
    JNIEnv* env,
    jobject thiz,
    jlong renderContextPtr
) {
    LOGI("CommandQueue JNI: Creating CommandServer");
    
    void* renderContext = reinterpret_cast<void*>(renderContextPtr);
    auto* server = new CommandServer(env, thiz, renderContext);
    
    LOGI("CommandQueue JNI: CommandServer created successfully");
    return reinterpret_cast<jlong>(server);
}

/**
 * Deletes the native CommandServer.
 * 
 * JNI signature: cppDelete(ptr: Long): Unit
 * 
 * @param env The JNI environment.
 * @param thiz The Java CommandQueue object.
 * @param ptr The native pointer to the CommandServer to delete.
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_CommandQueue_cppDelete(
    JNIEnv* env,
    jobject thiz,
    jlong ptr
) {
    LOGI("CommandQueue JNI: Deleting CommandServer");
    
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server != nullptr) {
        delete server;
        LOGI("CommandQueue JNI: CommandServer deleted successfully");
    } else {
        LOGW("CommandQueue JNI: Attempted to delete null CommandServer");
    }
}

/**
 * Polls messages from the CommandServer.
 * 
 * JNI signature: cppPollMessages(ptr: Long): Unit
 * 
 * @param env The JNI environment.
 * @param thiz The Java CommandQueue object.
 * @param ptr The native pointer to the CommandServer.
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_CommandQueue_cppPollMessages(
    JNIEnv* env,
    jobject thiz,
    jlong ptr
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server != nullptr) {
        server->pollMessages();
    } else {
        LOGW("CommandQueue JNI: Attempted to poll messages on null CommandServer");
    }
}

// Phase B+: Add more JNI bindings here
// - cppLoadFile
// - cppDeleteFile
// - cppGetArtboardNames
// - cppCreateDefaultArtboard
// - etc.

} // extern "C"
