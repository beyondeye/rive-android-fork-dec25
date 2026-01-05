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
static jmethodID g_onArtboardCreatedMethodID = nullptr;
static jmethodID g_onArtboardErrorMethodID = nullptr;
static jmethodID g_onArtboardDeletedMethodID = nullptr;
static jmethodID g_onStateMachineCreatedMethodID = nullptr;
static jmethodID g_onStateMachineErrorMethodID = nullptr;
static jmethodID g_onStateMachineDeletedMethodID = nullptr;
static jmethodID g_onStateMachineSettledMethodID = nullptr;
// Input operation callbacks
static jmethodID g_onInputCountResultMethodID = nullptr;
static jmethodID g_onInputNamesListedMethodID = nullptr;
static jmethodID g_onInputInfoResultMethodID = nullptr;
static jmethodID g_onNumberInputValueMethodID = nullptr;
static jmethodID g_onBooleanInputValueMethodID = nullptr;
static jmethodID g_onInputOperationSuccessMethodID = nullptr;
static jmethodID g_onInputOperationErrorMethodID = nullptr;

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
    
    g_onArtboardCreatedMethodID = env->GetMethodID(
        commandQueueClass,
        "onArtboardCreated",
        "(JJ)V"  // (requestID: Long, artboardHandle: Long) -> Unit
    );
    
    g_onArtboardErrorMethodID = env->GetMethodID(
        commandQueueClass,
        "onArtboardError",
        "(JLjava/lang/String;)V"  // (requestID: Long, error: String) -> Unit
    );
    
    g_onArtboardDeletedMethodID = env->GetMethodID(
        commandQueueClass,
        "onArtboardDeleted",
        "(JJ)V"  // (requestID: Long, artboardHandle: Long) -> Unit
    );
    
    g_onStateMachineCreatedMethodID = env->GetMethodID(
        commandQueueClass,
        "onStateMachineCreated",
        "(JJ)V"  // (requestID: Long, smHandle: Long) -> Unit
    );
    
    g_onStateMachineErrorMethodID = env->GetMethodID(
        commandQueueClass,
        "onStateMachineError",
        "(JLjava/lang/String;)V"  // (requestID: Long, error: String) -> Unit
    );
    
    g_onStateMachineDeletedMethodID = env->GetMethodID(
        commandQueueClass,
        "onStateMachineDeleted",
        "(JJ)V"  // (requestID: Long, smHandle: Long) -> Unit
    );
    
    g_onStateMachineSettledMethodID = env->GetMethodID(
        commandQueueClass,
        "onStateMachineSettled",
        "(JJ)V"  // (requestID: Long, smHandle: Long) -> Unit
    );

    // Input operation callbacks
    g_onInputCountResultMethodID = env->GetMethodID(
        commandQueueClass,
        "onInputCountResult",
        "(JI)V"  // (requestID: Long, count: Int) -> Unit
    );

    g_onInputNamesListedMethodID = env->GetMethodID(
        commandQueueClass,
        "onInputNamesListed",
        "(JLjava/util/List;)V"  // (requestID: Long, names: List<String>) -> Unit
    );

    g_onInputInfoResultMethodID = env->GetMethodID(
        commandQueueClass,
        "onInputInfoResult",
        "(JLjava/lang/String;I)V"  // (requestID: Long, name: String, type: Int) -> Unit
    );

    g_onNumberInputValueMethodID = env->GetMethodID(
        commandQueueClass,
        "onNumberInputValue",
        "(JF)V"  // (requestID: Long, value: Float) -> Unit
    );

    g_onBooleanInputValueMethodID = env->GetMethodID(
        commandQueueClass,
        "onBooleanInputValue",
        "(JZ)V"  // (requestID: Long, value: Boolean) -> Unit
    );

    g_onInputOperationSuccessMethodID = env->GetMethodID(
        commandQueueClass,
        "onInputOperationSuccess",
        "(J)V"  // (requestID: Long) -> Unit
    );

    g_onInputOperationErrorMethodID = env->GetMethodID(
        commandQueueClass,
        "onInputOperationError",
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
    auto messages = server->getMessages();
    
    // Deliver messages to Kotlin by calling the appropriate callbacks
    for (const auto& msg : messages) {
        switch (msg.type) {
            case rive_android::MessageType::FileLoaded:
                env->CallVoidMethod(thiz, g_onFileLoadedMethodID, 
                    static_cast<jlong>(msg.requestID), 
                    static_cast<jlong>(msg.handle));
                break;
                
            case rive_android::MessageType::FileError:
                {
                    jstring errorStr = env->NewStringUTF(msg.error.c_str());
                    env->CallVoidMethod(thiz, g_onFileErrorMethodID, 
                        static_cast<jlong>(msg.requestID), 
                        errorStr);
                    env->DeleteLocalRef(errorStr);
                }
                break;
                
            case rive_android::MessageType::FileDeleted:
                env->CallVoidMethod(thiz, g_onFileDeletedMethodID, 
                    static_cast<jlong>(msg.requestID), 
                    static_cast<jlong>(msg.handle));
                break;
                
            case rive_android::MessageType::ArtboardNamesListed:
            case rive_android::MessageType::StateMachineNamesListed:
            case rive_android::MessageType::ViewModelNamesListed:
                {
                    // Convert std::vector<std::string> to Java List<String>
                    jclass arrayListClass = env->FindClass("java/util/ArrayList");
                    jmethodID arrayListConstructor = env->GetMethodID(arrayListClass, "<init>", "()V");
                    jmethodID arrayListAdd = env->GetMethodID(arrayListClass, "add", "(Ljava/lang/Object;)Z");
                    
                    jobject arrayList = env->NewObject(arrayListClass, arrayListConstructor);
                    
                    for (const auto& name : msg.stringList) {
                        jstring nameStr = env->NewStringUTF(name.c_str());
                        env->CallBooleanMethod(arrayList, arrayListAdd, nameStr);
                        env->DeleteLocalRef(nameStr);
                    }
                    
                    // Call the appropriate callback based on message type
                    if (msg.type == rive_android::MessageType::ArtboardNamesListed) {
                        env->CallVoidMethod(thiz, g_onArtboardNamesListedMethodID, 
                            static_cast<jlong>(msg.requestID), arrayList);
                    } else if (msg.type == rive_android::MessageType::StateMachineNamesListed) {
                        env->CallVoidMethod(thiz, g_onStateMachineNamesListedMethodID, 
                            static_cast<jlong>(msg.requestID), arrayList);
                    } else if (msg.type == rive_android::MessageType::ViewModelNamesListed) {
                        env->CallVoidMethod(thiz, g_onViewModelNamesListedMethodID, 
                            static_cast<jlong>(msg.requestID), arrayList);
                    }
                    
                    env->DeleteLocalRef(arrayList);
                    env->DeleteLocalRef(arrayListClass);
                }
                break;
                
            case rive_android::MessageType::QueryError:
                {
                    jstring errorStr = env->NewStringUTF(msg.error.c_str());
                    env->CallVoidMethod(thiz, g_onQueryErrorMethodID, 
                        static_cast<jlong>(msg.requestID), 
                        errorStr);
                    env->DeleteLocalRef(errorStr);
                }
                break;
                
            case rive_android::MessageType::ArtboardCreated:
                env->CallVoidMethod(thiz, g_onArtboardCreatedMethodID, 
                    static_cast<jlong>(msg.requestID), 
                    static_cast<jlong>(msg.handle));
                break;
                
            case rive_android::MessageType::ArtboardError:
                {
                    jstring errorStr = env->NewStringUTF(msg.error.c_str());
                    env->CallVoidMethod(thiz, g_onArtboardErrorMethodID, 
                        static_cast<jlong>(msg.requestID), 
                        errorStr);
                    env->DeleteLocalRef(errorStr);
                }
                break;
                
            case rive_android::MessageType::ArtboardDeleted:
                env->CallVoidMethod(thiz, g_onArtboardDeletedMethodID, 
                    static_cast<jlong>(msg.requestID), 
                    static_cast<jlong>(msg.handle));
                break;
                
            case rive_android::MessageType::StateMachineCreated:
                env->CallVoidMethod(thiz, g_onStateMachineCreatedMethodID, 
                    static_cast<jlong>(msg.requestID), 
                    static_cast<jlong>(msg.handle));
                break;
                
            case rive_android::MessageType::StateMachineError:
                {
                    jstring errorStr = env->NewStringUTF(msg.error.c_str());
                    env->CallVoidMethod(thiz, g_onStateMachineErrorMethodID, 
                        static_cast<jlong>(msg.requestID), 
                        errorStr);
                    env->DeleteLocalRef(errorStr);
                }
                break;
                
            case rive_android::MessageType::StateMachineDeleted:
                env->CallVoidMethod(thiz, g_onStateMachineDeletedMethodID, 
                    static_cast<jlong>(msg.requestID), 
                    static_cast<jlong>(msg.handle));
                break;
                
            case rive_android::MessageType::StateMachineSettled:
                env->CallVoidMethod(thiz, g_onStateMachineSettledMethodID,
                    static_cast<jlong>(msg.requestID),
                    static_cast<jlong>(msg.handle));
                break;

            // Input operation messages
            case rive_android::MessageType::InputCountResult:
                env->CallVoidMethod(thiz, g_onInputCountResultMethodID,
                    static_cast<jlong>(msg.requestID),
                    static_cast<jint>(msg.intValue));
                break;

            case rive_android::MessageType::InputNamesListed:
                {
                    // Convert std::vector<std::string> to Java List<String>
                    jclass arrayListClass = env->FindClass("java/util/ArrayList");
                    jmethodID arrayListConstructor = env->GetMethodID(arrayListClass, "<init>", "()V");
                    jmethodID arrayListAdd = env->GetMethodID(arrayListClass, "add", "(Ljava/lang/Object;)Z");

                    jobject arrayList = env->NewObject(arrayListClass, arrayListConstructor);

                    for (const auto& name : msg.stringList) {
                        jstring nameStr = env->NewStringUTF(name.c_str());
                        env->CallBooleanMethod(arrayList, arrayListAdd, nameStr);
                        env->DeleteLocalRef(nameStr);
                    }

                    env->CallVoidMethod(thiz, g_onInputNamesListedMethodID,
                        static_cast<jlong>(msg.requestID), arrayList);

                    env->DeleteLocalRef(arrayList);
                    env->DeleteLocalRef(arrayListClass);
                }
                break;

            case rive_android::MessageType::InputInfoResult:
                {
                    jstring nameStr = env->NewStringUTF(msg.inputName.c_str());
                    env->CallVoidMethod(thiz, g_onInputInfoResultMethodID,
                        static_cast<jlong>(msg.requestID),
                        nameStr,
                        static_cast<jint>(msg.inputType));
                    env->DeleteLocalRef(nameStr);
                }
                break;

            case rive_android::MessageType::NumberInputValue:
                env->CallVoidMethod(thiz, g_onNumberInputValueMethodID,
                    static_cast<jlong>(msg.requestID),
                    static_cast<jfloat>(msg.floatValue));
                break;

            case rive_android::MessageType::BooleanInputValue:
                env->CallVoidMethod(thiz, g_onBooleanInputValueMethodID,
                    static_cast<jlong>(msg.requestID),
                    static_cast<jboolean>(msg.boolValue));
                break;

            case rive_android::MessageType::InputOperationSuccess:
                env->CallVoidMethod(thiz, g_onInputOperationSuccessMethodID,
                    static_cast<jlong>(msg.requestID));
                break;

            case rive_android::MessageType::InputOperationError:
                {
                    jstring errorStr = env->NewStringUTF(msg.error.c_str());
                    env->CallVoidMethod(thiz, g_onInputOperationErrorMethodID,
                        static_cast<jlong>(msg.requestID),
                        errorStr);
                    env->DeleteLocalRef(errorStr);
                }
                break;

            default:
                {
                    std::string errorMsg = "CommandQueue JNI: Unknown message type: " + 
                                          std::to_string(static_cast<int>(msg.type));
                    LOGW_STR(errorMsg.c_str());
                }
                break;
        }
        
        // Check for JNI exceptions after each callback
        if (env->ExceptionCheck()) {
            std::string errorMsg = "CommandQueue JNI: Exception occurred during callback for message type: " + 
                                  std::to_string(static_cast<int>(msg.type));
            LOGE_STR(errorMsg.c_str());
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
    }
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

/**
 * Creates the default artboard from a file.
 * 
 * JNI signature: cppCreateDefaultArtboard(ptr: Long, requestID: Long, fileHandle: Long): Unit
 * 
 * @param env The JNI environment.
 * @param thiz The Java CommandQueue object.
 * @param ptr The native pointer to the CommandServer.
 * @param requestID The request ID for async completion.
 * @param fileHandle The handle of the file to create artboard from.
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_CommandQueue_cppCreateDefaultArtboard(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong fileHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to create default artboard on null CommandServer");
        return;
    }
    
    server->createDefaultArtboard(static_cast<int64_t>(requestID), static_cast<int64_t>(fileHandle));
}

/**
 * Creates an artboard by name from a file.
 * 
 * JNI signature: cppCreateArtboardByName(ptr: Long, requestID: Long, fileHandle: Long, name: String): Unit
 * 
 * @param env The JNI environment.
 * @param thiz The Java CommandQueue object.
 * @param ptr The native pointer to the CommandServer.
 * @param requestID The request ID for async completion.
 * @param fileHandle The handle of the file to create artboard from.
 * @param name The name of the artboard to create.
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_CommandQueue_cppCreateArtboardByName(
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
        return;
    }
    
    // Convert Java string to C++ string
    const char* nameChars = env->GetStringUTFChars(name, nullptr);
    std::string artboardName(nameChars);
    env->ReleaseStringUTFChars(name, nameChars);
    
    server->createArtboardByName(static_cast<int64_t>(requestID), static_cast<int64_t>(fileHandle), artboardName);
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
Java_app_rive_mp_CommandQueue_cppDeleteArtboard(
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
Java_app_rive_mp_CommandQueue_cppCreateDefaultStateMachine(
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
Java_app_rive_mp_CommandQueue_cppCreateStateMachineByName(
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
 * JNI signature: cppAdvanceStateMachine(ptr: Long, requestID: Long, smHandle: Long, deltaTimeSeconds: Float): Unit
 * 
 * @param env The JNI environment.
 * @param thiz The Java CommandQueue object.
 * @param ptr The native pointer to the CommandServer.
 * @param requestID The request ID for async completion.
 * @param smHandle The handle of the state machine to advance.
 * @param deltaTimeSeconds The time delta in seconds.
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_CommandQueue_cppAdvanceStateMachine(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong smHandle,
    jfloat deltaTimeSeconds
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to advance state machine on null CommandServer");
        return;
    }
    
    server->advanceStateMachine(static_cast<int64_t>(requestID), static_cast<int64_t>(smHandle), static_cast<float>(deltaTimeSeconds));
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
Java_app_rive_mp_CommandQueue_cppDeleteStateMachine(
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
Java_app_rive_mp_CommandQueue_cppGetInputCount(
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
Java_app_rive_mp_CommandQueue_cppGetInputNames(
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
Java_app_rive_mp_CommandQueue_cppGetInputInfo(
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
Java_app_rive_mp_CommandQueue_cppGetNumberInput(
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
Java_app_rive_mp_CommandQueue_cppSetNumberInput(
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
Java_app_rive_mp_CommandQueue_cppGetBooleanInput(
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
Java_app_rive_mp_CommandQueue_cppSetBooleanInput(
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
Java_app_rive_mp_CommandQueue_cppFireTrigger(
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

} // extern "C"
