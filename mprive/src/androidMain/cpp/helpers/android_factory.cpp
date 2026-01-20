#include "helpers/android_factory.hpp"
#include "helpers/image_decode.hpp"
#include "render_context.hpp"
#include "rive_log.hpp"

namespace rive_mp
{

AndroidFactory::AndroidFactory(RenderContext* renderContext)
    : m_renderContext(renderContext)
{
    if (m_renderContext == nullptr)
    {
        LOGE("AndroidFactory: Created with null RenderContext!");
    }
    else
    {
        LOGD("AndroidFactory: Created with RenderContext");
    }
}

rive::rcp<rive::RenderImage> AndroidFactory::decodeImage(
    rive::Span<const uint8_t> encodedBytes)
{
    LOGD("AndroidFactory::decodeImage: Decoding %zu bytes using Android BitmapFactory",
         encodedBytes.size());
    
    // Use Android's BitmapFactory through JNI
    return renderImageFromAndroidDecode(encodedBytes, m_renderContext);
}

rive::rcp<rive::RenderBuffer> AndroidFactory::makeRenderBuffer(
    rive::RenderBufferType type,
    rive::RenderBufferFlags flags,
    size_t sizeInBytes)
{
    if (m_renderContext == nullptr || m_renderContext->riveContext == nullptr)
    {
        LOGE("AndroidFactory::makeRenderBuffer: No valid riveContext");
        return nullptr;
    }
    return m_renderContext->riveContext->makeRenderBuffer(type, flags, sizeInBytes);
}

rive::rcp<rive::RenderShader> AndroidFactory::makeLinearGradient(
    float sx,
    float sy,
    float ex,
    float ey,
    const rive::ColorInt colors[],
    const float stops[],
    size_t count)
{
    if (m_renderContext == nullptr || m_renderContext->riveContext == nullptr)
    {
        LOGE("AndroidFactory::makeLinearGradient: No valid riveContext");
        return nullptr;
    }
    return m_renderContext->riveContext->makeLinearGradient(sx, sy, ex, ey, colors, stops, count);
}

rive::rcp<rive::RenderShader> AndroidFactory::makeRadialGradient(
    float cx,
    float cy,
    float radius,
    const rive::ColorInt colors[],
    const float stops[],
    size_t count)
{
    if (m_renderContext == nullptr || m_renderContext->riveContext == nullptr)
    {
        LOGE("AndroidFactory::makeRadialGradient: No valid riveContext");
        return nullptr;
    }
    return m_renderContext->riveContext->makeRadialGradient(cx, cy, radius, colors, stops, count);
}

rive::rcp<rive::RenderPath> AndroidFactory::makeRenderPath(
    rive::RawPath& rawPath,
    rive::FillRule fillRule)
{
    if (m_renderContext == nullptr || m_renderContext->riveContext == nullptr)
    {
        LOGE("AndroidFactory::makeRenderPath: No valid riveContext");
        return nullptr;
    }
    return m_renderContext->riveContext->makeRenderPath(rawPath, fillRule);
}

rive::rcp<rive::RenderPath> AndroidFactory::makeEmptyRenderPath()
{
    if (m_renderContext == nullptr || m_renderContext->riveContext == nullptr)
    {
        LOGE("AndroidFactory::makeEmptyRenderPath: No valid riveContext");
        return nullptr;
    }
    return m_renderContext->riveContext->makeEmptyRenderPath();
}

rive::rcp<rive::RenderPaint> AndroidFactory::makeRenderPaint()
{
    if (m_renderContext == nullptr || m_renderContext->riveContext == nullptr)
    {
        LOGE("AndroidFactory::makeRenderPaint: No valid riveContext");
        return nullptr;
    }
    return m_renderContext->riveContext->makeRenderPaint();
}

} // namespace rive_mp