/**
 * Command Queue ViewModel Instance Bindings
 * 
 * This file contains JNI bindings for:
 * - ViewModel instance creation (blank, default, named, nested, list item)
 * - ViewModel instance deletion and binding
 * - Property get/set operations (number, string, boolean, enum, color, trigger, image, artboard)
 * - Property subscription/unsubscription
 * - List operations (size, insert, append, remove, swap)
 */

#include "bindings_command_queue.hpp"

using namespace rive_android;
using namespace rive_android::bindings;

extern "C"
{
    // =========================================================================
    // ViewModel Instance Creation
    // =========================================================================

    JNIEXPORT jlong JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppNamedVMCreateBlankVMI(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jFileHandle,
        jstring jViewModelName)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto fileHandle = handleFromLong<rive::FileHandle>(jFileHandle);
        auto viewModelName = JStringToString(env, jViewModelName);

        auto viewModelInstance =
            commandQueue->instantiateBlankViewModelInstance(fileHandle,
                                                            viewModelName,
                                                            nullptr,
                                                            requestID);
        return longFromHandle(viewModelInstance);
    }

    JNIEXPORT jlong JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppDefaultVMCreateBlankVMI(
        JNIEnv*,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jFileHandle,
        jlong jArtboardHandle)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto fileHandle = handleFromLong<rive::FileHandle>(jFileHandle);
        auto artboardHandle =
            handleFromLong<rive::ArtboardHandle>(jArtboardHandle);

        auto viewModelInstance =
            commandQueue->instantiateBlankViewModelInstance(fileHandle,
                                                            artboardHandle,
                                                            nullptr,
                                                            requestID);
        return longFromHandle(viewModelInstance);
    }

    JNIEXPORT jlong JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppNamedVMCreateDefaultVMI(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jFileHandle,
        jstring jViewModelName)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto fileHandle = handleFromLong<rive::FileHandle>(jFileHandle);
        auto viewModelName = JStringToString(env, jViewModelName);

        auto viewModelInstance =
            commandQueue->instantiateDefaultViewModelInstance(fileHandle,
                                                              viewModelName,
                                                              nullptr,
                                                              requestID);
        return longFromHandle(viewModelInstance);
    }

    JNIEXPORT jlong JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppDefaultVMCreateDefaultVMI(
        JNIEnv*,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jFileHandle,
        jlong jArtboardHandle)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto fileHandle = handleFromLong<rive::FileHandle>(jFileHandle);
        auto artboardHandle =
            handleFromLong<rive::ArtboardHandle>(jArtboardHandle);

        auto viewModelInstance =
            commandQueue->instantiateDefaultViewModelInstance(fileHandle,
                                                              artboardHandle,
                                                              nullptr,
                                                              requestID);
        return longFromHandle(viewModelInstance);
    }

    JNIEXPORT jlong JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppNamedVMCreateNamedVMI(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jFileHandle,
        jstring jViewModelName,
        jstring jInstanceName)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto fileHandle = handleFromLong<rive::FileHandle>(jFileHandle);
        auto viewModelName = JStringToString(env, jViewModelName);
        auto instanceName = JStringToString(env, jInstanceName);

        auto viewModelInstance =
            commandQueue->instantiateViewModelInstanceNamed(fileHandle,
                                                            viewModelName,
                                                            instanceName,
                                                            nullptr,
                                                            requestID);
        return longFromHandle(viewModelInstance);
    }

    JNIEXPORT jlong JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppDefaultVMCreateNamedVMI(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jFileHandle,
        jlong jArtboardHandle,
        jstring jInstanceName)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto fileHandle = handleFromLong<rive::FileHandle>(jFileHandle);
        auto artboardHandle =
            handleFromLong<rive::ArtboardHandle>(jArtboardHandle);
        auto instanceName = JStringToString(env, jInstanceName);

        auto viewModelInstance =
            commandQueue->instantiateViewModelInstanceNamed(fileHandle,
                                                            artboardHandle,
                                                            instanceName,
                                                            nullptr,
                                                            requestID);
        return longFromHandle(viewModelInstance);
    }

    JNIEXPORT jlong JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppReferenceNestedVMI(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jViewModelInstanceHandle,
        jstring jPath)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto viewModelInstanceHandle =
            handleFromLong<rive::ViewModelInstanceHandle>(
                jViewModelInstanceHandle);
        auto path = JStringToString(env, jPath);

        auto nestedViewModelInstance =
            commandQueue->referenceNestedViewModelInstance(
                viewModelInstanceHandle,
                path,
                nullptr,
                requestID);
        return longFromHandle(nestedViewModelInstance);
    }

    JNIEXPORT jlong JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppReferenceListItemVMI(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jViewModelInstanceHandle,
        jstring jPath,
        jint index)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto viewModelInstanceHandle =
            handleFromLong<rive::ViewModelInstanceHandle>(
                jViewModelInstanceHandle);
        auto path = JStringToString(env, jPath);

        auto nestedViewModelInstance =
            commandQueue->referenceListViewModelInstance(
                viewModelInstanceHandle,
                path,
                static_cast<int32_t>(index),
                nullptr,
                requestID);
        return longFromHandle(nestedViewModelInstance);
    }

    // =========================================================================
    // ViewModel Instance Management
    // =========================================================================

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppDeleteViewModelInstance(
        JNIEnv*,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jViewModelInstanceHandle)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        commandQueue->deleteViewModelInstance(
            handleFromLong<rive::ViewModelInstanceHandle>(
                jViewModelInstanceHandle),
            requestID);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppBindViewModelInstance(
        JNIEnv*,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jStateMachineHandle,
        jlong jViewModelInstanceHandle)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto stateMachineHandle =
            handleFromLong<rive::StateMachineHandle>(jStateMachineHandle);
        auto viewModelInstanceHandle =
            handleFromLong<rive::ViewModelInstanceHandle>(
                jViewModelInstanceHandle);

        commandQueue->bindViewModelInstance(stateMachineHandle,
                                            viewModelInstanceHandle,
                                            requestID);
    }

    // =========================================================================
    // Property Setters
    // =========================================================================

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppSetNumberProperty(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong jViewModelInstanceHandle,
        jstring jPropertyPath,
        jfloat value)
    {
        setProperty(env,
                    ref,
                    jViewModelInstanceHandle,
                    jPropertyPath,
                    value,
                    &rive::CommandQueue::setViewModelInstanceNumber);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppSetStringProperty(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong jViewModelInstanceHandle,
        jstring jPropertyPath,
        jstring jValue)
    {
        auto value = JStringToString(env, jValue);
        setProperty(env,
                    ref,
                    jViewModelInstanceHandle,
                    jPropertyPath,
                    value,
                    &rive::CommandQueue::setViewModelInstanceString);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppSetBooleanProperty(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong jViewModelInstanceHandle,
        jstring jPropertyPath,
        jboolean jValue)
    {
        setProperty<bool>(env,
                          ref,
                          jViewModelInstanceHandle,
                          jPropertyPath,
                          jValue,
                          &rive::CommandQueue::setViewModelInstanceBool);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppSetEnumProperty(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong jViewModelInstanceHandle,
        jstring jPropertyPath,
        jstring jValue)
    {
        auto value = JStringToString(env, jValue);
        setProperty(env,
                    ref,
                    jViewModelInstanceHandle,
                    jPropertyPath,
                    value,
                    &rive::CommandQueue::setViewModelInstanceEnum);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppSetColorProperty(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong jViewModelInstanceHandle,
        jstring jPropertyPath,
        jint jValue)
    {
        // ColorInt is uint32_t in C++
        auto value = static_cast<uint32_t>(jValue);
        setProperty(env,
                    ref,
                    jViewModelInstanceHandle,
                    jPropertyPath,
                    value,
                    &rive::CommandQueue::setViewModelInstanceColor);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppFireTriggerProperty(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong jViewModelInstanceHandle,
        jstring jPropertyPath)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto viewModelInstanceHandle =
            handleFromLong<rive::ViewModelInstanceHandle>(
                jViewModelInstanceHandle);
        auto propertyPath = JStringToString(env, jPropertyPath);

        commandQueue->fireViewModelTrigger(viewModelInstanceHandle,
                                           propertyPath);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppSetImageProperty(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong jViewModelInstanceHandle,
        jstring jPropertyPath,
        jlong jImageHandle)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto viewModelInstanceHandle =
            handleFromLong<rive::ViewModelInstanceHandle>(
                jViewModelInstanceHandle);
        auto propertyPath = JStringToString(env, jPropertyPath);
        auto imageHandle =
            handleFromLong<rive::RenderImageHandle>(jImageHandle);

        commandQueue->setViewModelInstanceImage(viewModelInstanceHandle,
                                                propertyPath,
                                                imageHandle);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppSetArtboardProperty(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong jViewModelInstanceHandle,
        jstring jPropertyPath,
        jlong jArtboardHandle)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto viewModelInstanceHandle =
            handleFromLong<rive::ViewModelInstanceHandle>(
                jViewModelInstanceHandle);
        auto propertyPath = JStringToString(env, jPropertyPath);
        auto artboardHandle =
            handleFromLong<rive::ArtboardHandle>(jArtboardHandle);

        commandQueue->setViewModelInstanceArtboard(viewModelInstanceHandle,
                                                   propertyPath,
                                                   artboardHandle);
    }

    // =========================================================================
    // Property Getters
    // =========================================================================

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppGetNumberProperty(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jViewModelInstanceHandle,
        jstring jPropertyPath)
    {
        getProperty(env,
                    ref,
                    requestID,
                    jViewModelInstanceHandle,
                    jPropertyPath,
                    &rive::CommandQueue::requestViewModelInstanceNumber);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppGetStringProperty(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jViewModelInstanceHandle,
        jstring jPropertyPath)
    {
        getProperty(env,
                    ref,
                    requestID,
                    jViewModelInstanceHandle,
                    jPropertyPath,
                    &rive::CommandQueue::requestViewModelInstanceString);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppGetBooleanProperty(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jViewModelInstanceHandle,
        jstring jPropertyPath)
    {
        getProperty(env,
                    ref,
                    requestID,
                    jViewModelInstanceHandle,
                    jPropertyPath,
                    &rive::CommandQueue::requestViewModelInstanceBool);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppGetEnumProperty(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jViewModelInstanceHandle,
        jstring jPropertyPath)
    {
        getProperty(env,
                    ref,
                    requestID,
                    jViewModelInstanceHandle,
                    jPropertyPath,
                    &rive::CommandQueue::requestViewModelInstanceEnum);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppGetColorProperty(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jViewModelInstanceHandle,
        jstring jPropertyPath)
    {
        getProperty(env,
                    ref,
                    requestID,
                    jViewModelInstanceHandle,
                    jPropertyPath,
                    &rive::CommandQueue::requestViewModelInstanceColor);
    }

    // =========================================================================
    // Property Subscriptions
    // =========================================================================

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppSubscribeToProperty(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong jViewModelInstanceHandle,
        jstring jPropertyPath,
        jint propertyType)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto viewModelInstanceHandle =
            handleFromLong<rive::ViewModelInstanceHandle>(
                jViewModelInstanceHandle);
        auto propertyPath = JStringToString(env, jPropertyPath);
        auto dataType = static_cast<rive::DataType>(propertyType);

        commandQueue->subscribeToViewModelProperty(viewModelInstanceHandle,
                                                   propertyPath,
                                                   dataType);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppUnsubscribeFromProperty(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong jViewModelInstanceHandle,
        jstring jPropertyPath,
        jint propertyType)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto viewModelInstanceHandle =
            handleFromLong<rive::ViewModelInstanceHandle>(
                jViewModelInstanceHandle);
        auto propertyPath = JStringToString(env, jPropertyPath);
        auto dataType = static_cast<rive::DataType>(propertyType);

        commandQueue->unsubscribeToViewModelProperty(viewModelInstanceHandle,
                                                     propertyPath,
                                                     dataType);
    }

    // =========================================================================
    // List Operations
    // =========================================================================

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppGetListSize(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jViewModelInstanceHandle,
        jstring jPropertyPath)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto viewModelInstanceHandle =
            handleFromLong<rive::ViewModelInstanceHandle>(
                jViewModelInstanceHandle);
        auto propertyPath = JStringToString(env, jPropertyPath);

        commandQueue->requestViewModelInstanceListSize(viewModelInstanceHandle,
                                                       propertyPath,
                                                       requestID);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppInsertToListAtIndex(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong jViewModelInstanceHandle,
        jstring jPropertyPath,
        jint index,
        jlong jItemHandle)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto viewModelInstanceHandle =
            handleFromLong<rive::ViewModelInstanceHandle>(
                jViewModelInstanceHandle);
        auto propertyPath = JStringToString(env, jPropertyPath);
        auto itemHandle =
            handleFromLong<rive::ViewModelInstanceHandle>(jItemHandle);

        commandQueue->insertViewModelInstanceListViewModel(
            viewModelInstanceHandle,
            propertyPath,
            itemHandle,
            static_cast<int32_t>(index));
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppAppendToList(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong jViewModelInstanceHandle,
        jstring jPropertyPath,
        jlong jItemHandle)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto viewModelInstanceHandle =
            handleFromLong<rive::ViewModelInstanceHandle>(
                jViewModelInstanceHandle);
        auto propertyPath = JStringToString(env, jPropertyPath);
        auto itemHandle =
            handleFromLong<rive::ViewModelInstanceHandle>(jItemHandle);

        commandQueue->appendViewModelInstanceListViewModel(
            viewModelInstanceHandle,
            propertyPath,
            itemHandle);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppRemoveFromListAtIndex(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong jViewModelInstanceHandle,
        jstring jPropertyPath,
        jint index)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto viewModelInstanceHandle =
            handleFromLong<rive::ViewModelInstanceHandle>(
                jViewModelInstanceHandle);
        auto propertyPath = JStringToString(env, jPropertyPath);

        commandQueue->removeViewModelInstanceListViewModel(
            viewModelInstanceHandle,
            propertyPath,
            static_cast<int32_t>(index));
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppRemoveFromList(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong jViewModelInstanceHandle,
        jstring jPropertyPath,
        jlong jItemHandle)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto viewModelInstanceHandle =
            handleFromLong<rive::ViewModelInstanceHandle>(
                jViewModelInstanceHandle);
        auto propertyPath = JStringToString(env, jPropertyPath);
        auto itemHandle =
            handleFromLong<rive::ViewModelInstanceHandle>(jItemHandle);

        commandQueue->removeViewModelInstanceListViewModel(
            viewModelInstanceHandle,
            propertyPath,
            itemHandle);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppSwapListItems(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong jViewModelInstanceHandle,
        jstring jPropertyPath,
        jint indexA,
        jint indexB)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto viewModelInstanceHandle =
            handleFromLong<rive::ViewModelInstanceHandle>(
                jViewModelInstanceHandle);
        auto propertyPath = JStringToString(env, jPropertyPath);

        commandQueue->swapViewModelInstanceListValues(
            viewModelInstanceHandle,
            propertyPath,
            static_cast<int32_t>(indexA),
            static_cast<int32_t>(indexB));
    }
}