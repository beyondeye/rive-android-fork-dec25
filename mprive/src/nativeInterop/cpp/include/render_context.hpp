#pragma once
#include <GLES3/gl3.h>

#include "rive_log.hpp"
#include "rive/renderer/gl/render_context_gl_impl.hpp"
#include "rive/renderer/render_context.hpp"
#include "rive/renderer/rive_render_image.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <EGL/egl.h>

// Include AndroidFactory for Android builds
// This is needed because render_context.hpp uses unique_ptr<AndroidFactory>
// which requires the complete type definition
#ifdef RIVE_ANDROID
#include "helpers/android_factory.hpp"
#endif

namespace rive_mp
{

#ifndef RIVE_ANDROID
// Forward declaration for non-Android platforms
class AndroidFactory;
#endif

// Log prefix for RenderContext messages (embedded in log strings since macros already provide LOG_TAG)
#define RC_PREFIX "[RenderContext] "

/**
 * Result of startup, both in RenderContext initialization and CommandQueue
 * startup.
 */
struct StartupResult
{
    bool success;
    int32_t errorCode;
    std::string message;
};

/** Map of EGL error codes to their string representations. */
static const std::unordered_map<int32_t, std::string> eglErrorMessages = {
    {EGL_SUCCESS, "EGL_SUCCESS"},
    {EGL_NOT_INITIALIZED, "EGL_NOT_INITIALIZED"},
    {EGL_BAD_ACCESS, "EGL_BAD_ACCESS"},
    {EGL_BAD_ALLOC, "EGL_BAD_ALLOC"},
    {EGL_BAD_ATTRIBUTE, "EGL_BAD_ATTRIBUTE"},
    {EGL_BAD_CONTEXT, "EGL_BAD_CONTEXT"},
    {EGL_BAD_CONFIG, "EGL_BAD_CONFIG"},
    {EGL_BAD_CURRENT_SURFACE, "EGL_BAD_CURRENT_SURFACE"},
    {EGL_BAD_DISPLAY, "EGL_BAD_DISPLAY"},
    {EGL_BAD_SURFACE, "EGL_BAD_SURFACE"},
    {EGL_BAD_MATCH, "EGL_BAD_MATCH"},
    {EGL_BAD_PARAMETER, "EGL_BAD_PARAMETER"},
    {EGL_BAD_NATIVE_PIXMAP, "EGL_BAD_NATIVE_PIXMAP"},
    {EGL_BAD_NATIVE_WINDOW, "EGL_BAD_NATIVE_WINDOW"},
    {EGL_CONTEXT_LOST, "EGL_CONTEXT_LOST"},
};

static std::string errorString(int32_t errorCode)
{
    auto it = eglErrorMessages.find(errorCode);
    if (it != eglErrorMessages.end())
        return it->second;

    char buffer[64];
    auto n = std::snprintf(buffer,
                           sizeof(buffer),
                           "Unknown EGL error (0x%04x)",
                           errorCode);
    if (n < 0)
        return "Unknown EGL error";
    return {buffer, static_cast<std::size_t>(n)};
}

/**
 * Abstract base class for native RenderContext implementations.
 * Contains the needed operations for:
 * - Initialization of resources
 * - Destruction of resources
 * - Beginning a frame (binding the context to a surface)
 * - Presenting a frame (swapping buffers)
 *
 * Also holds a pointer to the Rive RenderContext instance.
 *
 * This class has thread affinity. It must be initialized, used, and
 * destroyed on the same thread.
 * 
 * ## Platform Implementations
 * 
 * - **Android**: `RenderContextGL` (EGL/OpenGL ES) - implemented here
 * - **Desktop**: Would use GLFW or Skia (Phase F)
 * - **iOS**: Would use EAGLContext or Metal (Future)
 * - **Web/WASM**: Would use WebGL (Future)
 * 
 * ## Factory Pattern for File Loading
 * 
 * When loading Rive files via `rive::File::import()`, a `rive::Factory*` is
 * required to create GPU-accelerated render objects (paths, paints, images).
 * 
 * The `rive::gpu::RenderContext` (stored as `riveContext`) inherits from 
 * `rive::Factory`, so it can be used directly as the factory. The `getFactory()`
 * method provides access to this.
 * 
 * ### Why This Matters
 * 
 * If a `NoOpFactory` is used instead of the GPU factory, the file will load
 * correctly (artboards, state machines work) but all visual content becomes
 * invisible because no-op render objects don't actually draw anything!
 * 
 * ### Platform-Specific Customization
 * 
 * If a platform needs custom factory behavior (e.g., custom image decoding),
 * override `getFactory()` in the derived class to return a custom factory
 * wrapper. See `CommandServerFactory` in rive-android for an example.
 */
class RenderContext
{
public:
    virtual ~RenderContext() = default;

    /**
     * Initialize the RenderContext and its resources. Call on the render
     * thread.
     */
    virtual StartupResult initialize() = 0;
    /** Destroy the RenderContext's resources. Call on the render thread. */
    virtual void destroy() = 0;

    /** Begin a frame by binding the context to the provided surface. */
    virtual void beginFrame(void* surface) = 0;
    /** Present the frame by swapping buffers on the provided surface. */
    virtual void present(void* surface) = 0;

    /**
     * Get the Rive Factory for creating render objects.
     * 
     * This factory is used when importing Rive files to create GPU-accelerated
     * paths, paints, and images. The `rive::gpu::RenderContext` itself inherits
     * from `rive::Factory`, so we return it directly.
     * 
     * @return The factory, or nullptr if the render context is not initialized.
     * 
     * ## Multiplatform Note
     * 
     * This method is virtual to allow platform-specific overrides if needed.
     * For example, Android might want to wrap the factory for custom image
     * decoding (see `CommandServerFactory` in rive-android). By default,
     * all platforms can use the `riveContext` directly since it's Rive's
     * cross-platform GPU abstraction.
     * 
     * ## Usage Example (in CommandServer)
     * 
     * ```cpp
     * rive::Factory* factory = nullptr;
     * if (m_renderContext != nullptr) {
     *     auto* renderContext = static_cast<rive_mp::RenderContext*>(m_renderContext);
     *     factory = renderContext->getFactory();
     * }
     * if (factory == nullptr) {
     *     // Fallback for tests or headless mode
     *     factory = m_noOpFactory.get();
     * }
     * auto file = rive::File::import(bytes, factory, ...);
     * ```
     */
    virtual rive::Factory* getFactory() const
    {
        return riveContext ? riveContext.get() : nullptr;
    }

    /**
     * Create a GPU render image from RGBA pixel data.
     * 
     * This is used by platform-specific image decoders (like AndroidFactory)
     * to create GPU textures from decoded pixel data.
     * 
     * @param width Image width in pixels.
     * @param height Image height in pixels.
     * @param pixels RGBA pixel data (premultiplied alpha, 4 bytes per pixel).
     * @return A GPU-accelerated RenderImage, or nullptr if creation failed.
     */
    virtual rive::rcp<rive::RenderImage> makeImage(
        uint32_t width,
        uint32_t height,
        std::unique_ptr<uint8_t[]> pixels)
    {
        if (riveContext == nullptr)
        {
            LOGE(RC_PREFIX "makeImage: riveContext is null");
            return nullptr;
        }
        
        // Calculate mip level count based on image dimensions
        auto mipLevelCount = rive::math::msb(height | width);
        
        LOGD(RC_PREFIX "makeImage: Creating RiveRenderImage %ux%u with %u mip levels",
             width, height, mipLevelCount);
        
        // Create a GPU texture from the RGBA pixel data using the impl
        // The impl() gives us access to RenderContextImpl which has makeImageTexture
        auto texture = riveContext->impl()->makeImageTexture(
            width,
            height,
            mipLevelCount,
            pixels.get());
        
        if (texture == nullptr)
        {
            LOGE(RC_PREFIX "makeImage: Failed to create GPU texture");
            return nullptr;
        }
        
        // Create a RiveRenderImage wrapping the GPU texture
        return rive::make_rcp<rive::RiveRenderImage>(std::move(texture));
    }

    std::unique_ptr<rive::gpu::RenderContext> riveContext;
};

/** Create a 1x1 PBuffer surface to bind before Android provides a surface. */
static EGLSurface createPBufferSurface(EGLDisplay eglDisplay,
                                       EGLContext eglContext)
{
    LOGD(RC_PREFIX "Creating 1x1 PBuffer surface for EGL context");

    EGLint configID = 0;
    eglQueryContext(eglDisplay, eglContext, EGL_CONFIG_ID, &configID);

    EGLConfig config;
    EGLint configCount = 0;
    EGLint configAttributes[] = {EGL_CONFIG_ID, configID, EGL_NONE};
    eglChooseConfig(eglDisplay, configAttributes, &config, 1, &configCount);

    // We expect only one config.
    if (configCount == 1)
    {
        EGLint pBufferAttributes[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
        auto surface =
            eglCreatePbufferSurface(eglDisplay, config, pBufferAttributes);
        if (surface != EGL_NO_SURFACE)
        {
            LOGD(RC_PREFIX "Successfully created PBuffer surface");
            return surface;
        }
        else
        {
            std::string errMsg = "Failed to create PBuffer surface. Error: " + errorString(eglGetError());
            LOGE(RC_PREFIX "%s", errMsg.c_str());
            return EGL_NO_SURFACE;
        }
    }
    else
    {
        LOGE(RC_PREFIX "Failed to choose EGL config for PBuffer surface");
        return EGL_NO_SURFACE;
    }
}

/** 
 * Native RenderContext implementation for EGL/OpenGL ES.
 * 
 * This is the Android implementation. For other platforms:
 * - Desktop (Phase F): Would create a RenderContextGLFW or RenderContextSkia
 * - iOS (Future): Would create a RenderContextEAGL or RenderContextMetal
 * 
 * ## Android Image Decoding
 * 
 * On Android, this class uses AndroidFactory which wraps the GPU factory
 * and overrides decodeImage() to use Android's BitmapFactory through JNI.
 * This provides platform-native image decoding without requiring rive-runtime's
 * built-in image decoders (libpng, libjpeg, libwebp).
 */
struct RenderContextGL : RenderContext
{
    RenderContextGL(EGLDisplay eglDisplay, EGLContext eglContext) :
        RenderContext(),
        eglDisplay(eglDisplay),
        eglContext(eglContext),
        m_androidFactory(nullptr),
        pBuffer(createPBufferSurface(eglDisplay, eglContext))
    {}

    /**
     * Initialize the RenderContextGL by making the context current with the 1x1
     * PBuffer surface and creating the Rive RenderContextGL.
     *
     * @return Whether initialization succeeded, and the error code/message if
     * not.
     */
    StartupResult initialize() override
    {
        if (pBuffer == EGL_NO_SURFACE)
        {
            auto error = eglGetError();
            std::string errMsg = "Failed to create PBuffer surface. Error: " + errorString(error);
            LOGE(RC_PREFIX "%s", errMsg.c_str());
            return {false, error, "Failed to create PBuffer surface"};
        }

        LOGD(RC_PREFIX "Making EGL context current with PBuffer surface");
        auto contextCurrentSuccess =
            eglMakeCurrent(eglDisplay, pBuffer, pBuffer, eglContext);
        if (!contextCurrentSuccess)
        {
            auto error = eglGetError();
            std::string errMsg = "Failed to make EGL context current. Error: " + errorString(error);
            LOGE(RC_PREFIX "%s", errMsg.c_str());
            return {false, error, "Failed to make EGL context current"};
        }

        LOGD(RC_PREFIX "Creating Rive RenderContextGL");
        riveContext = rive::gpu::RenderContextGLImpl::MakeContext();
        if (!riveContext)
        {
            auto error = eglGetError();
            std::string errMsg = "Failed to create Rive RenderContextGL. Error: " + errorString(error);
            LOGE(RC_PREFIX "%s", errMsg.c_str());
            return {false, error, "Failed to create Rive RenderContextGL"};
        }

        // Create AndroidFactory for platform-native image decoding
        // This is done after riveContext is created since AndroidFactory delegates to it
        createAndroidFactory();

        return {true, EGL_SUCCESS, "RenderContextGL initialized successfully"};
    }

    /**
     * Destroy the RenderContextGL by releasing the EGL context and destroying
     * the PBuffer surface.
     */
    void destroy() override
    {
        // Cleanup the EGL context and surface
        LOGD(RC_PREFIX "Releasing EGL context and surface bindings");
        eglMakeCurrent(eglDisplay,
                       EGL_NO_SURFACE,
                       EGL_NO_SURFACE,
                       EGL_NO_CONTEXT);
        if (pBuffer != EGL_NO_SURFACE)
        {
            LOGD(RC_PREFIX "Destroying PBuffer surface");
            eglDestroySurface(eglDisplay, pBuffer);
            pBuffer = EGL_NO_SURFACE;
            if (auto error = eglGetError(); error != EGL_SUCCESS)
            {
                std::string errMsg = "Failed to destroy PBuffer surface. Error: " + errorString(error);
                LOGE(RC_PREFIX "%s", errMsg.c_str());
            }
        }
    }

    /** Bind the EGL context to the provided surface for rendering. */
    void beginFrame(void* surface) override
    {
        auto eglSurface = static_cast<EGLSurface>(surface);
        
        // Diagnostic logging for grey box debugging
        LOGD(RC_PREFIX "beginFrame: DIAGNOSTIC - surface ptr=%p, eglSurface=%p, eglDisplay=%p, eglContext=%p",
             surface, eglSurface, eglDisplay, eglContext);
        
        EGLBoolean result = eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
        if (!result)
        {
            EGLint error = eglGetError();
            std::string errMsg = "Failed to make EGL context current in beginFrame. Error: " + errorString(error);
            LOGE(RC_PREFIX "%s", errMsg.c_str());
        }
        else
        {
            LOGD(RC_PREFIX "beginFrame: DIAGNOSTIC - eglMakeCurrent succeeded");
            
            // Verify current context
            EGLContext currentCtx = eglGetCurrentContext();
            EGLSurface currentDraw = eglGetCurrentSurface(EGL_DRAW);
            EGLSurface currentRead = eglGetCurrentSurface(EGL_READ);
            LOGD(RC_PREFIX "beginFrame: DIAGNOSTIC - current context=%p, draw surface=%p, read surface=%p",
                 currentCtx, currentDraw, currentRead);
        }
    }

    /** Swap the EGL buffers on the provided surface to present the frame. */
    void present(void* surface) override
    {
        auto eglSurface = static_cast<EGLSurface>(surface);
        
        // Diagnostic logging for grey box debugging
        LOGD(RC_PREFIX "DIAGNOSTIC - present: surface ptr=%p, eglSurface=%p", surface, eglSurface);
        
        EGLBoolean result = eglSwapBuffers(eglDisplay, eglSurface);
        if (!result)
        {
            EGLint error = eglGetError();
            std::string errMsg = "Failed to swap EGL buffers in present. Error: " + errorString(error);
            LOGE(RC_PREFIX "%s (surface=%p)", errMsg.c_str(), eglSurface);
        }
        else
        {
            LOGD(RC_PREFIX "DIAGNOSTIC - present: eglSwapBuffers succeeded");
        }
    }

    /**
     * Get the Rive Factory for creating render objects.
     * 
     * On Android, this returns an AndroidFactory that uses platform-native
     * image decoding via BitmapFactory. All other factory methods delegate
     * to the underlying riveContext.
     * 
     * @return The AndroidFactory, or nullptr if not initialized.
     */
    rive::Factory* getFactory() const override
    {
        if (m_androidFactory)
        {
            return m_androidFactory.get();
        }
        // Fallback to riveContext if AndroidFactory not available
        return riveContext ? riveContext.get() : nullptr;
    }

    EGLDisplay eglDisplay;
    EGLContext eglContext;

private:
    /**
     * Create the AndroidFactory for platform-native image decoding.
     * Called after riveContext is initialized.
     */
    void createAndroidFactory();

    /** AndroidFactory for platform-native image decoding on Android.
     * Must be declared before pBuffer to ensure correct initialization order. */
    std::unique_ptr<AndroidFactory> m_androidFactory;

    /** A 1x1 PBuffer to bind to the context (some devices do not support
     * surface-less bindings).
     * We must have a valid binding for `MakeContext` to succeed. */
    EGLSurface pBuffer;
};

} // namespace rive_mp