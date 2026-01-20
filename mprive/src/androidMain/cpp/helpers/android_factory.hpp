#pragma once

#include "rive/factory.hpp"
#include "rive/renderer/rive_render_factory.hpp"

namespace rive_mp
{

// Forward declaration to avoid circular include with render_context.hpp
class RenderContext;

/**
 * AndroidFactory wraps the GPU RenderContext's factory and overrides
 * decodeImage() to use Android's BitmapFactory for platform-native decoding.
 * 
 * ## Why This Wrapper Is Needed
 * 
 * The rive-runtime's built-in image decoders (libpng, libjpeg, libwebp) require
 * these libraries to be properly built and linked. On Android, we can instead
 * use the platform's BitmapFactory which:
 * - Handles all Android-supported formats (PNG, JPEG, WebP, HEIF, AVIF, etc.)
 * - Is hardware-accelerated on modern devices
 * - Adds zero binary size (uses system libraries)
 * - Is battle-tested and automatically updated with the OS
 * 
 * ## How It Works
 * 
 * 1. All methods except decodeImage() delegate to the underlying riveContext
 * 2. decodeImage() calls renderImageFromAndroidDecode() which uses JNI to
 *    invoke Android's BitmapFactory
 * 3. The decoded RGBA pixels are then uploaded to GPU via the riveContext
 * 
 * ## Thread Safety
 * 
 * This factory must only be used on the render thread where the GL context
 * is current. Image decoding involves JNI calls that should happen on the
 * command server thread.
 */
class AndroidFactory : public rive::Factory
{
public:
    /**
     * Construct an AndroidFactory that wraps the given RenderContext.
     * 
     * @param renderContext The RenderContext that provides the GPU factory.
     *                      Must not be null and must outlive this factory.
     */
    explicit AndroidFactory(RenderContext* renderContext);

    ~AndroidFactory() override = default;

    // =========================================================================
    // rive::Factory interface
    // =========================================================================

    /**
     * Decode an image using Android's BitmapFactory.
     * This is the key method that makes this factory valuable - it uses
     * platform-native decoding instead of rive-runtime's built-in decoders.
     */
    rive::rcp<rive::RenderImage> decodeImage(
        rive::Span<const uint8_t> encodedBytes) override;

    /**
     * Create a render buffer. Delegates to the underlying riveContext.
     */
    rive::rcp<rive::RenderBuffer> makeRenderBuffer(
        rive::RenderBufferType type,
        rive::RenderBufferFlags flags,
        size_t sizeInBytes) override;

    /**
     * Create a linear gradient shader. Delegates to the underlying riveContext.
     */
    rive::rcp<rive::RenderShader> makeLinearGradient(
        float sx,
        float sy,
        float ex,
        float ey,
        const rive::ColorInt colors[],
        const float stops[],
        size_t count) override;

    /**
     * Create a radial gradient shader. Delegates to the underlying riveContext.
     */
    rive::rcp<rive::RenderShader> makeRadialGradient(
        float cx,
        float cy,
        float radius,
        const rive::ColorInt colors[],
        const float stops[],
        size_t count) override;

    /**
     * Create a render path from a raw path. Delegates to the underlying riveContext.
     */
    rive::rcp<rive::RenderPath> makeRenderPath(
        rive::RawPath& rawPath,
        rive::FillRule fillRule) override;

    /**
     * Create an empty render path. Delegates to the underlying riveContext.
     */
    rive::rcp<rive::RenderPath> makeEmptyRenderPath() override;

    /**
     * Create a render paint. Delegates to the underlying riveContext.
     */
    rive::rcp<rive::RenderPaint> makeRenderPaint() override;

private:
    RenderContext* m_renderContext;
};

} // namespace rive_mp