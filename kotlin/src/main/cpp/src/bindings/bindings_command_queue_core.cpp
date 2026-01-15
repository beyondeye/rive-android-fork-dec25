/**
 * Command Queue Core Lifecycle Bindings
 * 
 * This file contains JNI bindings for:
 * - CommandQueue constructor and destructor
 * - Listener creation and deletion
 * - Message polling
 */

#include "bindings_command_queue.hpp"

using namespace rive_android;
using namespace rive_android::bindings;

extern "C"
{
    JNIEXPORT jlong JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppConstructor(
        JNIEnv* env,
        jobject,
        jlong renderContextPtr)
    {
        auto* renderContext =
            reinterpret_cast<RenderContext*>(renderContextPtr);

        // Used by the CommandServer thread to signal startup success or failure
        std::promise<StartupResult> promise;
        std::future<StartupResult> resultFuture = promise.get_future();

        /* Create a command queue with an owned thread handle (ref count 1).
         * The command server thread will also own 1 after calling
         * startCommandServer.
         * This RCP is released here, and its ref is deleted in in cppDelete. */
        auto commandQueue =
            rive::rcp<CommandQueueWithThread>(new CommandQueueWithThread());
        // Start the C++ thread that drives the CommandServer
        commandQueue->startCommandServer(renderContext, std::move(promise));

        // Wait for the command server to start, blocking the main thread, and
        // return the result
        auto result = resultFuture.get();
        if (!result.success)
        {
            // Surface error to Java
            auto jRiveInitializationExceptionClass =
                FindClass(env, "app/rive/RiveInitializationException");
            char messageBuffer[256];
            snprintf(messageBuffer,
                     sizeof(messageBuffer),
                     "CommandQueue startup failed (EGL 0x%04x): %s",
                     result.errorCode,
                     result.message.c_str());
            env->ThrowNew(jRiveInitializationExceptionClass.get(),
                          messageBuffer);

            // Return null pointer
            return 0L;
        }

        return reinterpret_cast<jlong>(commandQueue.release());
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppDelete(JNIEnv*,
                                                       jobject,
                                                       jlong ref)
    {
        auto commandQueue = reinterpret_cast<CommandQueueWithThread*>(ref);
        // Blocks the calling thread until the command server thread shuts down
        commandQueue->shutdownAndJoin();

        // Second unref, matches the RefCnt constructor's default of 1 from
        // cppConstructor
        auto refCnt = commandQueue->debugging_refcnt();
        // Log any unexpected ref counts
        if (refCnt != 1)
        {
            RiveLogW(
                TAG_CQ,
                "Command queue ref count before cppDelete unref does not match expected value:\n"
                "  Expected: 1; Actual: %d\n"
                "    1. This main thread's released reference",
                refCnt);
            if (refCnt > 1)
            {
                RiveLogW(
                    TAG_CQ,
                    "Expected the count to be 1 but it is greater, likely indicating a leak.");
            }
            else if (refCnt < 1)
            {
                RiveLogW(
                    TAG_CQ,
                    "Expected the count to be 1 but it is less. May result in use-after-free.");
            }
        }
        commandQueue->unref();
    }

    JNIEXPORT jobject JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppCreateListeners(
        JNIEnv* env,
        jobject,
        jlong ref,
        jobject jReceiver)
    {
        auto listenersClass = FindClass(env, "app/rive/core/Listeners");
        auto listenersConstructor =
            env->GetMethodID(listenersClass.get(), "<init>", "(JJJJJJJ)V");

        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);

        auto fileListener = new FileListener(env, jReceiver);
        auto artboardListener = new ArtboardListener(env, jReceiver);
        auto stateMachineListener = new StateMachineListener(env, jReceiver);
        auto viewModelInstanceListener =
            new ViewModelInstanceListener(env, jReceiver);
        auto imageListener = new ImageListener(env, jReceiver);
        auto audioListener = new AudioListener(env, jReceiver);
        auto fontListener = new FontListener(env, jReceiver);

        commandQueue->setGlobalFileListener(fileListener);
        commandQueue->setGlobalArtboardListener(artboardListener);
        commandQueue->setGlobalStateMachineListener(stateMachineListener);
        commandQueue->setGlobalViewModelInstanceListener(
            viewModelInstanceListener);
        commandQueue->setGlobalRenderImageListener(imageListener);
        commandQueue->setGlobalAudioSourceListener(audioListener);
        commandQueue->setGlobalFontListener(fontListener);

        auto listeners =
            MakeObject(env,
                       listenersClass.get(),
                       listenersConstructor,
                       reinterpret_cast<jlong>(fileListener),
                       reinterpret_cast<jlong>(artboardListener),
                       reinterpret_cast<jlong>(stateMachineListener),
                       reinterpret_cast<jlong>(viewModelInstanceListener),
                       reinterpret_cast<jlong>(imageListener),
                       reinterpret_cast<jlong>(audioListener),
                       reinterpret_cast<jlong>(fontListener));
        return listeners.release();
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppPollMessages(JNIEnv*,
                                                             jobject,
                                                             jlong ref)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        commandQueue->processMessages();
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppRunOnCommandServer(
        JNIEnv* env,
        jobject,
        jlong ref,
        jobject jWork)
    {
        auto* commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);

        // Create a global reference to the Kotlin lambda so that it
        // survives until the command server thread invokes it
        jobject jGlobalWork = env->NewGlobalRef(jWork);

        commandQueue->runOnce([jGlobalWork](rive::CommandServer*) {
            auto* env = GetJNIEnv();
            if (env == nullptr)
            {
                RiveLogE(TAG_CQ, "Failed to get command server JNIEnv");
                return;
            }

            // Get the Function0 class and invoke() method
            // Kotlin's () -> Unit compiles to Function0<Unit>, and invoke()
            // returns Object at the JVM level due to generic type erasure
            auto function0Class = GetObjectClass(env, jGlobalWork);
            auto invokeMethod = env->GetMethodID(function0Class.get(),
                                                 "invoke",
                                                 "()Ljava/lang/Object;");

            // Invoke the Kotlin lambda (ignoring the Unit return value)
            env->CallObjectMethod(jGlobalWork, invokeMethod);

            // Check for exceptions. We won't re-throw though, since this is
            // intended to be fire-and-forget, and we don't want to crash the
            // command server thread.
            JNIExceptionHandler::ClearAndLogErrors(
                env,
                TAG_CQ,
                "runOnCommandServer: Exception thrown in Kotlin lambda:");

            // Clean up the global reference
            env->DeleteGlobalRef(jGlobalWork);
        });
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_Listeners_cppDelete(JNIEnv*,
                                           jobject,
                                           jlong fileListenerRef,
                                           jlong artboardListenerRef,
                                           jlong stateMachineListenerRef,
                                           jlong viewModelInstanceListenerRef,
                                           jlong imageListenerRef,
                                           jlong audioListenerRef,
                                           jlong fontListenerRef)
    {
        auto fileListener = reinterpret_cast<FileListener*>(fileListenerRef);
        auto artboardListener =
            reinterpret_cast<ArtboardListener*>(artboardListenerRef);
        auto stateMachineListener =
            reinterpret_cast<StateMachineListener*>(stateMachineListenerRef);
        auto viewModelInstanceListener =
            reinterpret_cast<ViewModelInstanceListener*>(
                viewModelInstanceListenerRef);
        auto imageListener = reinterpret_cast<ImageListener*>(imageListenerRef);
        auto audioListener = reinterpret_cast<AudioListener*>(audioListenerRef);
        auto fontListener = reinterpret_cast<FontListener*>(fontListenerRef);

        delete fileListener;
        delete artboardListener;
        delete stateMachineListener;
        delete viewModelInstanceListener;
        delete imageListener;
        delete audioListener;
        delete fontListener;
    }
}