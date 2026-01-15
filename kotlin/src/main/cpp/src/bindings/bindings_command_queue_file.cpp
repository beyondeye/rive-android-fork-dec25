/**
 * Command Queue File and Artboard Bindings
 * 
 * This file contains JNI bindings for:
 * - File loading and deletion
 * - Artboard names listing
 * - Artboard creation and deletion
 * - Artboard resizing
 * - ViewModel-related file queries
 */

#include "bindings_command_queue.hpp"

using namespace rive_android;
using namespace rive_android::bindings;

extern "C"
{
    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppLoadFile(JNIEnv* env,
                                                         jobject,
                                                         jlong ref,
                                                         jlong requestID,
                                                         jbyteArray bytes)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto byteVec = ByteArrayToUint8Vec(env, bytes);

        commandQueue->loadFile(byteVec, nullptr, requestID);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppDeleteFile(JNIEnv*,
                                                           jobject,
                                                           jlong ref,
                                                           jlong requestID,
                                                           jlong jFileHandle)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        commandQueue->deleteFile(handleFromLong<rive::FileHandle>(jFileHandle),
                                 requestID);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppGetArtboardNames(
        JNIEnv*,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jFileHandle)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto fileHandle = handleFromLong<rive::FileHandle>(jFileHandle);

        commandQueue->requestArtboardNames(fileHandle, requestID);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppGetStateMachineNames(
        JNIEnv*,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jArtboardHandle)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto artboardHandle =
            handleFromLong<rive::ArtboardHandle>(jArtboardHandle);

        commandQueue->requestStateMachineNames(artboardHandle, requestID);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppGetViewModelNames(
        JNIEnv*,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jFileHandle)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto fileHandle = handleFromLong<rive::FileHandle>(jFileHandle);

        commandQueue->requestViewModelNames(fileHandle, requestID);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppGetViewModelInstanceNames(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jFileHandle,
        jstring jViewModelName)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto name = JStringToString(env, jViewModelName);
        auto fileHandle = handleFromLong<rive::FileHandle>(jFileHandle);

        commandQueue->requestViewModelInstanceNames(fileHandle,
                                                    name,
                                                    requestID);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppGetViewModelProperties(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jFileHandle,
        jstring jViewModelName)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto name = JStringToString(env, jViewModelName);
        auto fileHandle = handleFromLong<rive::FileHandle>(jFileHandle);

        commandQueue->requestViewModelPropertyDefinitions(fileHandle,
                                                          name,
                                                          requestID);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppGetEnums(JNIEnv*,
                                                         jobject,
                                                         jlong ref,
                                                         jlong requestID,
                                                         jlong jFileHandle)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto fileHandle = handleFromLong<rive::FileHandle>(jFileHandle);

        commandQueue->requestViewModelEnums(fileHandle, requestID);
    }

    JNIEXPORT jlong JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppCreateDefaultArtboard(
        JNIEnv*,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jFileHandle)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);

        auto artboard = commandQueue->instantiateDefaultArtboard(
            handleFromLong<rive::FileHandle>(jFileHandle),
            nullptr,
            requestID);
        return longFromHandle(artboard);
    }

    JNIEXPORT jlong JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppCreateArtboardByName(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jFileHandle,
        jstring name)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto nativeName = JStringToString(env, name);

        auto artboard = commandQueue->instantiateArtboardNamed(
            handleFromLong<rive::FileHandle>(jFileHandle),
            nativeName,
            nullptr,
            requestID);
        return longFromHandle(artboard);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppDeleteArtboard(
        JNIEnv*,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jArtboardHandle)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        commandQueue->deleteArtboard(
            handleFromLong<rive::ArtboardHandle>(jArtboardHandle),
            requestID);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppResizeArtboard(
        JNIEnv*,
        jobject,
        jlong ref,
        jlong jArtboardHandle,
        jint jWidth,
        jint jHeight,
        jfloat jScaleFactor)
    {
        auto* commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto artboardHandle =
            handleFromLong<rive::ArtboardHandle>(jArtboardHandle);
        auto width = static_cast<float_t>(jWidth);
        auto height = static_cast<float_t>(jHeight);
        auto scaleFactor = static_cast<float_t>(jScaleFactor);

        commandQueue->setArtboardSize(artboardHandle,
                                      width,
                                      height,
                                      scaleFactor);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppResetArtboardSize(
        JNIEnv*,
        jobject,
        jlong ref,
        jlong jArtboardHandle)
    {
        auto* commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto artboardHandle =
            handleFromLong<rive::ArtboardHandle>(jArtboardHandle);

        commandQueue->resetArtboardSize(artboardHandle);
    }
}