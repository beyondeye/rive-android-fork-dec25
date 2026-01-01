#pragma once

#include <vector>
#include <cstdint>

// Forward declarations to avoid including OpenGL headers in the interface
typedef unsigned int GLuint;
typedef int GLint;

// Note: We don't forward declare rive::Fit and rive::Alignment here
// because they are fully defined in rive/math/aabb.hpp which we include anyway.
// Forward declaring would cause type conflicts.
namespace rive {
    class Artboard;
    class Renderer;
}

namespace rive_mp {

/**
 * Off-screen rendering buffer for Canvas/Bitmap fallback approach.
 * 
 * This class creates an OpenGL framebuffer object (FBO) for off-screen rendering,
 * renders Rive artboards to it, and provides pixel data (RGBA) that can be copied
 * to a bitmap for display.
 * 
 * WARNING: This is a FALLBACK approach with significant performance overhead:
 * - GPU → CPU pixel copy (glReadPixels) per frame
 * - 5-15ms per frame (vs 0.5-2ms for PLS or 1-3ms for Skia renderer)
 * - 300-600% slower than GPU-accelerated approaches
 * 
 * Only use when:
 * - PLS Renderer unavailable (e.g., no TextureView on Android)
 * - Skia Renderer unavailable (e.g., no Skia canvas on Desktop)
 * 
 * Architecture:
 * ```
 * Artboard → Renderer → FBO (GPU) → glReadPixels → CPU Buffer → Bitmap → Display
 *                                    ^^^^^^^^^^^^^^^
 *                                    EXPENSIVE COPY
 * ```
 */
class RenderBuffer {
public:
    /**
     * Create a render buffer with the specified dimensions.
     * 
     * @param width Buffer width in pixels
     * @param height Buffer height in pixels
     * 
     * NOTE: Requires an active OpenGL context on the calling thread.
     */
    RenderBuffer(int width, int height);
    
    /**
     * Destroy the render buffer and release OpenGL resources.
     * 
     * NOTE: Must be called on the same thread that created the buffer,
     * with the same OpenGL context active.
     */
    ~RenderBuffer();
    
    // Disable copy (OpenGL resources are not copyable)
    RenderBuffer(const RenderBuffer&) = delete;
    RenderBuffer& operator=(const RenderBuffer&) = delete;
    
    /**
     * Render an artboard to the buffer.
     * 
     * This performs the following steps:
     * 1. Bind the FBO for off-screen rendering
     * 2. Clear the buffer with transparent color
     * 3. Set up viewport and matrices
     * 4. Render the artboard using the provided renderer
     * 5. Read pixels back from GPU to CPU (glReadPixels)
     * 6. Unbind the FBO
     * 
     * @param artboard The artboard to render (must not be null)
     * @param renderer The renderer to use (must not be null)
     * @param fitValue How to fit the artboard in the buffer (raw enum value)
     * @param alignmentValue How to align the artboard in the buffer (raw enum value)
     * 
     * WARNING: This function includes an expensive GPU→CPU copy (glReadPixels).
     * Expect 5-15ms per call on typical hardware.
     * 
     * NOTE: We use int parameters instead of rive::Fit/Alignment enums to avoid
     * including Rive headers in this header file (keeping dependencies minimal).
     */
    void render(rive::Artboard* artboard,
                rive::Renderer* renderer,
                int fitValue,
                int alignmentValue);
    
    /**
     * Get the rendered pixel data (RGBA format).
     * 
     * @return Reference to pixel buffer (width * height * 4 bytes)
     * 
     * Format: RGBA8888 (R, G, B, A in that order)
     * - Top-left pixel is at index 0
     * - Row-major order (scanlines from top to bottom)
     * 
     * NOTE: Data is valid until the next render() call or buffer destruction.
     */
    const std::vector<uint8_t>& getPixels() const { return m_pixels; }
    
    /**
     * Get buffer width in pixels.
     */
    int getWidth() const { return m_width; }
    
    /**
     * Get buffer height in pixels.
     */
    int getHeight() const { return m_height; }
    
    /**
     * Resize the buffer to new dimensions.
     * 
     * This destroys and recreates the OpenGL resources.
     * 
     * @param width New width in pixels
     * @param height New height in pixels
     * 
     * NOTE: Requires an active OpenGL context on the calling thread.
     */
    void resize(int width, int height);
    
    /**
     * Check if the buffer is valid (OpenGL resources created successfully).
     * 
     * @return true if valid, false if creation failed
     */
    bool isValid() const { return m_fbo != 0 && m_texture != 0; }

private:
    /**
     * Create OpenGL framebuffer object and texture.
     * 
     * @return true if successful, false on error
     */
    bool createFramebuffer();
    
    /**
     * Destroy OpenGL framebuffer object and texture.
     */
    void destroyFramebuffer();

private:
    int m_width;                        ///< Buffer width in pixels
    int m_height;                       ///< Buffer height in pixels
    GLuint m_fbo;                       ///< OpenGL framebuffer object
    GLuint m_texture;                   ///< OpenGL color texture attachment
    GLuint m_depthStencilRenderbuffer;  ///< OpenGL depth/stencil renderbuffer
    std::vector<uint8_t> m_pixels;      ///< CPU-side pixel buffer (RGBA)
};

} // namespace rive_mp
