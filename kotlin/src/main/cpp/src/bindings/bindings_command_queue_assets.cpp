/**
 * Command Queue Asset Bindings
 * 
 * This file contains JNI bindings for:
 * - Image decoding, deletion, registration, and unregistration
 * - Audio decoding, deletion, registration, and unregistration
 * - Font decoding, deletion, registration, and unregistration
 */

#include "bindings_command_queue.hpp"

using namespace rive_android;
using namespace rive_android::bindings;

extern "C"
{
    // =========================================================================
    // Image Assets
    // =========================================================================

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppDecodeImage(JNIEnv* env,
                                                            jobject,
                                                            jlong ref,
                                                            jlong requestID,
                                                            jbyteArray bytes)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto byteVec = ByteArrayToUint8Vec(env, bytes);

        commandQueue->decodeImage(byteVec, nullptr, requestID);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppDeleteImage(JNIEnv*,
                                                            jobject,
                                                            jlong ref,
                                                            jlong jImageHandle)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto imageHandle =
            handleFromLong<rive::RenderImageHandle>(jImageHandle);

        commandQueue->deleteImage(imageHandle);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppRegisterImage(
        JNIEnv* env,
        jobject,
        jlong ref,
        jstring jPath,
        jlong jImageHandle)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto path = JStringToString(env, jPath);
        auto imageHandle =
            handleFromLong<rive::RenderImageHandle>(jImageHandle);

        commandQueue->addGlobalImageAsset(path, imageHandle);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppUnregisterImage(JNIEnv* env,
                                                                jobject,
                                                                jlong ref,
                                                                jstring jPath)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto path = JStringToString(env, jPath);

        commandQueue->removeGlobalImageAsset(path);
    }

    // =========================================================================
    // Audio Assets
    // =========================================================================

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppDecodeAudio(JNIEnv* env,
                                                            jobject,
                                                            jlong ref,
                                                            jlong requestID,
                                                            jbyteArray bytes)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto byteVec = ByteArrayToUint8Vec(env, bytes);

        commandQueue->decodeAudio(byteVec, nullptr, requestID);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppDeleteAudio(JNIEnv*,
                                                            jobject,
                                                            jlong ref,
                                                            jlong jAudioHandle)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto audioHandle =
            handleFromLong<rive::AudioSourceHandle>(jAudioHandle);

        commandQueue->deleteAudio(audioHandle);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppRegisterAudio(
        JNIEnv* env,
        jobject,
        jlong ref,
        jstring jPath,
        jlong jAudioHandle)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto path = JStringToString(env, jPath);
        auto audioHandle =
            handleFromLong<rive::AudioSourceHandle>(jAudioHandle);

        commandQueue->addGlobalAudioAsset(path, audioHandle);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppUnregisterAudio(JNIEnv* env,
                                                                jobject,
                                                                jlong ref,
                                                                jstring jPath)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto path = JStringToString(env, jPath);

        commandQueue->removeGlobalAudioAsset(path);
    }

    // =========================================================================
    // Font Assets
    // =========================================================================

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppDecodeFont(JNIEnv* env,
                                                           jobject,
                                                           jlong ref,
                                                           jlong requestID,
                                                           jbyteArray bytes)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto byteVec = ByteArrayToUint8Vec(env, bytes);

        commandQueue->decodeFont(byteVec, nullptr, requestID);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppDeleteFont(JNIEnv*,
                                                           jobject,
                                                           jlong ref,
                                                           jlong jFontHandle)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto fontHandle = handleFromLong<rive::FontHandle>(jFontHandle);

        commandQueue->deleteFont(fontHandle);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppRegisterFont(JNIEnv* env,
                                                             jobject,
                                                             jlong ref,
                                                             jstring jPath,
                                                             jlong jFontHandle)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto path = JStringToString(env, jPath);
        auto fontHandle = handleFromLong<rive::FontHandle>(jFontHandle);

        commandQueue->addGlobalFontAsset(path, fontHandle);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppUnregisterFont(JNIEnv* env,
                                                               jobject,
                                                               jlong ref,
                                                               jstring jPath)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto path = JStringToString(env, jPath);

        commandQueue->removeGlobalFontAsset(path);
    }
}