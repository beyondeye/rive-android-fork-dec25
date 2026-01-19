#include "bindings_commandqueue_internal.hpp"

// Cached method IDs for callbacks - definitions
jmethodID g_onFileLoadedMethodID = nullptr;
jmethodID g_onFileErrorMethodID = nullptr;
jmethodID g_onFileDeletedMethodID = nullptr;
jmethodID g_onArtboardNamesListedMethodID = nullptr;
jmethodID g_onStateMachineNamesListedMethodID = nullptr;
jmethodID g_onViewModelNamesListedMethodID = nullptr;
// Phase E.2: File Introspection APIs
jmethodID g_onViewModelInstanceNamesListedMethodID = nullptr;
jmethodID g_onViewModelPropertiesListedMethodID = nullptr;
jmethodID g_onEnumsListedMethodID = nullptr;
jmethodID g_onQueryErrorMethodID = nullptr;
jmethodID g_onArtboardCreatedMethodID = nullptr;
jmethodID g_onArtboardErrorMethodID = nullptr;
jmethodID g_onArtboardDeletedMethodID = nullptr;
jmethodID g_onStateMachineCreatedMethodID = nullptr;
jmethodID g_onStateMachineErrorMethodID = nullptr;
jmethodID g_onStateMachineDeletedMethodID = nullptr;
jmethodID g_onStateMachineSettledMethodID = nullptr;
// Input operation callbacks
jmethodID g_onInputCountResultMethodID = nullptr;
jmethodID g_onInputNamesListedMethodID = nullptr;
jmethodID g_onInputInfoResultMethodID = nullptr;
jmethodID g_onNumberInputValueMethodID = nullptr;
jmethodID g_onBooleanInputValueMethodID = nullptr;
jmethodID g_onInputOperationSuccessMethodID = nullptr;
jmethodID g_onInputOperationErrorMethodID = nullptr;
// ViewModelInstance callbacks
jmethodID g_onVMICreatedMethodID = nullptr;
jmethodID g_onVMIErrorMethodID = nullptr;
jmethodID g_onVMIDeletedMethodID = nullptr;
// Property operation callbacks (Phase D.2)
jmethodID g_onNumberPropertyValueMethodID = nullptr;
jmethodID g_onStringPropertyValueMethodID = nullptr;
jmethodID g_onBooleanPropertyValueMethodID = nullptr;
jmethodID g_onPropertyErrorMethodID = nullptr;
jmethodID g_onPropertySetSuccessMethodID = nullptr;
// Additional property callbacks (Phase D.3)
jmethodID g_onEnumPropertyValueMethodID = nullptr;
jmethodID g_onColorPropertyValueMethodID = nullptr;
jmethodID g_onTriggerFiredMethodID = nullptr;
// Property subscription update callbacks (Phase D.4)
jmethodID g_onNumberPropertyUpdatedMethodID = nullptr;
jmethodID g_onStringPropertyUpdatedMethodID = nullptr;
jmethodID g_onBooleanPropertyUpdatedMethodID = nullptr;
jmethodID g_onEnumPropertyUpdatedMethodID = nullptr;
jmethodID g_onColorPropertyUpdatedMethodID = nullptr;
jmethodID g_onTriggerPropertyFiredMethodID = nullptr;
// List operation callbacks (Phase D.5)
jmethodID g_onListSizeResultMethodID = nullptr;
jmethodID g_onListItemResultMethodID = nullptr;
jmethodID g_onListOperationSuccessMethodID = nullptr;
jmethodID g_onListOperationErrorMethodID = nullptr;
// Nested VMI operation callbacks (Phase D.5)
jmethodID g_onInstancePropertyResultMethodID = nullptr;
jmethodID g_onInstancePropertySetSuccessMethodID = nullptr;
jmethodID g_onInstancePropertyErrorMethodID = nullptr;
// Asset property operation callbacks (Phase D.5)
jmethodID g_onAssetPropertySetSuccessMethodID = nullptr;
jmethodID g_onAssetPropertyErrorMethodID = nullptr;
// VMI binding operation callbacks (Phase D.6)
jmethodID g_onVMIBindingSuccessMethodID = nullptr;
jmethodID g_onVMIBindingErrorMethodID = nullptr;
jmethodID g_onDefaultVMIResultMethodID = nullptr;
jmethodID g_onDefaultVMIErrorMethodID = nullptr;
// Render target operation callbacks (Phase C.2.3)
jmethodID g_onRenderTargetCreatedMethodID = nullptr;
jmethodID g_onRenderTargetErrorMethodID = nullptr;
jmethodID g_onRenderTargetDeletedMethodID = nullptr;
// Asset operation callbacks (Phase E.1)
jmethodID g_onImageDecodedMethodID = nullptr;
jmethodID g_onImageErrorMethodID = nullptr;
jmethodID g_onAudioDecodedMethodID = nullptr;
jmethodID g_onAudioErrorMethodID = nullptr;
jmethodID g_onFontDecodedMethodID = nullptr;
jmethodID g_onFontErrorMethodID = nullptr;

/**
 * Initialize cached method IDs for JNI callbacks.
 * This is called once during first use to avoid repeated JNI lookups.
 */
void initCallbackMethodIDs(JNIEnv* env, jobject commandQueue) {
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

    // Phase E.2: File Introspection APIs
    g_onViewModelInstanceNamesListedMethodID = env->GetMethodID(
        commandQueueClass,
        "onViewModelInstanceNamesListed",
        "(JLjava/util/List;)V"  // (requestID: Long, names: List<String>) -> Unit
    );

    g_onViewModelPropertiesListedMethodID = env->GetMethodID(
        commandQueueClass,
        "onViewModelPropertiesListed",
        "(JLjava/util/List;)V"  // (requestID: Long, properties: List<ViewModelProperty>) -> Unit
    );

    g_onEnumsListedMethodID = env->GetMethodID(
        commandQueueClass,
        "onEnumsListed",
        "(JLjava/util/List;)V"  // (requestID: Long, enums: List<RiveEnum>) -> Unit
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

    // Phase E.1: Asset operation callbacks
    g_onImageDecodedMethodID = env->GetMethodID(
        commandQueueClass,
        "onImageDecoded",
        "(JJ)V"  // (requestID: Long, imageHandle: Long) -> Unit
    );

    g_onImageErrorMethodID = env->GetMethodID(
        commandQueueClass,
        "onImageError",
        "(JLjava/lang/String;)V"  // (requestID: Long, error: String) -> Unit
    );

    g_onAudioDecodedMethodID = env->GetMethodID(
        commandQueueClass,
        "onAudioDecoded",
        "(JJ)V"  // (requestID: Long, audioHandle: Long) -> Unit
    );

    g_onAudioErrorMethodID = env->GetMethodID(
        commandQueueClass,
        "onAudioError",
        "(JLjava/lang/String;)V"  // (requestID: Long, error: String) -> Unit
    );

    g_onFontDecodedMethodID = env->GetMethodID(
        commandQueueClass,
        "onFontDecoded",
        "(JJ)V"  // (requestID: Long, fontHandle: Long) -> Unit
    );

    g_onFontErrorMethodID = env->GetMethodID(
        commandQueueClass,
        "onFontError",
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
            case rive_android::MessageType::ViewModelInstanceNamesListed:
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
                    } else if (msg.type == rive_android::MessageType::ViewModelInstanceNamesListed) {
                        env->CallVoidMethod(receiver, g_onViewModelInstanceNamesListedMethodID, 
                            static_cast<jlong>(msg.requestID), arrayList);
                    }
                    
                    env->DeleteLocalRef(arrayList);
                    env->DeleteLocalRef(arrayListClass);
                }
                break;

            // Phase E.2: ViewModel Properties (stringList contains alternating name/type pairs)
            case rive_android::MessageType::ViewModelPropertiesListed:
                {
                    // Convert std::vector<std::string> to Java List<String>
                    // The Kotlin side will parse name/type pairs into ViewModelProperty objects
                    jclass arrayListClass = env->FindClass("java/util/ArrayList");
                    jmethodID arrayListConstructor = env->GetMethodID(arrayListClass, "<init>", "()V");
                    jmethodID arrayListAdd = env->GetMethodID(arrayListClass, "add", "(Ljava/lang/Object;)Z");
                    
                    jobject arrayList = env->NewObject(arrayListClass, arrayListConstructor);
                    
                    for (const auto& str : msg.stringList) {
                        jstring strVal = env->NewStringUTF(str.c_str());
                        env->CallBooleanMethod(arrayList, arrayListAdd, strVal);
                        env->DeleteLocalRef(strVal);
                    }
                    
                    env->CallVoidMethod(receiver, g_onViewModelPropertiesListedMethodID, 
                        static_cast<jlong>(msg.requestID), arrayList);
                    
                    env->DeleteLocalRef(arrayList);
                    env->DeleteLocalRef(arrayListClass);
                }
                break;

            // Phase E.2: Enums (stringList contains enum data: name, valueCount, value1, value2, ...)
            case rive_android::MessageType::EnumsListed:
                {
                    // Convert std::vector<std::string> to Java List<String>
                    // The Kotlin side will parse the structured data into RiveEnum objects
                    jclass arrayListClass = env->FindClass("java/util/ArrayList");
                    jmethodID arrayListConstructor = env->GetMethodID(arrayListClass, "<init>", "()V");
                    jmethodID arrayListAdd = env->GetMethodID(arrayListClass, "add", "(Ljava/lang/Object;)Z");
                    
                    jobject arrayList = env->NewObject(arrayListClass, arrayListConstructor);
                    
                    for (const auto& str : msg.stringList) {
                        jstring strVal = env->NewStringUTF(str.c_str());
                        env->CallBooleanMethod(arrayList, arrayListAdd, strVal);
                        env->DeleteLocalRef(strVal);
                    }
                    
                    env->CallVoidMethod(receiver, g_onEnumsListedMethodID, 
                        static_cast<jlong>(msg.requestID), arrayList);
                    
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

            // Phase E.1: Asset operation results
            case rive_android::MessageType::ImageDecoded:
                env->CallVoidMethod(receiver, g_onImageDecodedMethodID,
                    static_cast<jlong>(msg.requestID),
                    static_cast<jlong>(msg.handle));
                break;

            case rive_android::MessageType::ImageError:
                {
                    jstring errorStr = env->NewStringUTF(msg.error.c_str());
                    env->CallVoidMethod(receiver, g_onImageErrorMethodID,
                        static_cast<jlong>(msg.requestID),
                        errorStr);
                    env->DeleteLocalRef(errorStr);
                }
                break;

            case rive_android::MessageType::AudioDecoded:
                env->CallVoidMethod(receiver, g_onAudioDecodedMethodID,
                    static_cast<jlong>(msg.requestID),
                    static_cast<jlong>(msg.handle));
                break;

            case rive_android::MessageType::AudioError:
                {
                    jstring errorStr = env->NewStringUTF(msg.error.c_str());
                    env->CallVoidMethod(receiver, g_onAudioErrorMethodID,
                        static_cast<jlong>(msg.requestID),
                        errorStr);
                    env->DeleteLocalRef(errorStr);
                }
                break;

            case rive_android::MessageType::FontDecoded:
                env->CallVoidMethod(receiver, g_onFontDecodedMethodID,
                    static_cast<jlong>(msg.requestID),
                    static_cast<jlong>(msg.handle));
                break;

            case rive_android::MessageType::FontError:
                {
                    jstring errorStr = env->NewStringUTF(msg.error.c_str());
                    env->CallVoidMethod(receiver, g_onFontErrorMethodID,
                        static_cast<jlong>(msg.requestID),
                        errorStr);
                    env->DeleteLocalRef(errorStr);
                }
                break;

            // Phase C.2.6: Draw operation results (fire-and-forget, just log)
            case rive_android::MessageType::DrawComplete:
                LOGI("CommandQueue JNI: Draw completed (drawKey=%lld)", static_cast<long long>(msg.handle));
                break;

            case rive_android::MessageType::DrawError:
                {
                    LOGW("CommandQueue JNI: Draw error: %s", msg.error.c_str());
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

} // extern "C"