/**
 * Shared header for bindings_command_queue_*.cpp files.
 * 
 * This header contains common includes, utility functions, and class definitions
 * used across all command queue binding implementation files.
 */
#ifndef BINDINGS_COMMAND_QUEUE_HPP
#define BINDINGS_COMMAND_QUEUE_HPP

#include <jni.h>
#include <android/native_window_jni.h>
#include <GLES3/gl3.h>

#include "models/render_context.hpp"
#include "helpers/android_factories.hpp"
#include "helpers/image_decode.hpp"
#include "helpers/jni_resource.hpp"
#include "helpers/rive_log.hpp"
#include "models/jni_renderer.hpp"
#include "rive/animation/state_machine_instance.hpp"
#include "rive/animation/state_machine_input.hpp"
#include "rive/animation/state_machine_number.hpp"
#include "rive/animation/state_machine_bool.hpp"
#include "rive/animation/state_machine_trigger.hpp"
#include "rive/command_queue.hpp"
#include "rive/command_server.hpp"
#include "rive/file.hpp"
#include "rive/renderer/gl/render_buffer_gl_impl.hpp"
#include "rive/renderer/gl/render_context_gl_impl.hpp"
#include "rive/renderer/gl/render_target_gl.hpp"
#include "rive/renderer/rive_render_image.hpp"

#include <future>
#include <string>
#include <utility>
#include <vector>
#include <cstring>
#include <atomic>
#include <unordered_set>
#include <thread>

// ARM NEON support for optimized pixel format conversion
// Only use NEON on arm64 where vqtbl1q_u8 is available
#if defined(__aarch64__)
#include <arm_neon.h>
#define HAVE_NEON 1
#else
#define HAVE_NEON 0
#endif

// GL_BGRA_EXT for direct BGRA readback (if available)
#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT 0x80E1
#endif

namespace rive_android {
namespace bindings {

/** Log tag for command queue bindings */
constexpr const char* TAG_CQ = "RiveN/CQ";

// =============================================================================
// Image Processing Utilities
// =============================================================================

/**
 * Check if GL_EXT_read_format_bgra is supported.
 * This allows reading pixels directly in BGRA format without conversion.
 */
inline bool checkBgraExtSupport()
{
    static int supported = -1; // -1 = unknown, 0 = no, 1 = yes
    if (supported == -1)
    {
        const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
        supported = (extensions && strstr(extensions, "GL_EXT_read_format_bgra")) ? 1 : 0;
    }
    return supported == 1;
}

#if HAVE_NEON
/**
 * Convert RGBA to BGRA using ARM NEON (processes 4 pixels at a time).
 * This is ~4x faster than scalar code.
 * Note: Only compiled for arm64 where vqtbl1q_u8 is available.
 */
inline void convertRgbaToBgraNeon(uint8_t* data, int pixelCount)
{
    // Process 4 pixels (16 bytes) at a time
    int simdCount = pixelCount / 4;
    int remainder = pixelCount % 4;
    
    // Shuffle indices for RGBA -> BGRA conversion
    // Input:  [R0,G0,B0,A0, R1,G1,B1,A1, R2,G2,B2,A2, R3,G3,B3,A3]
    // Output: [B0,G0,R0,A0, B1,G1,R1,A1, B2,G2,R2,A2, B3,G3,R3,A3]
    static const uint8_t shuffleData[16] = {
        2, 1, 0, 3,    // Pixel 0: B,G,R,A
        6, 5, 4, 7,    // Pixel 1: B,G,R,A
        10, 9, 8, 11,  // Pixel 2: B,G,R,A
        14, 13, 12, 15 // Pixel 3: B,G,R,A
    };
    uint8x16_t indices = vld1q_u8(shuffleData);
    
    uint8_t* ptr = data;
    for (int i = 0; i < simdCount; ++i)
    {
        // Load 4 RGBA pixels (16 bytes)
        uint8x16_t rgba = vld1q_u8(ptr);
        
        // Shuffle to swap R and B using table lookup
        uint8x16_t bgra = vqtbl1q_u8(rgba, indices);
        
        // Store result
        vst1q_u8(ptr, bgra);
        ptr += 16;
    }
    
    // Handle remaining pixels with scalar code
    for (int i = 0; i < remainder; ++i)
    {
        uint8_t r = ptr[0];
        uint8_t b = ptr[2];
        ptr[0] = b;
        ptr[2] = r;
        ptr += 4;
    }
}
#endif

/**
 * Convert RGBA to BGRA using scalar code (fallback).
 */
inline void convertRgbaToBgraScalar(uint8_t* data, int pixelCount)
{
    for (int i = 0; i < pixelCount; ++i)
    {
        uint8_t r = data[0];
        uint8_t b = data[2];
        data[0] = b;
        data[2] = r;
        data += 4;
    }
}

/**
 * Convert RGBA to BGRA using the best available method.
 */
inline void convertRgbaToBgra(uint8_t* data, int pixelCount)
{
#if HAVE_NEON
    convertRgbaToBgraNeon(data, pixelCount);
#else
    convertRgbaToBgraScalar(data, pixelCount);
#endif
}

/**
 * Flip image vertically in-place (for OpenGL to Android coordinate conversion).
 * Uses memcpy which is highly optimized on all platforms.
 */
inline void flipImageVertically(uint8_t* data, int width, int height)
{
    auto rowBytes = static_cast<size_t>(width) * 4;
    std::vector<uint8_t> tempRow(rowBytes);
    
    for (int y = 0; y < height / 2; ++y)
    {
        auto* top = data + (static_cast<size_t>(y) * rowBytes);
        auto* bottom = data + (static_cast<size_t>(height - 1 - y) * rowBytes);
        
        // Swap rows using memcpy (highly optimized)
        std::memcpy(tempRow.data(), top, rowBytes);
        std::memcpy(top, bottom, rowBytes);
        std::memcpy(bottom, tempRow.data(), rowBytes);
    }
}

// =============================================================================
// Handle Conversion Utilities
// =============================================================================

/** Convert a JVM long handle to a typed C++ handle */
template <typename HandleT> 
inline HandleT handleFromLong(jlong handle)
{
    return reinterpret_cast<HandleT>(static_cast<uint64_t>(handle));
}

/** Convert a typed C++ handle to a JVM long handle */
template <typename HandleT> 
inline jlong longFromHandle(HandleT handle)
{
    return static_cast<jlong>(reinterpret_cast<uint64_t>(handle));
}

// =============================================================================
// JCommandQueue - Kotlin Callback Helper
// =============================================================================

/**
 * Holds a reference to a Kotlin CommandQueue instance and allows calling
 * methods on it. Used by the Listener classes for callbacks.
 */
class JCommandQueue
{
public:
    JCommandQueue(JNIEnv* env, jobject jQueue) :
        m_class(reinterpret_cast<jclass>(
            env->NewGlobalRef(GetObjectClass(env, jQueue).get()))),
        m_jQueue(env->NewGlobalRef(jQueue))
    {}

    ~JCommandQueue()
    {
        auto env = GetJNIEnv();
        env->DeleteGlobalRef(m_class);
        env->DeleteGlobalRef(m_jQueue);
    }

    /**
     * Call a CommandQueue Kotlin instance method.
     *
     * @param name Kotlin method name
     * @param sig JNI signature string
     * @param args Arguments forwarded to CallVoidMethod
     */
    template <typename... Args>
    void call(const char* name, const char* sig, Args... args) const
    {
        auto env = GetJNIEnv();
        jmethodID mid = env->GetMethodID(m_class, name, sig);
        env->CallVoidMethod(m_jQueue, mid, args...);
    }

private:
    jclass m_class;
    jobject m_jQueue;
};

// =============================================================================
// Listener Classes
// =============================================================================

class FileListener : public rive::CommandQueue::FileListener
{
public:
    FileListener(JNIEnv* env, jobject jQueue) :
        rive::CommandQueue::FileListener(), m_queue(env, jQueue)
    {}

    virtual ~FileListener() = default;

    void onFileError(const rive::FileHandle,
                     uint64_t requestID,
                     std::string error) override
    {
        auto jError = MakeJString(GetJNIEnv(), error);
        m_queue.call("onFileError",
                     "(JLjava/lang/String;)V",
                     requestID,
                     jError.get());
    }

    void onFileLoaded(const rive::FileHandle handle,
                      uint64_t requestID) override
    {
        m_queue.call("onFileLoaded", "(JJ)V", requestID, handle);
    }

    void onArtboardsListed(const rive::FileHandle,
                           uint64_t requestID,
                           std::vector<std::string> artboardNames) override
    {
        auto jList = VecStringToJStringList(GetJNIEnv(), artboardNames);
        m_queue.call("onArtboardsListed",
                     "(JLjava/util/List;)V",
                     requestID,
                     jList.get());
    }

    void onViewModelsListed(const rive::FileHandle,
                            uint64_t requestID,
                            std::vector<std::string> viewModelNames) override
    {
        auto jList = VecStringToJStringList(GetJNIEnv(), viewModelNames);
        m_queue.call("onViewModelsListed",
                     "(JLjava/util/List;)V",
                     requestID,
                     jList.get());
    }

    void onViewModelInstanceNamesListed(
        const rive::FileHandle,
        uint64_t requestID,
        std::string,
        std::vector<std::string> instanceNames) override
    {
        auto jList = VecStringToJStringList(GetJNIEnv(), instanceNames);
        m_queue.call("onViewModelInstancesListed",
                     "(JLjava/util/List;)V",
                     requestID,
                     jList.get());
    }

    void onViewModelPropertiesListed(
        const rive::FileHandle,
        uint64_t requestID,
        std::string viewModelName,
        std::vector<rive::CommandQueue::FileListener::ViewModelPropertyData>
            properties) override
    {
        auto env = GetJNIEnv();
        auto arrayListClass = FindClass(env, "java/util/ArrayList");
        auto arrayListConstructor =
            env->GetMethodID(arrayListClass.get(), "<init>", "()V");
        auto arrayListAddFn = env->GetMethodID(arrayListClass.get(),
                                               "add",
                                               "(Ljava/lang/Object;)Z");

        auto propertyClass =
            FindClass(env, "app/rive/runtime/kotlin/core/ViewModel$Property");
        auto constructor =
            env->GetMethodID(propertyClass.get(),
                             "<init>",
                             "(Lapp/rive/runtime/kotlin/core/"
                             "ViewModel$PropertyDataType;Ljava/lang/String;)V");

        auto dataTypeClass = FindClass(
            env,
            "app/rive/runtime/kotlin/core/ViewModel$PropertyDataType");
        auto fromIntFn = env->GetStaticMethodID(
            dataTypeClass.get(),
            "fromInt",
            "(I)Lapp/rive/runtime/kotlin/core/ViewModel$PropertyDataType;");

        auto jPropertyList =
            MakeObject(env, arrayListClass.get(), arrayListConstructor);

        for (const auto& property : properties)
        {
            auto jName = MakeJString(env, property.name);
            auto jDataType = JniResource(
                env->CallStaticObjectMethod(dataTypeClass.get(),
                                            fromIntFn,
                                            static_cast<jint>(property.type)),
                env);
            auto propertyObject = MakeObject(env,
                                             propertyClass.get(),
                                             constructor,
                                             jDataType.get(),
                                             jName.get());

            env->CallBooleanMethod(jPropertyList.get(),
                                   arrayListAddFn,
                                   propertyObject.get());
        }

        m_queue.call("onViewModelPropertiesListed",
                     "(JLjava/util/List;)V",
                     requestID,
                     jPropertyList.get());
    }

    void onViewModelEnumsListed(const rive::FileHandle,
                                uint64_t requestID,
                                std::vector<rive::ViewModelEnum> enums) override
    {
        auto env = GetJNIEnv();
        auto arrayListClass = FindClass(env, "java/util/ArrayList");
        auto arrayListConstructor =
            env->GetMethodID(arrayListClass.get(), "<init>", "()V");
        auto arrayListAddFn = env->GetMethodID(arrayListClass.get(),
                                               "add",
                                               "(Ljava/lang/Object;)Z");

        auto enumClass =
            FindClass(env, "app/rive/runtime/kotlin/core/File$Enum");
        auto enumConstructor =
            env->GetMethodID(enumClass.get(),
                             "<init>",
                             "(Ljava/lang/String;Ljava/util/List;)V");

        // Outer list of enums to be returned
        auto jEnumsList =
            MakeObject(env, arrayListClass.get(), arrayListConstructor);

        for (const auto& enumItem : enums)
        {
            auto jName = MakeJString(env, enumItem.name);
            // Inner list of enum values
            auto jValuesList =
                MakeObject(env, arrayListClass.get(), arrayListConstructor);

            // For each value in the enum item, add it to the inner list
            for (const auto& value : enumItem.enumerants)
            {
                auto jValue = MakeJString(env, value);
                env->CallBooleanMethod(jValuesList.get(),
                                       arrayListAddFn,
                                       jValue.get());
            }

            auto jEnumObject = MakeObject(env,
                                          enumClass.get(),
                                          enumConstructor,
                                          jName.get(),
                                          jValuesList.get());

            // Add the inner enum description to the outer list
            env->CallBooleanMethod(jEnumsList.get(),
                                   arrayListAddFn,
                                   jEnumObject.get());
        }

        m_queue.call("onEnumsListed",
                     "(JLjava/util/List;)V",
                     requestID,
                     jEnumsList.get());
    }

private:
    JCommandQueue m_queue;
};

class ArtboardListener : public rive::CommandQueue::ArtboardListener
{
public:
    ArtboardListener(JNIEnv* env, jobject jQueue) :
        rive::CommandQueue::ArtboardListener(), m_queue(env, jQueue)
    {}

    virtual ~ArtboardListener() = default;

    void onArtboardError(const rive::ArtboardHandle,
                         uint64_t requestID,
                         std::string error) override
    {
        auto jError = MakeJString(GetJNIEnv(), error);
        m_queue.call("onArtboardError",
                     "(JLjava/lang/String;)V",
                     requestID,
                     jError.get());
    }

    void onStateMachinesListed(
        const rive::ArtboardHandle,
        uint64_t requestID,
        std::vector<std::string> stateMachineNames) override
    {
        auto jList = VecStringToJStringList(GetJNIEnv(), stateMachineNames);
        m_queue.call("onStateMachinesListed",
                     "(JLjava/util/List;)V",
                     requestID,
                     jList.get());
    }

private:
    JCommandQueue m_queue;
};

class StateMachineListener : public rive::CommandQueue::StateMachineListener
{
public:
    StateMachineListener(JNIEnv* env, jobject jQueue) :
        rive::CommandQueue::StateMachineListener(), m_queue(env, jQueue)
    {}

    virtual ~StateMachineListener() = default;

    void onStateMachineError(const rive::StateMachineHandle,
                             uint64_t requestID,
                             std::string error) override
    {
        auto jError = MakeJString(GetJNIEnv(), error);
        m_queue.call("onStateMachineError",
                     "(JLjava/lang/String;)V",
                     requestID,
                     jError.get());
    }

    void onStateMachineSettled(const rive::StateMachineHandle smHandle,
                               uint64_t requestID) override
    {
        m_queue.call("onStateMachineSettled", "(J)V", longFromHandle(smHandle));
    }

private:
    JCommandQueue m_queue;
};

class ViewModelInstanceListener
    : public rive::CommandQueue::ViewModelInstanceListener
{
public:
    ViewModelInstanceListener(JNIEnv* env, jobject jQueue) :
        rive::CommandQueue::ViewModelInstanceListener(), m_queue(env, jQueue)
    {}

    virtual ~ViewModelInstanceListener() = default;

    void onViewModelInstanceError(const rive::ViewModelInstanceHandle,
                                  uint64_t requestID,
                                  std::string error) override
    {
        auto jError = MakeJString(GetJNIEnv(), error);
        m_queue.call("onViewModelInstanceError",
                     "(JLjava/lang/String;)V",
                     requestID,
                     jError.get());
    }

    void onViewModelDataReceived(
        const rive::ViewModelInstanceHandle vmiHandle,
        uint64_t requestID,
        rive::CommandQueue::ViewModelInstanceData data) override
    {
        auto env = GetJNIEnv();
        auto jPropertyName = MakeJString(env, data.metaData.name);

        switch (data.metaData.type)
        {
            case rive::DataType::number:
                m_queue.call("onNumberPropertyUpdated",
                             "(JJLjava/lang/String;F)V",
                             requestID,
                             longFromHandle(vmiHandle),
                             jPropertyName.get(),
                             data.numberValue);
                break;
            case rive::DataType::string:
                m_queue.call("onStringPropertyUpdated",
                             "(JJLjava/lang/String;Ljava/"
                             "lang/String;)V",
                             requestID,
                             longFromHandle(vmiHandle),
                             jPropertyName.get(),
                             MakeJString(env, data.stringValue).get());
                break;
            case rive::DataType::boolean:
                m_queue.call("onBooleanPropertyUpdated",
                             "(JJLjava/lang/String;Z)V",
                             requestID,
                             longFromHandle(vmiHandle),
                             jPropertyName.get(),
                             data.boolValue);
                break;
            case rive::DataType::enumType:
                m_queue.call("onEnumPropertyUpdated",
                             "(JJLjava/lang/String;Ljava/"
                             "lang/String;)V",
                             requestID,
                             longFromHandle(vmiHandle),
                             jPropertyName.get(),
                             MakeJString(env, data.stringValue).get());
                break;
            case rive::DataType::color:
                m_queue.call("onColorPropertyUpdated",
                             "(JJLjava/lang/String;I)V",
                             requestID,
                             longFromHandle(vmiHandle),
                             jPropertyName.get(),
                             data.colorValue);
                break;
            case rive::DataType::trigger:
                m_queue.call("onTriggerPropertyUpdated",
                             "(JJLjava/lang/String;)V",
                             requestID,
                             longFromHandle(vmiHandle),
                             jPropertyName.get());
                break;
            default:
                RiveLogE(TAG_CQ,
                         "Unknown ViewModelInstance property type: %d",
                         static_cast<int>(data.metaData.type));
        }
    }

    void onViewModelListSizeReceived(const rive::ViewModelInstanceHandle,
                                     uint64_t requestId,
                                     std::string path,
                                     size_t size) override
    {
        m_queue.call("onViewModelListSizeReceived", "(JI)V", requestId, size);
    }

private:
    constexpr static const char* TAG = "RiveN/VMIListener";
    JCommandQueue m_queue;
};

class ImageListener : public rive::CommandQueue::RenderImageListener
{
public:
    ImageListener(JNIEnv* env, jobject jQueue) :
        rive::CommandQueue::RenderImageListener(), m_queue(env, jQueue)
    {}

    virtual ~ImageListener() = default;

    void onRenderImageDecoded(const rive::RenderImageHandle handle,
                              uint64_t requestID) override
    {
        m_queue.call("onImageDecoded",
                     "(JJ)V",
                     requestID,
                     longFromHandle(handle));
    }

    void onRenderImageError(const rive::RenderImageHandle,
                            uint64_t requestID,
                            std::string error) override
    {
        auto jError = MakeJString(GetJNIEnv(), error);
        m_queue.call("onImageError",
                     "(JLjava/lang/String;)V",
                     requestID,
                     jError.get());
    }

private:
    JCommandQueue m_queue;
};

class AudioListener : public rive::CommandQueue::AudioSourceListener
{
public:
    AudioListener(JNIEnv* env, jobject queue) :
        rive::CommandQueue::AudioSourceListener(), m_queue(env, queue)
    {}

    virtual ~AudioListener() = default;

    void onAudioSourceDecoded(const rive::AudioSourceHandle handle,
                              uint64_t requestID) override
    {
        m_queue.call("onAudioDecoded",
                     "(JJ)V",
                     requestID,
                     longFromHandle(handle));
    }

    void onAudioSourceError(const rive::AudioSourceHandle,
                            uint64_t requestID,
                            std::string error) override
    {
        auto jError = MakeJString(GetJNIEnv(), error);
        m_queue.call("onAudioError",
                     "(JLjava/lang/String;)V",
                     requestID,
                     jError.get());
    }

private:
    JCommandQueue m_queue;
};

class FontListener : public rive::CommandQueue::FontListener
{
public:
    FontListener(JNIEnv* env, jobject jQueue) :
        rive::CommandQueue::FontListener(), m_queue(env, jQueue)
    {}

    virtual ~FontListener() = default;

    void onFontDecoded(const rive::FontHandle handle,
                       uint64_t requestID) override
    {
        m_queue.call("onFontDecoded",
                     "(JJ)V",
                     requestID,
                     longFromHandle(handle));
    }

    void onFontError(const rive::FontHandle,
                     uint64_t requestID,
                     std::string error) override
    {
        auto jError = MakeJString(GetJNIEnv(), error);
        m_queue.call("onFontError",
                     "(JLjava/lang/String;)V",
                     requestID,
                     jError.get());
    }

private:
    JCommandQueue m_queue;
};

// =============================================================================
// Property Getter/Setter Helpers
// =============================================================================

/** Typedef for the below setProperty function. */
template <typename T>
using PropertySetter =
    void (rive::CommandQueue::*)(rive::ViewModelInstanceHandle,
                                 std::string,
                                 T,
                                 uint64_t);

/** A generic setter for all property types. */
template <typename T>
inline void setProperty(JNIEnv* env,
                        jlong ref,
                        jlong jViewModelInstanceHandle,
                        jstring jPropertyPath,
                        T value,
                        PropertySetter<T> setter)
{
    auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
    auto viewModelInstanceHandle =
        handleFromLong<rive::ViewModelInstanceHandle>(jViewModelInstanceHandle);
    auto propertyPath = JStringToString(env, jPropertyPath);

    (commandQueue->*setter)(viewModelInstanceHandle,
                            propertyPath,
                            std::move(value),
                            0); // Pass 0 for requestID
}

/** Typedef for the below getProperty function. */
using PropertyGetter =
    void (rive::CommandQueue::*)(rive::ViewModelInstanceHandle,
                                 std::string,
                                 uint64_t);

/** A generic getter for all property types. */
inline void getProperty(JNIEnv* env,
                        jlong ref,
                        jlong requestID,
                        jlong jViewModelInstanceHandle,
                        jstring jPropertyPath,
                        PropertyGetter getter)
{
    auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
    auto viewModelInstanceHandle =
        handleFromLong<rive::ViewModelInstanceHandle>(jViewModelInstanceHandle);
    auto propertyPath = JStringToString(env, jPropertyPath);

    (commandQueue->*getter)(viewModelInstanceHandle, propertyPath, requestID);
}

// =============================================================================
// CommandServerFactory
// =============================================================================

/**
 * A factory for use with the command server.
 *
 * The base class implements most methods, requiring this class to define both
 * how to create a buffer and decode an image.
 *
 * For buffers, we defer to the existing rive::gpu::RenderContext's
 * implementation, since it is itself a factory.
 *
 * For decoding images, we pass to a method that performs Kotlin decoding.
 *
 * Ideally this would subclass RenderContext to only override decoding images,
 * but it's constructor is effectively hidden behind the static MakeContext
 * function. So instead this class wraps and delegates to it instead.
 *
 * This class must not outlive the passed renderContext, which is held as a raw
 * pointer.
 */
class CommandServerFactory : public rive::RiveRenderFactory
{
public:
    explicit CommandServerFactory(RenderContext* renderContext) :
        m_renderContext(renderContext)
    {}
    ~CommandServerFactory() override = default;

    rive::rcp<rive::RenderImage> decodeImage(
        rive::Span<const uint8_t> encodedBytes) override
    {
        RiveLogD("RiveN/CQFactory", "Decoding encoded image");
        return renderImageFromAndroidDecode(encodedBytes,
                                            false,
                                            m_renderContext);
    }

    rive::rcp<rive::RenderBuffer> makeRenderBuffer(
        rive::RenderBufferType type,
        rive::RenderBufferFlags flags,
        size_t sizeInBytes) override
    {
        RiveLogD("RiveN/CQFactory", "Creating render buffer");
        return m_renderContext->riveContext->makeRenderBuffer(type,
                                                              flags,
                                                              sizeInBytes);
    }

    RenderContext* getRenderContext() { return m_renderContext; }

private:
    RenderContext* const m_renderContext = nullptr;
};

// =============================================================================
// CommandQueueWithThread
// =============================================================================

/**
 * A subclass of rive::CommandQueue which handles starting and stopping of the
 * std::thread.
 */
class CommandQueueWithThread : public rive::CommandQueue
{
public:
    CommandQueueWithThread() : rive::CommandQueue() {}

    /**
     * Starts the command server thread.
     *
     * @param renderContext The render context to use in the command server
     * thread. Its resources will be created and destroyed within the thread,
     * but the object itself must still be managed by the caller and outlive the
     * thread.
     * @param promise A promise to signal startup success or failure.
     */
    void startCommandServer(RenderContext* renderContext,
                            std::promise<StartupResult>&& promise)
    {
        // Wrap command queue in an RCP, adding +1 ref with ref_rcp.
        // Before this RCP falls out of scope (-1) it is copied (+1) into the
        // thread lambda which is unref'd (-1) at the end of the lambda scope.
        auto self = rive::ref_rcp<CommandQueueWithThread>(this);

        m_commandServerThread = std::thread([renderContext,
                                             self,
                                             promise =
                                                 std::move(promise)]() mutable {
            const auto THREAD_NAME = "Rive CmdServer";
            JNIEnv* env = nullptr;
            JavaVMAttachArgs args{.version = JNI_VERSION_1_6,
                                  .name = THREAD_NAME,
                                  .group = nullptr};
            auto attachResult = g_JVM->AttachCurrentThread(&env, &args);
            RiveLogD(TAG_CQ, "Attached command server thread to JVM");
            if (attachResult != JNI_OK)
            {
                RiveLogE(TAG_CQ,
                         "Failed to attach command server thread to JVM: %d",
                         attachResult);
                promise.set_value(
                    {false, EGL_BAD_ALLOC, "Failed to attach thread to JVM"});
                return;
            }

            RiveLogD(TAG_CQ, "Setting command server thread name");
            // Set the native thread name
            pthread_setname_np(pthread_self(), THREAD_NAME);
            // Set the JVM thread name
            // Scope the JniResource objects to fall out of scope and delete
            // local refs before detaching the thread (which makes the JNIEnv
            // invalid).
            {
                auto jThreadClass = FindClass(env, "java/lang/Thread");
                auto currentThreadFn =
                    env->GetStaticMethodID(jThreadClass.get(),
                                           "currentThread",
                                           "()Ljava/lang/Thread;");
                auto jThreadObj = MakeJniResource(
                    env->CallStaticObjectMethod(jThreadClass.get(),
                                                currentThreadFn),
                    env);

                auto setNameFn = env->GetMethodID(jThreadClass.get(),
                                                  "setName",
                                                  "(Ljava/lang/String;)V");
                auto jName = MakeJString(env, THREAD_NAME);
                env->CallVoidMethod(jThreadObj.get(), setNameFn, jName.get());
            }

            RiveLogD(TAG_CQ,
                     "Initializing Rive render context for command server");
            auto result = renderContext->initialize();
            if (!result.success)
            {
                RiveLogE(TAG_CQ,
                         "Failed to initialize the Rive render context");
                promise.set_value(result);
                return;
            }

            // Stack allocated factory for the command server
            RiveLogD(TAG_CQ, "Creating command server factory");
            auto factory = CommandServerFactory(renderContext);

            // Stack allocated command server
            // Takes a copy of this object's RCP, increasing the ref count to 3,
            // releasing it when the command server falls out of scope.
            RiveLogD(TAG_CQ, "Creating command server");
            auto commandServer = rive::CommandServer(self, &factory);

            // Signal success and unblock the main thread
            promise.set_value(
                {true, EGL_SUCCESS, "Command Server started successfully"});

            // Begin the serving loop. This will "block" the thread until
            // the server receives the disconnect command.
            RiveLogD(TAG_CQ, "Beginning command server processing loop");
            commandServer.serveUntilDisconnect();

            RiveLogD(TAG_CQ, "Command server disconnected, cleaning up");

            RiveLogD(TAG_CQ, "Deleting render context");
            renderContext->destroy();

            // Extra information for debugging command queue lifetimes
            auto refCnt = self->debugging_refcnt();
            if (refCnt != 3)
            {
                RiveLogW(
                    TAG_CQ,
                    "Command queue ref count before worker thread detach does not match expected value:\n"
                    "  Expected: 3; Actual: %d\n"
                    "    1. Main thread's released reference\n"
                    "    2. This worker thread, cleaned by rcp scope\n"
                    "    3. Command server rcp, stack allocated and about to fall from scope",
                    refCnt);
            }

            // Cleanup JVM thread attachment
            RiveLogD(TAG_CQ, "Detaching command server thread from JVM");
            g_JVM->DetachCurrentThread();
        });
    }

    /**
     * Shuts down the command server by sending a disconnect command, then joins
     * the thread. This means it is a blocking call until the command server
     * thread has fully exited.
     */
    void shutdownAndJoin()
    {
        disconnect();
        if (m_commandServerThread.joinable())
        {
            m_commandServerThread.join();
        }
    }

    /**
     * Check if we've logged a missing artboard error already for this draw
     * key.
     */
    bool shouldLogArtboardNull(rive::DrawKey key)
    {
        return m_artboardNullKeys.insert(key).second;
    }

    /**
     * Check if we've logged a missing state machine error already for this
     * draw key.
     */
    bool shouldLogStateMachineNull(rive::DrawKey key)
    {
        return m_stateMachineNullKeys.insert(key).second;
    }

private:
    std::thread m_commandServerThread;
    // Holds that an error has been reported, to avoid log spam
    std::unordered_set<rive::DrawKey> m_artboardNullKeys;
    std::unordered_set<rive::DrawKey> m_stateMachineNullKeys;
};

} // namespace bindings
} // namespace rive_android

#endif // BINDINGS_COMMAND_QUEUE_HPP