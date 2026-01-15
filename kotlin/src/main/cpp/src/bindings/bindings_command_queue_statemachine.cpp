/**
 * Command Queue State Machine Bindings
 * 
 * This file contains JNI bindings for:
 * - State machine creation and deletion
 * - State machine advancement
 * - Legacy state machine input manipulation (SMINumber, SMIBool, SMITrigger)
 */

#include "bindings_command_queue.hpp"

using namespace rive_android;
using namespace rive_android::bindings;

extern "C"
{
    JNIEXPORT jlong JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppCreateDefaultStateMachine(
        JNIEnv*,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jArtboardHandle)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);

        auto stateMachine = commandQueue->instantiateDefaultStateMachine(
            handleFromLong<rive::ArtboardHandle>(jArtboardHandle),
            nullptr,
            requestID);
        return longFromHandle(stateMachine);
    }

    JNIEXPORT jlong JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppCreateStateMachineByName(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong requestID,
        jlong jArtboardHandle,
        jstring name)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto nativeName = JStringToString(env, name);

        auto stateMachine = commandQueue->instantiateStateMachineNamed(
            handleFromLong<rive::ArtboardHandle>(jArtboardHandle),
            nativeName,
            nullptr,
            requestID);
        return longFromHandle(stateMachine);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppDeleteStateMachine(
        JNIEnv*,
        jobject,
        jlong ref,
        jlong requestID,
        jlong stateMachineHandle)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        commandQueue->deleteStateMachine(
            handleFromLong<rive::StateMachineHandle>(stateMachineHandle),
            requestID);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppAdvanceStateMachine(
        JNIEnv*,
        jobject,
        jlong ref,
        jlong stateMachineHandle,
        jlong deltaTimeNs)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto deltaSeconds = static_cast<float_t>(deltaTimeNs) / 1e9f; // NS to S
        commandQueue->advanceStateMachine(
            handleFromLong<rive::StateMachineHandle>(stateMachineHandle),
            deltaSeconds);
    }

/**
 * ==============================================================================
 * STATE MACHINE INPUT MANIPULATION VIA HANDLE
 * ==============================================================================
 *
 * MOTIVATION:
 * -----------
 * The Compose/RiveSprite architecture uses a handle-based approach where
 * StateMachineHandle (an opaque Long ID) is used instead of direct C++ pointers.
 * This is different from the View-based architecture (RiveAnimationView/
 * RiveFileController) which uses StateMachineInstance with direct pointer access.
 *
 * The handle-based architecture was designed primarily for the new ViewModel
 * property binding system. However, many existing Rive files do not have
 * ViewModel definitions and rely on the legacy state machine input mechanism
 * (SMINumber, SMIBoolean, SMITrigger) for animation control.
 *
 * These methods bridge this gap by allowing state machine inputs to be
 * manipulated via handles, enabling the Compose architecture to work with
 * legacy Rive files that don't have ViewModels.
 *
 * USE CASE:
 * ---------
 * RiveSprite (Compose) needs to control animations in legacy .riv files that:
 * 1. Have state machine inputs (Number, Boolean, Trigger) defined
 * 2. Do NOT have ViewModel definitions
 *
 * Without these methods, RiveSprite can only control animations via ViewModel
 * properties, making it incompatible with legacy files.
 *
 * IMPLEMENTATION:
 * ---------------
 * These methods use CommandQueue::runOnce() to execute on the command server
 * thread where the actual StateMachineInstance lives. The handle is converted
 * to a StateMachineInstance pointer via CommandServer::getStateMachineInstance(),
 * then the input is found by name and its value is set.
 *
 * This is the equivalent of what RiveFileController does synchronously:
 *   val smiInput = stateMachineInstance.input(inputName)
 *   when (smiInput) { is SMINumber -> smiInput.value = value }
 *
 * But adapted for the asynchronous, handle-based CommandQueue architecture.
 *
 * KOTLIN USAGE (RiveSprite):
 * --------------------------
 * // Unified API - automatically uses ViewModel if available, falls back to SMI
 * sprite.setNumber("Level", 2f)
 * sprite.setBoolean("isActive", true)
 * sprite.fireTrigger("attack")
 * ==============================================================================
 */

    /**
     * Set a number input on a state machine by name.
     *
     * This method finds the SMINumber input with the given name on the state
     * machine and sets its value. This is used for legacy Rive files without
     * ViewModel definitions.
     *
     * @param ref CommandQueue native pointer
     * @param stateMachineHandle Handle to the target StateMachineInstance
     * @param jInputName Name of the number input (e.g., "Level", "Speed")
     * @param value The float value to set
     *
     * @note If the input is not found or is not a number type, the operation is
     *       silently ignored (matches RiveFileController behavior).
     * @note This executes asynchronously on the command server thread.
     */
    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppSetStateMachineNumberInput(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong stateMachineHandle,
        jstring jInputName,
        jfloat value)
    {
        auto* commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto inputName = JStringToString(env, jInputName);
        auto smHandle =
            handleFromLong<rive::StateMachineHandle>(stateMachineHandle);

        commandQueue->runOnce(
            [smHandle, inputName = std::move(inputName), value](
                rive::CommandServer* server) {
                auto* sm = server->getStateMachineInstance(smHandle);
                if (sm == nullptr)
                {
                    return;
                }

                for (size_t i = 0; i < sm->inputCount(); ++i)
                {
                    auto* input = sm->input(i);
                    if (input->name() == inputName &&
                        input->input()->is<rive::StateMachineNumber>())
                    {
                        static_cast<rive::SMINumber*>(input)->value(value);
                        return;
                    }
                }
            });
    }

    /**
     * Set a boolean input on a state machine by name.
     *
     * This method finds the SMIBoolean input with the given name on the state
     * machine and sets its value. This is used for legacy Rive files without
     * ViewModel definitions.
     *
     * @param ref CommandQueue native pointer
     * @param stateMachineHandle Handle to the target StateMachineInstance
     * @param jInputName Name of the boolean input (e.g., "isActive", "enabled")
     * @param value The boolean value to set
     *
     * @note If the input is not found or is not a boolean type, the operation
     *       is silently ignored (matches RiveFileController behavior).
     * @note This executes asynchronously on the command server thread.
     */
    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppSetStateMachineBooleanInput(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong stateMachineHandle,
        jstring jInputName,
        jboolean value)
    {
        auto* commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto inputName = JStringToString(env, jInputName);
        auto smHandle =
            handleFromLong<rive::StateMachineHandle>(stateMachineHandle);

        commandQueue->runOnce(
            [smHandle, inputName = std::move(inputName), value](
                rive::CommandServer* server) {
                auto* sm = server->getStateMachineInstance(smHandle);
                if (sm == nullptr)
                {
                    return;
                }

                for (size_t i = 0; i < sm->inputCount(); ++i)
                {
                    auto* input = sm->input(i);
                    if (input->name() == inputName &&
                        input->input()->is<rive::StateMachineBool>())
                    {
                        static_cast<rive::SMIBool*>(input)->value(value);
                        return;
                    }
                }
            });
    }

    /**
     * Fire a trigger input on a state machine by name.
     *
     * This method finds the SMITrigger input with the given name on the state
     * machine and fires it. This is used for legacy Rive files without
     * ViewModel definitions.
     *
     * @param ref CommandQueue native pointer
     * @param stateMachineHandle Handle to the target StateMachineInstance
     * @param jInputName Name of the trigger input (e.g., "attack", "jump")
     *
     * @note If the input is not found or is not a trigger type, the operation
     *       is silently ignored (matches RiveFileController behavior).
     * @note This executes asynchronously on the command server thread.
     */
    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppFireStateMachineTrigger(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong stateMachineHandle,
        jstring jInputName)
    {
        auto* commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto inputName = JStringToString(env, jInputName);
        auto smHandle =
            handleFromLong<rive::StateMachineHandle>(stateMachineHandle);

        commandQueue->runOnce(
            [smHandle, inputName = std::move(inputName)](
                rive::CommandServer* server) {
                auto* sm = server->getStateMachineInstance(smHandle);
                if (sm == nullptr)
                {
                    return;
                }

                for (size_t i = 0; i < sm->inputCount(); ++i)
                {
                    auto* input = sm->input(i);
                    if (input->name() == inputName &&
                        input->input()->is<rive::StateMachineTrigger>())
                    {
                        static_cast<rive::SMITrigger*>(input)->fire();
                        return;
                    }
                }
            });
    }
}