#include "render_buffer.hpp"
#include "rive_log.hpp"
#include "platform.hpp"

// Include OpenGL headers based on platform
#if RIVE_PLATFORM_ANDROID
    #include <GLES3/gl3.h>
#elif RIVE_PLATFORM_IOS
    #include <OpenGLES/ES3/gl.h>
#elif RIVE_PLATFORM_DESKTOP
    // Desktop OpenGL
    #if defined(__APPLE__)
        #include <OpenGL/gl3.h>
    #else
        #include <GL/gl.h>
    #endif
#elif RIVE_PLATFORM_WASM
    #include <GLES3/gl3.h>
#else
    #error "Unsupported platform for RenderBuffer"
#endif

#include <rive/artboard.hpp>
#include <rive/renderer.hpp>
#include <rive/math/aabb.hpp>

namespace rive_mp {

RenderBuffer::RenderBuffer(int width, int height)
    : m_width(width)
    , m_height(height)
    , m_fbo(0)
    , m_texture(0)
    , m_depthStencilRenderbuffer(0)
{
    LOGD("RenderBuffer::RenderBuffer - Creating buffer");
    
    if (width <= 0 || height <= 0) {
        LOGE("Invalid buffer dimensions");
        return;
    }
    
    // Allocate pixel buffer
    m_pixels.resize(width * height * 4); // RGBA
    
    // Create OpenGL resources
    if (!createFramebuffer()) {
        LOGE("Failed to create framebuffer");
        return;
    }
    
    LOGD("RenderBuffer created successfully");
}

RenderBuffer::~RenderBuffer()
{
    LOGD("RenderBuffer::~RenderBuffer - Destroying buffer");
    destroyFramebuffer();
}

bool RenderBuffer::createFramebuffer()
{
    // Generate framebuffer
    glGenFramebuffers(1, &m_fbo);
    if (m_fbo == 0) {
        LOGE("Failed to generate framebuffer");
        return false;
    }
    
    // Bind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    
    // Create color texture
    glGenTextures(1, &m_texture);
    if (m_texture == 0) {
        LOGE("Failed to generate texture");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
        return false;
    }
    
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Attach texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);
    
    // Create depth/stencil renderbuffer (required for Rive rendering)
    glGenRenderbuffers(1, &m_depthStencilRenderbuffer);
    if (m_depthStencilRenderbuffer == 0) {
        LOGE("Failed to generate depth/stencil renderbuffer");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteTextures(1, &m_texture);
        glDeleteFramebuffers(1, &m_fbo);
        m_texture = 0;
        m_fbo = 0;
        return false;
    }
    
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthStencilRenderbuffer);
    
    // Use combined depth/stencil format if available
#if RIVE_PLATFORM_ANDROID || RIVE_PLATFORM_IOS
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
#else
    // Desktop OpenGL
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
#endif
    
    // Attach depth/stencil to framebuffer
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthStencilRenderbuffer);
    
    // Check framebuffer completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("Framebuffer is not complete");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        destroyFramebuffer();
        return false;
    }
    
    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    LOGD("Framebuffer created successfully");
    return true;
}

void RenderBuffer::destroyFramebuffer()
{
    if (m_depthStencilRenderbuffer != 0) {
        glDeleteRenderbuffers(1, &m_depthStencilRenderbuffer);
        m_depthStencilRenderbuffer = 0;
    }
    
    if (m_texture != 0) {
        glDeleteTextures(1, &m_texture);
        m_texture = 0;
    }
    
    if (m_fbo != 0) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
}

void RenderBuffer::render(
    rive::Artboard* artboard,
    rive::Renderer* renderer,
    rive::Fit fit,
    rive::Alignment alignment)
{
    if (!isValid()) {
        LOGE("RenderBuffer is not valid");
        return;
    }
    
    if (artboard == nullptr) {
        LOGE("Artboard is null");
        return;
    }
    
    if (renderer == nullptr) {
        LOGE("Renderer is null");
        return;
    }
    
    // Save current framebuffer binding
    GLint previousFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFBO);
    
    // Save current viewport
    GLint previousViewport[4];
    glGetIntegerv(GL_VIEWPORT, previousViewport);
    
    // Bind our framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    
    // Set viewport to match buffer size
    glViewport(0, 0, m_width, m_height);
    
    // Clear buffer with transparent color
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    // Calculate artboard transform
    rive::AABB artboardBounds = artboard->bounds();
    float artboardWidth = artboardBounds.width();
    float artboardHeight = artboardBounds.height();
    
    // Compute the fit and alignment transform
    // This matches the logic in the existing Rive SDK
    float scaleX = m_width / artboardWidth;
    float scaleY = m_height / artboardHeight;
    float scale = 1.0f;
    
    // Apply fit strategy
    switch (fit) {
        case rive::Fit::fill:
            // Stretch to fill (different scale for X and Y)
            scaleX = scaleX;
            scaleY = scaleY;
            break;
        case rive::Fit::contain:
            // Fit inside (maintain aspect ratio, may have letterboxing)
            scale = std::min(scaleX, scaleY);
            scaleX = scale;
            scaleY = scale;
            break;
        case rive::Fit::cover:
            // Cover entire area (maintain aspect ratio, may crop)
            scale = std::max(scaleX, scaleY);
            scaleX = scale;
            scaleY = scale;
            break;
        case rive::Fit::fitWidth:
            scale = scaleX;
            scaleX = scale;
            scaleY = scale;
            break;
        case rive::Fit::fitHeight:
            scale = scaleY;
            scaleX = scale;
            scaleY = scale;
            break;
        case rive::Fit::none:
            scaleX = 1.0f;
            scaleY = 1.0f;
            break;
        case rive::Fit::scaleDown:
            if (artboardWidth > m_width || artboardHeight > m_height) {
                scale = std::min(scaleX, scaleY);
                scaleX = scale;
                scaleY = scale;
            } else {
                scaleX = 1.0f;
                scaleY = 1.0f;
            }
            break;
    }
    
    // Calculate translation based on alignment
    float scaledWidth = artboardWidth * scaleX;
    float scaledHeight = artboardHeight * scaleY;
    float translateX = 0.0f;
    float translateY = 0.0f;
    
    // Horizontal alignment
    int alignmentValue = static_cast<int>(alignment);
    int horizontalAlign = alignmentValue % 3; // 0=left, 1=center, 2=right
    
    switch (horizontalAlign) {
        case 0: // Left
            translateX = 0.0f;
            break;
        case 1: // Center
            translateX = (m_width - scaledWidth) * 0.5f;
            break;
        case 2: // Right
            translateX = m_width - scaledWidth;
            break;
    }
    
    // Vertical alignment
    int verticalAlign = alignmentValue / 3; // 0=top, 1=center, 2=bottom
    
    switch (verticalAlign) {
        case 0: // Top
            translateY = 0.0f;
            break;
        case 1: // Center
            translateY = (m_height - scaledHeight) * 0.5f;
            break;
        case 2: // Bottom
            translateY = m_height - scaledHeight;
            break;
    }
    
    // Apply transform to renderer
    renderer->save();
    renderer->translate(translateX, translateY);
    renderer->scale(scaleX, scaleY);
    
    // Render artboard
    artboard->draw(renderer);
    
    // Restore renderer state
    renderer->restore();
    
    // Flush renderer (ensure all commands are submitted)
    // Note: The renderer should handle this internally
    
    // Read pixels from GPU to CPU
    // WARNING: This is the expensive operation (GPU → CPU copy)
    glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, m_pixels.data());
    
    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        LOGE("OpenGL error after glReadPixels");
    }
    
    // Note: Pixels are in bottom-up order (OpenGL convention)
    // If needed, we could flip them here, but it's more efficient to do it
    // when copying to the bitmap on the Kotlin side
    
    // Restore previous framebuffer and viewport
    glBindFramebuffer(GL_FRAMEBUFFER, previousFBO);
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
}

void RenderBuffer::resize(int width, int height)
{
    if (width <= 0 || height <= 0) {
        LOGE("Invalid resize dimensions");
        return;
    }
    
    if (width == m_width && height == m_height) {
        // No change
        return;
    }
    
    LOGD("RenderBuffer::resize - Resizing buffer");
    
    // Destroy old resources
    destroyFramebuffer();
    
    // Update dimensions
    m_width = width;
    m_height = height;
    
    // Resize pixel buffer
    m_pixels.resize(width * height * 4);
    
    // Create new resources
    if (!createFramebuffer()) {
        LOGE("Failed to recreate framebuffer after resize");
    }
}

} // namespace rive_mp
