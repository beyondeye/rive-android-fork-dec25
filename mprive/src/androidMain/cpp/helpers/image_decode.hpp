#pragma once

#include "rive/refcnt.hpp"
#include "rive/renderer.hpp"
#include "rive/span.hpp"

#include <cstdint>

namespace rive_mp
{

// Forward declaration to avoid circular include with render_context.hpp
class RenderContext;
/**
 * Decode images using Android's BitmapFactory through JNI.
 * 
 * This function is called by AndroidFactory::decodeImage() to decode
 * embedded images in Rive files using the platform-native decoder.
 *
 * @param encodedBytes The encoded bytes of the image (PNG, JPEG, WebP, etc.)
 * @param renderContext The RenderContext to create GPU render images.
 *                      Must not be null.
 * @return A GPU-accelerated RenderImage, or nullptr if decoding failed.
 */
rive::rcp<rive::RenderImage> renderImageFromAndroidDecode(
    rive::Span<const uint8_t> encodedBytes,
    RenderContext* renderContext);

/**
 * Helper functions for pixel format conversion.
 */

/** Premultiply a color channel by alpha. */
uint32_t premultiply(uint8_t c, uint8_t a);

/** Unpremultiply a color channel by alpha. */
uint32_t unpremultiply(uint8_t c, uint8_t a);

} // namespace rive_mp