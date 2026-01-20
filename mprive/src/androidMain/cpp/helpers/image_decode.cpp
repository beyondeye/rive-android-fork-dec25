#include "helpers/image_decode.hpp"
#include "render_context.hpp"
#include "jni_refs.hpp"
#include "rive_log.hpp"

using namespace rive;

namespace rive_mp
{
const uint32_t LSB_MASK = 0xFFu;

/**
 * Helper to find a class using JNI.
 * Returns a local reference that must be deleted after use.
 */
static jclass FindClass(JNIEnv* env, const char* className)
{
    jclass cls = env->FindClass(className);
    if (cls == nullptr)
    {
        LOGE("FindClass failed for: %s", className);
        // Clear any pending exception
        if (env->ExceptionCheck())
        {
            env->ExceptionClear();
        }
    }
    return cls;
}

/**
 * Convert size_t to jint safely.
 */
static jint SizeTToJInt(size_t value)
{
    if (value > static_cast<size_t>(INT32_MAX))
    {
        LOGW("Size %zu exceeds INT32_MAX, truncating", value);
        return INT32_MAX;
    }
    return static_cast<jint>(value);
}

rive::rcp<rive::RenderImage> renderImageFromAndroidDecode(
    Span<const uint8_t> encodedBytes,
    RenderContext* renderContext)
{
    if (renderContext == nullptr)
    {
        LOGE("renderImageFromAndroidDecode: renderContext is null");
        return nullptr;
    }

    auto env = GetJNIEnv();
    if (env == nullptr)
    {
        LOGE("renderImageFromAndroidDecode: Failed to get JNI environment");
        return nullptr;
    }

    // Find the ImageDecoder class in mprive package
    auto imageDecoderClass = FindClass(env, "app/rive/mp/core/ImageDecoder");
    if (imageDecoderClass == nullptr)
    {
        LOGE("renderImageFromAndroidDecode: ImageDecoder class not found");
        return nullptr;
    }

    // Get the decodeToBitmap method
    auto decodeToBitmap = env->GetStaticMethodID(imageDecoderClass,
                                                 "decodeToBitmap",
                                                 "([B)[I");
    if (decodeToBitmap == nullptr)
    {
        LOGE("renderImageFromAndroidDecode: decodeToBitmap method not found");
        env->DeleteLocalRef(imageDecoderClass);
        return nullptr;
    }

    // Create byte array from encoded bytes
    auto encoded = env->NewByteArray(SizeTToJInt(encodedBytes.size()));
    if (encoded == nullptr)
    {
        LOGE("renderImageFromAndroidDecode: Failed to allocate NewByteArray");
        env->DeleteLocalRef(imageDecoderClass);
        return nullptr;
    }

    env->SetByteArrayRegion(encoded,
                            0,
                            SizeTToJInt(encodedBytes.size()),
                            reinterpret_cast<const jbyte*>(encodedBytes.data()));

    // Call decodeToBitmap
    auto jPixels = static_cast<jintArray>(
        env->CallStaticObjectMethod(imageDecoderClass, decodeToBitmap, encoded));
    
    // Clean up local refs
    env->DeleteLocalRef(encoded);
    env->DeleteLocalRef(imageDecoderClass);

    // Check for exceptions
    if (env->ExceptionCheck())
    {
        LOGE("renderImageFromAndroidDecode: JNI exception during decodeToBitmap");
        env->ExceptionClear();
        return nullptr;
    }

    if (jPixels == nullptr)
    {
        LOGE("renderImageFromAndroidDecode: ImageDecoder.decodeToBitmap returned null");
        return nullptr;
    }

    // Process the decoded results
    jsize arrayCount = env->GetArrayLength(jPixels);
    if (arrayCount < 2)
    {
        LOGE("renderImageFromAndroidDecode: Bad array length (unexpected)");
        env->DeleteLocalRef(jPixels);
        return nullptr;
    }

    auto rawPixels = env->GetIntArrayElements(jPixels, nullptr);
    if (rawPixels == nullptr)
    {
        LOGE("renderImageFromAndroidDecode: Failed to get int array elements");
        env->DeleteLocalRef(jPixels);
        return nullptr;
    }

    const auto rawWidth = static_cast<uint32_t>(rawPixels[0]);
    const auto rawHeight = static_cast<uint32_t>(rawPixels[1]);
    const size_t pixelCount = static_cast<size_t>(rawWidth) * rawHeight;
    
    if (pixelCount == 0)
    {
        LOGE("renderImageFromAndroidDecode: Unsupported empty image (zero dimension)");
        env->ReleaseIntArrayElements(jPixels, rawPixels, JNI_ABORT);
        env->DeleteLocalRef(jPixels);
        return nullptr;
    }
    
    if (static_cast<size_t>(arrayCount) < 2u + pixelCount)
    {
        LOGE("renderImageFromAndroidDecode: Not enough elements in pixel array");
        env->ReleaseIntArrayElements(jPixels, rawPixels, JNI_ABORT);
        env->DeleteLocalRef(jPixels);
        return nullptr;
    }

    LOGD("renderImageFromAndroidDecode: Decoded image %ux%u", rawWidth, rawHeight);

    // Convert ARGB to premultiplied RGBA
    std::unique_ptr<uint8_t[]> out(new uint8_t[pixelCount * 4]);
    auto* bytes = out.get();
    
    // Android BitmapFactory returns non-premultiplied ARGB
    const bool isPremultiplied = false;
    
    for (size_t i = 0; i < pixelCount; ++i)
    {
        auto p = static_cast<uint32_t>(rawPixels[2 + i]);
        uint32_t a = (p >> 24) & LSB_MASK;
        uint32_t r = (p >> 16) & LSB_MASK;
        uint32_t g = (p >> 8) & LSB_MASK;
        uint32_t b = (p >> 0) & LSB_MASK;
        
        if (!isPremultiplied)
        {
            r = premultiply(static_cast<uint8_t>(r), static_cast<uint8_t>(a));
            g = premultiply(static_cast<uint8_t>(g), static_cast<uint8_t>(a));
            b = premultiply(static_cast<uint8_t>(b), static_cast<uint8_t>(a));
        }
        
        bytes[0] = static_cast<uint8_t>(r);
        bytes[1] = static_cast<uint8_t>(g);
        bytes[2] = static_cast<uint8_t>(b);
        bytes[3] = static_cast<uint8_t>(a);
        bytes += 4;
    }
    
    env->ReleaseIntArrayElements(jPixels, rawPixels, JNI_ABORT);
    env->DeleteLocalRef(jPixels);

    // Create GPU render image using the render context
    return renderContext->makeImage(rawWidth, rawHeight, std::move(out));
}

uint32_t premultiply(uint8_t c, uint8_t a)
{
    switch (a)
    {
        case 0:
            return 0;
        case 255:
            return c;
        default:
            // Slightly faster than (c * a + 127) / 255
            return (c * a + 128) * 257 >> 16;
    }
}

uint32_t unpremultiply(uint8_t c, uint8_t a)
{
    if (a == 0)
        return 0;
    auto out = (c * 255u + (a / 2u)) / a;
    return out > 255u ? 255u : out;
}

} // namespace rive_mp