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
// ViewModelInstance callbacks
static jmethodID g_onVMICreatedMethodID = nullptr;
static jmethodID g_onVMIErrorMethodID = nullptr;
static jmethodID g_onVMIDeletedMethodID = nullptr;
// Property operation callbacks (Phase D.2)
static jmethodID g_onNumberPropertyValueMethodID = nullptr;
static jmethodID g_onStringPropertyValueMethodID = nullptr;
static jmethodID g_onBooleanPropertyValueMethodID = nullptr;
static jmethodID g_onPropertyErrorMethodID = nullptr;
static jmethodID g_onPropertySetSuccessMethodID = nullptr;
// Additional property callbacks (Phase D.3)
static jmethodID g_onEnumPropertyValueMethodID = nullptr;
static jmethodID g_onColorPropertyValueMethodID = nullptr;
static jmethodID g_onTriggerFiredMethodID = nullptr;
// Property subscription update callbacks (Phase D.4)
static jmethodID g_onNumberPropertyUpdatedMethodID = nullptr;
static jmethodID g_onStringPropertyUpdatedMethodID = nullptr;
static jmethodID g_onBooleanPropertyUpdatedMethodID = nullptr;
static jmethodID g_onEnumPropertyUpdatedMethodID = nullptr;
static jmethodID g_onColorPropertyUpdatedMethodID = nullptr;
static jmethodID g_onTriggerPropertyFiredMethodID = nullptr;
// List operation callbacks (Phase D.5)
static jmethodID g_onListSizeResultMethodID = nullptr;
static jmethodID g_onListItemResultMethodID = nullptr;
static jmethodID g_onListOperationSuccessMethodID = nullptr;
static jmethodID g_onListOperationErrorMethodID = nullptr;
// Nested VMI operation callbacks (Phase D.5)
static jmethodID g_onInstancePropertyResultMethodID = nullptr;
static jmethodID g_onInstancePropertySetSuccessMethodID = nullptr;
static jmethodID g_onInstancePropertyErrorMethodID = nullptr;
// Asset property operation callbacks (Phase D.5)
static jmethodID g_onAssetPropertySetSuccessMethodID = nullptr;
static jmethodID g_onAssetPropertyErrorMethodID = nullptr;
// VMI binding operation callbacks (Phase D.6)
static jmethodID g_onVMIBindingSuccessMethodID = nullptr;
static jmethodID g_onVMIBindingErrorMethodID = nullptr;
static jmethodID g_onDefaultVMIResultMethodID = nullptr;
static jmethodID g_onDefaultVMIErrorMethodID = nullptr;
// Render target operation callbacks (Phase C.2.3)
static jmethodID g_onRenderTargetCreatedMethodID = nullptr;
static jmethodID g_onRenderTargetErrorMethodID = nullptr;
static jmethodID g_onRenderTargetDeletedMethodID = nullptr;

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

    // ViewModelInstance callbacks
    g_onVMICreatedMethodID = env->GetMethodID(
        commandQueueClass,
        "onVMICreated",
        "(JJ)V"  // (requestID: Long, vmiHandle: Long) -> Unit
    );

    g_onVMIErrorMethodID = env->GetMethodID(
        commandQueueClass,
        "onVMIError",
        "(JLjava/lang/String;)V"  // (requestID: Long, error: String) -> Unit
    );

    g_onVMIDeletedMethodID = env->GetMethodID(
        commandQueueClass,
        "onVMIDeleted",
        "(JJ)V"  // (requestID: Long, vmiHandle: Long) -> Unit
    );

    // Property operation callbacks (Phase D.2)
    g_onNumberPropertyValueMethodID = env->GetMethodID(
        commandQueueClass,
        "onNumberPropertyValue",
        "(JF)V"  // (requestID: Long, value: Float) -> Unit
    );

    g_onStringPropertyValueMethodID = env->GetMethodID(
        commandQueueClass,
        "onStringPropertyValue",
        "(JLjava/lang/String;)V"  // (requestID: Long, value: String) -> Unit
    );

    g_onBooleanPropertyValueMethodID = env->GetMethodID(
        commandQueueClass,
        "onBooleanPropertyValue",
        "(JZ)V"  // (requestID: Long, value: Boolean) -> Unit
    );

    g_onPropertyErrorMethodID = env->GetMethodID(
        commandQueueClass,
        "onPropertyError",
        "(JLjava/lang/String;)V"  // (requestID: Long, error: String) -> Unit
    );

    g_onPropertySetSuccessMethodID = env->GetMethodID(
        commandQueueClass,
        "onPropertySetSuccess",
        "(J)V"  // (requestID: Long) -> Unit
    );

    // Additional property callbacks (Phase D.3)
    g_onEnumPropertyValueMethodID = env->GetMethodID(
        commandQueueClass,
        "onEnumPropertyValue",
        "(JLjava/lang/String;)V"  // (requestID: Long, value: String) -> Unit
    );

    g_onColorPropertyValueMethodID = env->GetMethodID(
        commandQueueClass,
        "onColorPropertyValue",
        "(JI)V"  // (requestID: Long, value: Int) -> Unit
    );

    g_onTriggerFiredMethodID = env->GetMethodID(
        commandQueueClass,
        "onTriggerFired",
        "(J)V"  // (requestID: Long) -> Unit
    );

    // Property subscription update callbacks (Phase D.4)
    g_onNumberPropertyUpdatedMethodID = env->GetMethodID(
        commandQueueClass,
        "onNumberPropertyUpdated",
        "(JLjava/lang/String;F)V"  // (vmiHandle: Long, propertyPath: String, value: Float) -> Unit
    );

    g_onStringPropertyUpdatedMethodID = env->GetMethodID(
        commandQueueClass,
        "onStringPropertyUpdated",
        "(JLjava/lang/String;Ljava/lang/String;)V"  // (vmiHandle: Long, propertyPath: String, value: String) -> Unit
    );

    g_onBooleanPropertyUpdatedMethodID = env->GetMethodID(
        commandQueueClass,
        "onBooleanPropertyUpdated",
        "(JLjava/lang/String;Z)V"  // (vmiHandle: Long, propertyPath: String, value: Boolean) -> Unit
    );

    g_onEnumPropertyUpdatedMethodID = env->GetMethodID(
        commandQueueClass,
        "onEnumPropertyUpdated",
        "(JLjava/lang/String;Ljava/lang/String;)V"  // (vmiHandle: Long, propertyPath: String, value: String) -> Unit
    );

    g_onColorPropertyUpdatedMethodID = env->GetMethodID(
        commandQueueClass,
        "onColorPropertyUpdated",
        "(JLjava/lang/String;I)V"  // (vmiHandle: Long, propertyPath: String, value: Int) -> Unit
    );

    g_onTriggerPropertyFiredMethodID = env->GetMethodID(
        commandQueueClass,
        "onTriggerPropertyFired",
        "(JLjava/lang/String;)V"  // (vmiHandle: Long, propertyPath: String) -> Unit
    );

    // Phase D.5: List operation callbacks
    g_onListSizeResultMethodID = env->GetMethodID(
        commandQueueClass,
        "onListSizeResult",
        "(JI)V"  // (requestID: Long, size: Int) -> Unit
    );

    g_onListItemResultMethodID = env->GetMethodID(
        commandQueueClass,
        "onListItemResult",
        "(JJ)V"  // (requestID: Long, itemHandle: Long) -> Unit
    );

    g_onListOperationSuccessMethodID = env->GetMethodID(
        commandQueueClass,
        "onListOperationSuccess",
        "(J)V"  // (requestID: Long) -> Unit
    );

    g_onListOperationErrorMethodID = env->GetMethodID(
        commandQueueClass,
        "onListOperationError",
        "(JLjava/lang/String;)V"  // (requestID: Long, error: String) -> Unit
    );

    // Phase D.5: Nested VMI operation callbacks
    g_onInstancePropertyResultMethodID = env->GetMethodID(
        commandQueueClass,
        "onInstancePropertyResult",
        "(JJ)V"  // (requestID: Long, nestedHandle: Long) -> Unit
    );

    g_onInstancePropertySetSuccessMethodID = env->GetMethodID(
        commandQueueClass,
        "onInstancePropertySetSuccess",
        "(J)V"  // (requestID: Long) -> Unit
    );

    g_onInstancePropertyErrorMethodID = env->GetMethodID(
        commandQueueClass,
        "onInstancePropertyError",
        "(JLjava/lang/String;)V"  // (requestID: Long, error: String) -> Unit
    );

    // Phase D.5: Asset property operation callbacks
    g_onAssetPropertySetSuccessMethodID = env->GetMethodID(
        commandQueueClass,
        "onAssetPropertySetSuccess",
        "(J)V"  // (requestID: Long) -> Unit
    );

    g_onAssetPropertyErrorMethodID = env->GetMethodID(
        commandQueueClass,
        "onAssetPropertyError",
        "(JLjava/lang/String;)V"  // (requestID: Long, error: String) -> Unit
    );

    // Phase D.6: VMI binding operation callbacks
    g_onVMIBindingSuccessMethodID = env->GetMethodID(
        commandQueueClass,
        "onVMIBindingSuccess",
        "(J)V"  // (requestID: Long) -> Unit
    );

    g_onVMIBindingErrorMethodID = env->GetMethodID(
        commandQueueClass,
        "onVMIBindingError",
        "(JLjava/lang/String;)V"  // (requestID: Long, error: String) -> Unit
    );

    g_onDefaultVMIResultMethodID = env->GetMethodID(
        commandQueueClass,
        "onDefaultVMIResult",
        "(JJ)V"  // (requestID: Long, vmiHandle: Long) -> Unit
    );

    g_onDefaultVMIErrorMethodID = env->GetMethodID(
        commandQueueClass,
        "onDefaultVMIError",
        "(JLjava/lang/String;)V"  // (requestID: Long, error: String) -> Unit
    );

    // Phase C.2.3: Render target operations
    g_onRenderTargetCreatedMethodID = env->GetMethodID(
        commandQueueClass,
        "onRenderTargetCreated",
        "(JJ)V"  // (requestID: Long, renderTargetHandle: Long) -> Unit
    );

    g_onRenderTargetErrorMethodID = env->GetMethodID(
        commandQueueClass,
        "onRenderTargetError",
        "(JLjava/lang/String;)V"  // (requestID: Long, error: String) -> Unit
    );

    g_onRenderTargetDeletedMethodID = env->GetMethodID(
        commandQueueClass,
        "onRenderTargetDeleted",
        "(J)V"  // (requestID: Long) -> Unit
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
Java_app_rive_mp_core_CommandQueueJNIBridge_cppConstructor(
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
Java_app_rive_mp_core_CommandQueueJNIBridge_cppDelete(
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
 * JNI signature: cppPollMessages(ptr: Long, receiver: CommandQueue): Unit
 * 
 * @param env The JNI environment.
 * @param thiz The Java CommandQueueJNIBridge object.
 * @param ptr The native pointer to the CommandServer.
 * @param receiver The CommandQueue instance to receive callbacks.
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppPollMessages(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jobject receiver
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to poll messages on null CommandServer");
        return;
    }
    
    if (receiver == nullptr) {
        LOGW("CommandQueue JNI: Attempted to poll messages with null receiver");
        return;
    }
    
    // Initialize callback method IDs using the CommandQueue receiver class
    initCallbackMethodIDs(env, receiver);
    
    // Get messages from the server
    auto messages = server->getMessages();
    
    // Deliver messages to Kotlin by calling the appropriate callbacks on the receiver
    for (const auto& msg : messages) {
        switch (msg.type) {
            case rive_android::MessageType::FileLoaded:
                env->CallVoidMethod(receiver, g_onFileLoadedMethodID, 
                    static_cast<jlong>(msg.requestID), 
                    static_cast<jlong>(msg.handle));
                break;
                
            case rive_android::MessageType::FileError:
                {
                    jstring errorStr = env->NewStringUTF(msg.error.c_str());
                    env->CallVoidMethod(receiver, g_onFileErrorMethodID, 
                        static_cast<jlong>(msg.requestID), 
                        errorStr);
                    env->DeleteLocalRef(errorStr);
                }
                break;
                
            case rive_android::MessageType::FileDeleted:
                env->CallVoidMethod(receiver, g_onFileDeletedMethodID, 
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
                        env->CallVoidMethod(receiver, g_onArtboardNamesListedMethodID, 
                            static_cast<jlong>(msg.requestID), arrayList);
                    } else if (msg.type == rive_android::MessageType::StateMachineNamesListed) {
                        env->CallVoidMethod(receiver, g_onStateMachineNamesListedMethodID, 
                            static_cast<jlong>(msg.requestID), arrayList);
                    } else if (msg.type == rive_android::MessageType::ViewModelNamesListed) {
                        env->CallVoidMethod(receiver, g_onViewModelNamesListedMethodID, 
                            static_cast<jlong>(msg.requestID), arrayList);
                    }
                    
                    env->DeleteLocalRef(arrayList);
                    env->DeleteLocalRef(arrayListClass);
                }
                break;
                
            case rive_android::MessageType::QueryError:
                {
                    jstring errorStr = env->NewStringUTF(msg.error.c_str());
                    env->CallVoidMethod(receiver, g_onQueryErrorMethodID, 
                        static_cast<jlong>(msg.requestID), 
                        errorStr);
                    env->DeleteLocalRef(errorStr);
                }
                break;
                
            case rive_android::MessageType::ArtboardCreated:
                env->CallVoidMethod(receiver, g_onArtboardCreatedMethodID, 
                    static_cast<jlong>(msg.requestID), 
                    static_cast<jlong>(msg.handle));
                break;
                
            case rive_android::MessageType::ArtboardError:
                {
                    jstring errorStr = env->NewStringUTF(msg.error.c_str());
                    env->CallVoidMethod(receiver, g_onArtboardErrorMethodID, 
                        static_cast<jlong>(msg.requestID), 
                        errorStr);
                    env->DeleteLocalRef(errorStr);
                }
                break;
                
            case rive_android::MessageType::ArtboardDeleted:
                env->CallVoidMethod(receiver, g_onArtboardDeletedMethodID, 
                    static_cast<jlong>(msg.requestID), 
                    static_cast<jlong>(msg.handle));
                break;
                
            case rive_android::MessageType::StateMachineCreated:
                env->CallVoidMethod(receiver, g_onStateMachineCreatedMethodID, 
                    static_cast<jlong>(msg.requestID), 
                    static_cast<jlong>(msg.handle));
                break;
                
            case rive_android::MessageType::StateMachineError:
                {
                    jstring errorStr = env->NewStringUTF(msg.error.c_str());
                    env->CallVoidMethod(receiver, g_onStateMachineErrorMethodID, 
                        static_cast<jlong>(msg.requestID), 
                        errorStr);
                    env->DeleteLocalRef(errorStr);
                }
                break;
                
            case rive_android::MessageType::StateMachineDeleted:
                env->CallVoidMethod(receiver, g_onStateMachineDeletedMethodID, 
                    static_cast<jlong>(msg.requestID), 
                    static_cast<jlong>(msg.handle));
                break;
                
            case rive_android::MessageType::StateMachineSettled:
                env->CallVoidMethod(receiver, g_onStateMachineSettledMethodID,
                    static_cast<jlong>(msg.requestID),
                    static_cast<jlong>(msg.handle));
                break;

            // Input operation messages
            case rive_android::MessageType::InputCountResult:
                env->CallVoidMethod(receiver, g_onInputCountResultMethodID,
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

                    env->CallVoidMethod(receiver, g_onInputNamesListedMethodID,
                        static_cast<jlong>(msg.requestID), arrayList);

                    env->DeleteLocalRef(arrayList);
                    env->DeleteLocalRef(arrayListClass);
                }
                break;

            case rive_android::MessageType::InputInfoResult:
                {
                    jstring nameStr = env->NewStringUTF(msg.inputName.c_str());
                    env->CallVoidMethod(receiver, g_onInputInfoResultMethodID,
                        static_cast<jlong>(msg.requestID),
                        nameStr,
                        static_cast<jint>(msg.inputType));
                    env->DeleteLocalRef(nameStr);
                }
                break;

            case rive_android::MessageType::NumberInputValue:
                env->CallVoidMethod(receiver, g_onNumberInputValueMethodID,
                    static_cast<jlong>(msg.requestID),
                    static_cast<jfloat>(msg.floatValue));
                break;

            case rive_android::MessageType::BooleanInputValue:
                env->CallVoidMethod(receiver, g_onBooleanInputValueMethodID,
                    static_cast<jlong>(msg.requestID),
                    static_cast<jboolean>(msg.boolValue));
                break;

            case rive_android::MessageType::InputOperationSuccess:
                env->CallVoidMethod(receiver, g_onInputOperationSuccessMethodID,
                    static_cast<jlong>(msg.requestID));
                break;

            case rive_android::MessageType::InputOperationError:
                {
                    jstring errorStr = env->NewStringUTF(msg.error.c_str());
                    env->CallVoidMethod(receiver, g_onInputOperationErrorMethodID,
                        static_cast<jlong>(msg.requestID),
                        errorStr);
                    env->DeleteLocalRef(errorStr);
                }
                break;

            // ViewModelInstance messages
            case rive_android::MessageType::VMICreated:
                env->CallVoidMethod(receiver, g_onVMICreatedMethodID,
                    static_cast<jlong>(msg.requestID),
                    static_cast<jlong>(msg.handle));
                break;

            case rive_android::MessageType::VMIError:
                {
                    jstring errorStr = env->NewStringUTF(msg.error.c_str());
                    env->CallVoidMethod(receiver, g_onVMIErrorMethodID,
                        static_cast<jlong>(msg.requestID),
                        errorStr);
                    env->DeleteLocalRef(errorStr);
                }
                break;

            case rive_android::MessageType::VMIDeleted:
                env->CallVoidMethod(receiver, g_onVMIDeletedMethodID,
                    static_cast<jlong>(msg.requestID),
                    static_cast<jlong>(msg.handle));
                break;

            // Property operation messages (Phase D.2)
            case rive_android::MessageType::NumberPropertyValue:
                env->CallVoidMethod(receiver, g_onNumberPropertyValueMethodID,
                    static_cast<jlong>(msg.requestID),
                    static_cast<jfloat>(msg.floatValue));
                break;

            case rive_android::MessageType::StringPropertyValue:
                {
                    jstring valueStr = env->NewStringUTF(msg.stringValue.c_str());
                    env->CallVoidMethod(receiver, g_onStringPropertyValueMethodID,
                        static_cast<jlong>(msg.requestID),
                        valueStr);
                    env->DeleteLocalRef(valueStr);
                }
                break;

            case rive_android::MessageType::BooleanPropertyValue:
                env->CallVoidMethod(receiver, g_onBooleanPropertyValueMethodID,
                    static_cast<jlong>(msg.requestID),
                    static_cast<jboolean>(msg.boolValue));
                break;

            case rive_android::MessageType::PropertyError:
                {
                    jstring errorStr = env->NewStringUTF(msg.error.c_str());
                    env->CallVoidMethod(receiver, g_onPropertyErrorMethodID,
                        static_cast<jlong>(msg.requestID),
                        errorStr);
                    env->DeleteLocalRef(errorStr);
                }
                break;

            case rive_android::MessageType::PropertySetSuccess:
                env->CallVoidMethod(receiver, g_onPropertySetSuccessMethodID,
                    static_cast<jlong>(msg.requestID));
                break;

            // Additional property messages (Phase D.3)
            case rive_android::MessageType::EnumPropertyValue:
                {
                    jstring valueStr = env->NewStringUTF(msg.stringValue.c_str());
                    env->CallVoidMethod(receiver, g_onEnumPropertyValueMethodID,
                        static_cast<jlong>(msg.requestID),
                        valueStr);
                    env->DeleteLocalRef(valueStr);
                }
                break;

            case rive_android::MessageType::ColorPropertyValue:
                env->CallVoidMethod(receiver, g_onColorPropertyValueMethodID,
                    static_cast<jlong>(msg.requestID),
                    static_cast<jint>(msg.colorValue));
                break;

            case rive_android::MessageType::TriggerFired:
                env->CallVoidMethod(receiver, g_onTriggerFiredMethodID,
                    static_cast<jlong>(msg.requestID));
                break;

            // Property subscription updates (Phase D.4)
            case rive_android::MessageType::NumberPropertyUpdated:
                {
                    jstring pathStr = env->NewStringUTF(msg.propertyPath.c_str());
                    env->CallVoidMethod(receiver, g_onNumberPropertyUpdatedMethodID,
                        static_cast<jlong>(msg.vmiHandle),
                        pathStr,
                        static_cast<jfloat>(msg.floatValue));
                    env->DeleteLocalRef(pathStr);
                }
                break;

            case rive_android::MessageType::StringPropertyUpdated:
                {
                    jstring pathStr = env->NewStringUTF(msg.propertyPath.c_str());
                    jstring valueStr = env->NewStringUTF(msg.stringValue.c_str());
                    env->CallVoidMethod(receiver, g_onStringPropertyUpdatedMethodID,
                        static_cast<jlong>(msg.vmiHandle),
                        pathStr,
                        valueStr);
                    env->DeleteLocalRef(valueStr);
                    env->DeleteLocalRef(pathStr);
                }
                break;

            case rive_android::MessageType::BooleanPropertyUpdated:
                {
                    jstring pathStr = env->NewStringUTF(msg.propertyPath.c_str());
                    env->CallVoidMethod(receiver, g_onBooleanPropertyUpdatedMethodID,
                        static_cast<jlong>(msg.vmiHandle),
                        pathStr,
                        static_cast<jboolean>(msg.boolValue));
                    env->DeleteLocalRef(pathStr);
                }
                break;

            case rive_android::MessageType::EnumPropertyUpdated:
                {
                    jstring pathStr = env->NewStringUTF(msg.propertyPath.c_str());
                    jstring valueStr = env->NewStringUTF(msg.stringValue.c_str());
                    env->CallVoidMethod(receiver, g_onEnumPropertyUpdatedMethodID,
                        static_cast<jlong>(msg.vmiHandle),
                        pathStr,
                        valueStr);
                    env->DeleteLocalRef(valueStr);
                    env->DeleteLocalRef(pathStr);
                }
                break;

            case rive_android::MessageType::ColorPropertyUpdated:
                {
                    jstring pathStr = env->NewStringUTF(msg.propertyPath.c_str());
                    env->CallVoidMethod(receiver, g_onColorPropertyUpdatedMethodID,
                        static_cast<jlong>(msg.vmiHandle),
                        pathStr,
                        static_cast<jint>(msg.colorValue));
                    env->DeleteLocalRef(pathStr);
                }
                break;

            case rive_android::MessageType::TriggerPropertyFired:
                {
                    jstring pathStr = env->NewStringUTF(msg.propertyPath.c_str());
                    env->CallVoidMethod(receiver, g_onTriggerPropertyFiredMethodID,
                        static_cast<jlong>(msg.vmiHandle),
                        pathStr);
                    env->DeleteLocalRef(pathStr);
                }
                break;

            // Phase D.5: List operation results
            case rive_android::MessageType::ListSizeResult:
                env->CallVoidMethod(receiver, g_onListSizeResultMethodID,
                    static_cast<jlong>(msg.requestID),
                    static_cast<jint>(msg.intValue));
                break;

            case rive_android::MessageType::ListItemResult:
                env->CallVoidMethod(receiver, g_onListItemResultMethodID,
                    static_cast<jlong>(msg.requestID),
                    static_cast<jlong>(msg.handle));
                break;

            case rive_android::MessageType::ListOperationSuccess:
                env->CallVoidMethod(receiver, g_onListOperationSuccessMethodID,
                    static_cast<jlong>(msg.requestID));
                break;

            case rive_android::MessageType::ListOperationError:
                {
                    jstring errorStr = env->NewStringUTF(msg.error.c_str());
                    env->CallVoidMethod(receiver, g_onListOperationErrorMethodID,
                        static_cast<jlong>(msg.requestID),
                        errorStr);
                    env->DeleteLocalRef(errorStr);
                }
                break;

            // Phase D.5: Nested VMI operation results
            case rive_android::MessageType::InstancePropertyResult:
                env->CallVoidMethod(receiver, g_onInstancePropertyResultMethodID,
                    static_cast<jlong>(msg.requestID),
                    static_cast<jlong>(msg.handle));
                break;

            case rive_android::MessageType::InstancePropertySetSuccess:
                env->CallVoidMethod(receiver, g_onInstancePropertySetSuccessMethodID,
                    static_cast<jlong>(msg.requestID));
                break;

            case rive_android::MessageType::InstancePropertyError:
                {
                    jstring errorStr = env->NewStringUTF(msg.error.c_str());
                    env->CallVoidMethod(receiver, g_onInstancePropertyErrorMethodID,
                        static_cast<jlong>(msg.requestID),
                        errorStr);
                    env->DeleteLocalRef(errorStr);
                }
                break;

            // Phase D.5: Asset property operation results
            case rive_android::MessageType::AssetPropertySetSuccess:
                env->CallVoidMethod(receiver, g_onAssetPropertySetSuccessMethodID,
                    static_cast<jlong>(msg.requestID));
                break;

            case rive_android::MessageType::AssetPropertyError:
                {
                    jstring errorStr = env->NewStringUTF(msg.error.c_str());
                    env->CallVoidMethod(receiver, g_onAssetPropertyErrorMethodID,
                        static_cast<jlong>(msg.requestID),
                        errorStr);
                    env->DeleteLocalRef(errorStr);
                }
                break;

            // Phase D.6: VMI Binding operation results
            case rive_android::MessageType::VMIBindingSuccess:
                env->CallVoidMethod(receiver, g_onVMIBindingSuccessMethodID,
                    static_cast<jlong>(msg.requestID));
                break;

            case rive_android::MessageType::VMIBindingError:
                {
                    jstring errorStr = env->NewStringUTF(msg.error.c_str());
                    env->CallVoidMethod(receiver, g_onVMIBindingErrorMethodID,
                        static_cast<jlong>(msg.requestID),
                        errorStr);
                    env->DeleteLocalRef(errorStr);
                }
                break;

            case rive_android::MessageType::DefaultVMIResult:
                env->CallVoidMethod(receiver, g_onDefaultVMIResultMethodID,
                    static_cast<jlong>(msg.requestID),
                    static_cast<jlong>(msg.handle));
                break;

            case rive_android::MessageType::DefaultVMIError:
                {
                    jstring errorStr = env->NewStringUTF(msg.error.c_str());
                    env->CallVoidMethod(receiver, g_onDefaultVMIErrorMethodID,
                        static_cast<jlong>(msg.requestID),
                        errorStr);
                    env->DeleteLocalRef(errorStr);
                }
                break;

            // Phase C.2.3: Render target operations
            case rive_android::MessageType::RenderTargetCreated:
                env->CallVoidMethod(receiver, g_onRenderTargetCreatedMethodID,
                    static_cast<jlong>(msg.requestID),
                    static_cast<jlong>(msg.handle));
                break;

            case rive_android::MessageType::RenderTargetError:
                {
                    jstring errorStr = env->NewStringUTF(msg.error.c_str());
                    env->CallVoidMethod(receiver, g_onRenderTargetErrorMethodID,
                        static_cast<jlong>(msg.requestID),
                        errorStr);
                    env->DeleteLocalRef(errorStr);
                }
                break;

            case rive_android::MessageType::RenderTargetDeleted:
                env->CallVoidMethod(receiver, g_onRenderTargetDeletedMethodID,
                    static_cast<jlong>(msg.requestID));
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
Java_app_rive_mp_core_CommandQueueJNIBridge_cppAdvanceStateMachine(
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
 * JNI signature: cppSetNumberProperty(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String, value: Float): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppSetNumberProperty(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
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

    server->setNumberProperty(static_cast<int64_t>(requestID), static_cast<int64_t>(vmiHandle), path, static_cast<float>(value));
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
 * JNI signature: cppSetStringProperty(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String, value: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppSetStringProperty(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
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

    server->setStringProperty(static_cast<int64_t>(requestID), static_cast<int64_t>(vmiHandle), path, valueStr);
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
 * JNI signature: cppSetBooleanProperty(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String, value: Boolean): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppSetBooleanProperty(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
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

    server->setBooleanProperty(static_cast<int64_t>(requestID), static_cast<int64_t>(vmiHandle), path, static_cast<bool>(value));
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
 * JNI signature: cppSetEnumProperty(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String, value: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppSetEnumProperty(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
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

    server->setEnumProperty(static_cast<int64_t>(requestID), static_cast<int64_t>(vmiHandle), path, valueStr);
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
 * JNI signature: cppSetColorProperty(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String, value: Int): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppSetColorProperty(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
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

    server->setColorProperty(static_cast<int64_t>(requestID), static_cast<int64_t>(vmiHandle), path, static_cast<int32_t>(value));
}

/**
 * Fires a trigger property on a ViewModelInstance.
 *
 * JNI signature: cppFireTriggerProperty(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppFireTriggerProperty(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
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

    server->fireTriggerProperty(static_cast<int64_t>(requestID), static_cast<int64_t>(vmiHandle), path);
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

/**
 * Creates a Rive render target on the render thread (Phase C.2.3).
 * Enqueues a CreateRenderTarget command and returns asynchronously via pollMessages.
 *
 * JNI signature: cppCreateRenderTarget(ptr: Long, requestID: Long, width: Int, height: Int, sampleCount: Int): Unit
 *
 * @param env The JNI environment.
 * @param thiz The Java CommandQueue object.
 * @param ptr The native pointer to the CommandServer.
 * @param requestID The request ID for async completion.
 * @param width The width of the render target in pixels.
 * @param height The height of the render target in pixels.
 * @param sampleCount MSAA sample count (0 = no MSAA).
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppCreateRenderTarget(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jint width,
    jint height,
    jint sampleCount
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to create render target on null CommandServer");
        return;
    }

    // Enqueue the create render target command
    server->createRenderTarget(static_cast<int64_t>(requestID),
                               static_cast<int32_t>(width),
                               static_cast<int32_t>(height),
                               static_cast<int32_t>(sampleCount));
}

/**
 * Deletes a Rive render target on the render thread (Phase C.2.3).
 * Enqueues a DeleteRenderTarget command and returns asynchronously via pollMessages.
 *
 * JNI signature: cppDeleteRenderTarget(ptr: Long, requestID: Long, renderTargetHandle: Long): Unit
 *
 * @param env The JNI environment.
 * @param thiz The Java CommandQueue object.
 * @param ptr The native pointer to the CommandServer.
 * @param requestID The request ID for async completion.
 * @param renderTargetHandle The handle of the render target to delete.
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppDeleteRenderTarget(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong renderTargetHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to delete render target on null CommandServer");
        return;
    }

    // Enqueue the delete render target command
    server->deleteRenderTarget(static_cast<int64_t>(requestID),
                               static_cast<int64_t>(renderTargetHandle));
}

/**
 * Enqueues a draw command to render an artboard to a surface.
 *
 * This is a fire-and-forget operation. The actual rendering happens asynchronously
 * on the CommandServer thread with the PLS renderer.
 *
 * JNI signature: cppDraw(ptr: Long, requestID: Long, artboardHandle: Long, smHandle: Long,
 *                        surfacePtr: Long, renderTargetPtr: Long, drawKey: Long,
 *                        surfaceWidth: Int, surfaceHeight: Int, fitMode: Int, alignmentMode: Int,
 *                        clearColor: Int, scaleFactor: Float): Unit
 *
 * @param env The JNI environment.
 * @param thiz The Java CommandQueue object.
 * @param ptr The native pointer to the CommandServer.
 * @param requestID The request ID for async completion.
 * @param artboardHandle Handle to the artboard to draw.
 * @param smHandle Handle to the state machine (0 for static artboards).
 * @param surfacePtr Native EGL surface pointer.
 * @param renderTargetPtr Rive render target pointer.
 * @param drawKey Unique draw operation key for correlation.
 * @param surfaceWidth Surface width in pixels.
 * @param surfaceHeight Surface height in pixels.
 * @param fitMode Fit enum ordinal (0=FILL, 1=CONTAIN, etc.).
 * @param alignmentMode Alignment enum ordinal (0=TOP_LEFT, 1=TOP_CENTER, etc.).
 * @param clearColor Background clear color in 0xAARRGGBB format.
 * @param scaleFactor Scale factor for high DPI displays.
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppDraw(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong requestID,
    jlong artboardHandle,
    jlong smHandle,
    jlong surfacePtr,
    jlong renderTargetPtr,
    jlong drawKey,
    jint surfaceWidth,
    jint surfaceHeight,
    jint fitMode,
    jint alignmentMode,
    jint clearColor,
    jfloat scaleFactor
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to draw on null CommandServer");
        return;
    }

    // Enqueue the draw command
    server->draw(
        static_cast<int64_t>(requestID),
        static_cast<int64_t>(artboardHandle),
        static_cast<int64_t>(smHandle),
        static_cast<int64_t>(surfacePtr),
        static_cast<int64_t>(renderTargetPtr),
        static_cast<int64_t>(drawKey),
        static_cast<int32_t>(surfaceWidth),
        static_cast<int32_t>(surfaceHeight),
        static_cast<int32_t>(fitMode),
        static_cast<int32_t>(alignmentMode),
        static_cast<uint32_t>(clearColor),
        static_cast<float>(scaleFactor)
    );
}

/**
 * Deletes a Rive render target.
 *
 * @param ptr Native pointer to the render target
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_RiveSurface_cppDeleteRenderTarget(
    JNIEnv* env,
    jobject thiz,
    jlong ptr
) {
    if (ptr == 0) {
        return;
    }
    
    LOGD("Deleting Rive render target");
    
    // TODO: In Phase C.2.6, implement actual render target deletion
    // auto* renderTarget = reinterpret_cast<rive::gpu::RenderTargetGL*>(ptr);
    // delete renderTarget;
    
    LOGD("Render target deleted (placeholder)");
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

// =============================================================================
// Phase 0.4: Batch Sprite Rendering
// =============================================================================

/**
 * Draw multiple sprites in a single batch operation.
 *
 * JNI signature: cppDrawMultiple(ptr: Long, renderContextPtr: Long, surfacePtr: Long, 
 *                                drawKey: Long, renderTargetPtr: Long, viewportWidth: Int, 
 *                                viewportHeight: Int, clearColor: Int, 
 *                                artboardHandles: LongArray, stateMachineHandles: LongArray,
 *                                transforms: FloatArray, artboardWidths: FloatArray, 
 *                                artboardHeights: FloatArray, count: Int): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppDrawMultiple(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong renderContextPtr,
    jlong surfacePtr,
    jlong drawKey,
    jlong renderTargetPtr,
    jint viewportWidth,
    jint viewportHeight,
    jint clearColor,
    jlongArray artboardHandles,
    jlongArray stateMachineHandles,
    jfloatArray transforms,
    jfloatArray artboardWidths,
    jfloatArray artboardHeights,
    jint count
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to drawMultiple on null CommandServer");
        return;
    }

    // Get array elements
    jlong* abHandles = env->GetLongArrayElements(artboardHandles, nullptr);
    jlong* smHandles = env->GetLongArrayElements(stateMachineHandles, nullptr);
    jfloat* xforms = env->GetFloatArrayElements(transforms, nullptr);
    jfloat* widths = env->GetFloatArrayElements(artboardWidths, nullptr);
    jfloat* heights = env->GetFloatArrayElements(artboardHeights, nullptr);

    // Convert to vectors for CommandServer
    std::vector<int64_t> abVec(abHandles, abHandles + count);
    std::vector<int64_t> smVec(smHandles, smHandles + count);
    std::vector<float> xformVec(xforms, xforms + count * 6);
    std::vector<float> widthVec(widths, widths + count);
    std::vector<float> heightVec(heights, heights + count);

    // Release array elements
    env->ReleaseLongArrayElements(artboardHandles, abHandles, JNI_ABORT);
    env->ReleaseLongArrayElements(stateMachineHandles, smHandles, JNI_ABORT);
    env->ReleaseFloatArrayElements(transforms, xforms, JNI_ABORT);
    env->ReleaseFloatArrayElements(artboardWidths, widths, JNI_ABORT);
    env->ReleaseFloatArrayElements(artboardHeights, heights, JNI_ABORT);

    // TODO: Implement actual batch rendering in CommandServer
    // server->drawMultiple(renderContextPtr, surfacePtr, drawKey, renderTargetPtr,
    //                      viewportWidth, viewportHeight, clearColor,
    //                      abVec, smVec, xformVec, widthVec, heightVec, count);
    
    LOGD("CommandQueue JNI: drawMultiple called with %d sprites (placeholder)\n", count);
}

/**
 * Draw multiple sprites in a single batch operation and read result to buffer.
 *
 * JNI signature: cppDrawMultipleToBuffer(ptr: Long, renderContextPtr: Long, surfacePtr: Long, 
 *                                        drawKey: Long, renderTargetPtr: Long, viewportWidth: Int, 
 *                                        viewportHeight: Int, clearColor: Int, 
 *                                        artboardHandles: LongArray, stateMachineHandles: LongArray,
 *                                        transforms: FloatArray, artboardWidths: FloatArray, 
 *                                        artboardHeights: FloatArray, count: Int, buffer: ByteArray): Unit
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppDrawMultipleToBuffer(
    JNIEnv* env,
    jobject thiz,
    jlong ptr,
    jlong renderContextPtr,
    jlong surfacePtr,
    jlong drawKey,
    jlong renderTargetPtr,
    jint viewportWidth,
    jint viewportHeight,
    jint clearColor,
    jlongArray artboardHandles,
    jlongArray stateMachineHandles,
    jfloatArray transforms,
    jfloatArray artboardWidths,
    jfloatArray artboardHeights,
    jint count,
    jbyteArray buffer
) {
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server == nullptr) {
        LOGW("CommandQueue JNI: Attempted to drawMultipleToBuffer on null CommandServer");
        return;
    }

    // Get array elements
    jlong* abHandles = env->GetLongArrayElements(artboardHandles, nullptr);
    jlong* smHandles = env->GetLongArrayElements(stateMachineHandles, nullptr);
    jfloat* xforms = env->GetFloatArrayElements(transforms, nullptr);
    jfloat* widths = env->GetFloatArrayElements(artboardWidths, nullptr);
    jfloat* heights = env->GetFloatArrayElements(artboardHeights, nullptr);
    jbyte* bufferPtr = env->GetByteArrayElements(buffer, nullptr);

    // Convert to vectors for CommandServer
    std::vector<int64_t> abVec(abHandles, abHandles + count);
    std::vector<int64_t> smVec(smHandles, smHandles + count);
    std::vector<float> xformVec(xforms, xforms + count * 6);
    std::vector<float> widthVec(widths, widths + count);
    std::vector<float> heightVec(heights, heights + count);

    // TODO: Implement actual batch rendering with buffer readback in CommandServer
    // server->drawMultipleToBuffer(renderContextPtr, surfacePtr, drawKey, renderTargetPtr,
    //                              viewportWidth, viewportHeight, clearColor,
    //                              abVec, smVec, xformVec, widthVec, heightVec, count,
    //                              bufferPtr);
    
    LOGD("CommandQueue JNI: drawMultipleToBuffer called with %d sprites (placeholder)\n", count);

    // Release array elements
    env->ReleaseLongArrayElements(artboardHandles, abHandles, JNI_ABORT);
    env->ReleaseLongArrayElements(stateMachineHandles, smHandles, JNI_ABORT);
    env->ReleaseFloatArrayElements(transforms, xforms, JNI_ABORT);
    env->ReleaseFloatArrayElements(artboardWidths, widths, JNI_ABORT);
    env->ReleaseFloatArrayElements(artboardHeights, heights, JNI_ABORT);
    env->ReleaseByteArrayElements(buffer, bufferPtr, 0);  // Copy back changes
}

} // extern "C"
