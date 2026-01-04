#include <jni.h>
#include <vector>
#include "command_server.hpp"
#include "jni_helpers.hpp"
#include "rive_log.hpp"

using namespace rive_android;
using namespace rive_mp;

// Cached method IDs for callbacks
static jmethodID g_onFileLoadedMethodID = nullptr;
static jmethodID g_onFileErrorMethodID = nullptr;
static jmethodID g_onFileDeletedMethodID = nullptr;
static jmethodID g_onArtboardNamesListedMethodID = nullptr;
static jmethodID g_onStateMachineNamesListedMethodID = nullptr;
static jmethodID g_onViewModelNamesListedMethodID = nullptr;
static jmethodID g_onQueryErrorMethodID = nullptr;

/**
 * Initialize cached method IDs for JNI callbacks.
 * This is called once during first use to avoid repeated JNI lookups.
 */
static void initCallbackMethodIDs(JNIEnv* env, jobject commandQueue) {
    if (g_onFileLoadedMethodID != nullptr) {
        return;  // Already initialized
    }
    
    jclass commandQueueClass = env->GetObjectClass(commandQueue);
    
    g_onFileLoadedMethodID = env->GetMethodID(
        commandQueueClass, 
        "onFileLoaded", 
        "(JJ)V"  // (requestID: Long, fileHandle: Long) -> Unit
    );
    
    g_onFileErrorMethodID = env->GetMethodID(
        commandQueueClass, 
        "onFileError", 
        "(JLjava/lang/String;)V"  // (requestID: Long, error: String) -> Unit
    );
    
    g_onFileDeletedMethodID = env->GetMethodID(
        commandQueueClass, 
        "onFileDeleted", 
        "(JJ)V"  // (requestID: Long, fileHandle: Long) -> Unit
    );
    
    g_onArtboardNamesListedMethodID = env->GetMethodID(
        commandQueueClass,
        "onArtboardNamesListed",
        "(JLjava/util/List;)V"  // (requestID: Long, names: List<String>) -> Unit
    );
    
    g_onStateMachineNamesListedMethodID = env->GetMethodID(
        commandQueueClass,
        "onStateMachineNamesListed",
        "(JLjava/util/List;)V"  // (requestID: Long, names: List<String>) -> Unit
    );
    
    g_onViewModelNamesListedMethodID = env->GetMethodID(
        commandQueueClass,
        "onViewModelNamesListed",
        "(JLjava/util/List;)V"  // (requestID: Long, names: List<String>) -> Unit
    );
    
    g_onQueryErrorMethodID = env->GetMethodID(
        commandQueueClass,
        "onQueryError",
        "(JLjava/lang/String;)V"  // (requestID: Long, error: String) -> Unit
    );
    
    env->DeleteLocalRef(commandQueueClass);
}

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
 * Polls messages from the CommandServer and delivers them to Kotlin.
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
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to poll messages on null CommandServer");
        return;
    }
    
    // Initialize callback method IDs if needed
    initCallbackMethodIDs(env, thiz);
    
    // Get messages from the server
    // For Phase B.1, we'll implement a proper polling mechanism
    // For now, pollMessages just logs the messages
    server->pollMessages();
    
    // TODO: In a future iteration, we'll extract messages from the server
    // and call the appropriate Kotlin callbacks here
}

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
Java_app_rive_mp_CommandQueue_cppLoadFile(
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
Java_app_rive_mp_CommandQueue_cppDeleteFile(
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
Java_app_rive_mp_CommandQueue_cppGetArtboardNames(
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
Java_app_rive_mp_CommandQueue_cppGetStateMachineNames(
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
Java_app_rive_mp_CommandQueue_cppGetViewModelNames(
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

// Phase B+: Add more JNI bindings here
// - cppCreateDefaultArtboard
// - etc.

} // extern "C"
