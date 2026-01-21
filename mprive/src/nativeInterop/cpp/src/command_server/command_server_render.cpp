#include "command_server.hpp"
#include "render_context.hpp"
#include "rive_log.hpp"
#include "rive/renderer/rive_renderer.hpp"
#include "rive/renderer/gl/render_target_gl.hpp"
#include "rive/math/aabb.hpp"
#include <GLES3/gl3.h>

namespace rive_android {

// =============================================================================
// Phase C.2.3: Render Target Operations - Public API
// =============================================================================

void CommandServer::createRenderTarget(int64_t requestID, int32_t width, int32_t height, int32_t sampleCount)
{
    LOGI("CommandServer: Enqueuing CreateRenderTarget command (requestID=%lld, width=%d, height=%d, sampleCount=%d)",
         static_cast<long long>(requestID), width, height, sampleCount);

    Command cmd(CommandType::CreateRenderTarget, requestID);
    cmd.rtWidth = width;
    cmd.rtHeight = height;
    cmd.sampleCount = sampleCount;

    enqueueCommand(std::move(cmd));
}

void CommandServer::deleteRenderTarget(int64_t requestID, int64_t renderTargetHandle)
{
    LOGI("CommandServer: Enqueuing DeleteRenderTarget command (requestID=%lld, handle=%lld)",
         static_cast<long long>(requestID), static_cast<long long>(renderTargetHandle));

    Command cmd(CommandType::DeleteRenderTarget, requestID);
    cmd.handle = renderTargetHandle;

    enqueueCommand(std::move(cmd));
}

// =============================================================================
// Phase C.2.6: Rendering Operations - Public API
// =============================================================================

void CommandServer::draw(int64_t requestID,
                          int64_t artboardHandle,
                          int64_t smHandle,
                          int64_t surfacePtr,
                          int64_t renderTargetPtr,
                          int64_t drawKey,
                          int32_t width,
                          int32_t height,
                          int32_t fitMode,
                          int32_t alignmentMode,
                          uint32_t clearColor,
                          float scaleFactor)
{
    LOGI("CommandServer: Enqueuing Draw command (requestID=%lld, artboard=%lld, sm=%lld)",
         static_cast<long long>(requestID),
         static_cast<long long>(artboardHandle),
         static_cast<long long>(smHandle));

    Command cmd(CommandType::Draw, requestID);
    cmd.artboardHandle = artboardHandle;
    cmd.smHandle = smHandle;
    cmd.surfacePtr = surfacePtr;
    cmd.renderTargetPtr = renderTargetPtr;
    cmd.drawKey = drawKey;
    cmd.surfaceWidth = width;
    cmd.surfaceHeight = height;
    cmd.fitMode = fitMode;
    cmd.alignmentMode = alignmentMode;
    cmd.clearColor = clearColor;
    cmd.scaleFactor = scaleFactor;

    enqueueCommand(std::move(cmd));
}

// =============================================================================
// Phase C.2.3: Render Target Operations - Handlers
// =============================================================================

void CommandServer::handleCreateRenderTarget(const Command& cmd)
{
    LOGI("CommandServer: Handling CreateRenderTarget command (requestID=%lld, %dx%d, samples=%d)",
         static_cast<long long>(cmd.requestID), cmd.rtWidth, cmd.rtHeight, cmd.sampleCount);

    // Validate dimensions
    if (cmd.rtWidth <= 0 || cmd.rtHeight <= 0) {
        LOGW("CommandServer: Invalid render target dimensions: %dx%d", cmd.rtWidth, cmd.rtHeight);

        Message msg(MessageType::RenderTargetError, cmd.requestID);
        msg.error = "Invalid render target dimensions";
        enqueueMessage(std::move(msg));
        return;
    }

    // TODO (Phase C.2.6 full rendering): Create actual rive::gpu::FramebufferRenderTargetGL
    // This requires:
    // - rive::gpu::RenderContext to be available and initialized
    // - GL context to be current on this thread
    // - Proper GPU renderer setup
    //
    // Placeholder implementation:
    // For now, we create a placeholder handle to unblock development.
    // The actual render target creation will be implemented when full GPU rendering
    // is integrated in Phase C.2.6.

    int64_t handle = m_nextHandle.fetch_add(1);

    // Store nullptr for now - will be replaced with actual RenderTargetGL in Phase C.2.6
    m_renderTargets[handle] = nullptr;

    LOGI("CommandServer: Created render target handle %lld (placeholder)", static_cast<long long>(handle));

    Message msg(MessageType::RenderTargetCreated, cmd.requestID);
    msg.handle = handle;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleDeleteRenderTarget(const Command& cmd)
{
    LOGI("CommandServer: Handling DeleteRenderTarget command (requestID=%lld, handle=%lld)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle));

    auto it = m_renderTargets.find(cmd.handle);
    if (it == m_renderTargets.end()) {
        LOGW("CommandServer: Invalid render target handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::RenderTargetError, cmd.requestID);
        msg.error = "Invalid render target handle";
        enqueueMessage(std::move(msg));
        return;
    }

    // TODO (Phase C.2.6 full rendering): Delete actual rive::gpu::RenderTargetGL
    // if (it->second != nullptr) {
    //     delete it->second;
    // }

    m_renderTargets.erase(it);

    LOGI("CommandServer: Deleted render target handle %lld", static_cast<long long>(cmd.handle));

    Message msg(MessageType::RenderTargetDeleted, cmd.requestID);
    enqueueMessage(std::move(msg));
}

// =============================================================================
// Phase C.2.6: Rendering Operations - Helper Functions
// =============================================================================

/**
 * Convert Fit ordinal to rive::Fit enum.
 * Ordinal mapping:
 * 0=FILL, 1=CONTAIN, 2=COVER, 3=FIT_WIDTH, 4=FIT_HEIGHT, 5=NONE, 6=SCALE_DOWN, 7=LAYOUT
 */
static rive::Fit getFitFromOrdinal(int32_t ordinal)
{
    switch (ordinal)
    {
        case 0: return rive::Fit::fill;
        case 1: return rive::Fit::contain;
        case 2: return rive::Fit::cover;
        case 3: return rive::Fit::fitWidth;
        case 4: return rive::Fit::fitHeight;
        case 5: return rive::Fit::none;
        case 6: return rive::Fit::scaleDown;
        case 7: return rive::Fit::layout;
        default:
            LOGW("CommandServer: Invalid Fit ordinal %d, defaulting to CONTAIN", ordinal);
            return rive::Fit::contain;
    }
}

/**
 * Convert Alignment ordinal to rive::Alignment enum.
 * Ordinal mapping:
 * 0=TOP_LEFT, 1=TOP_CENTER, 2=TOP_RIGHT,
 * 3=CENTER_LEFT, 4=CENTER, 5=CENTER_RIGHT,
 * 6=BOTTOM_LEFT, 7=BOTTOM_CENTER, 8=BOTTOM_RIGHT
 */
static rive::Alignment getAlignmentFromOrdinal(int32_t ordinal)
{
    switch (ordinal)
    {
        case 0: return rive::Alignment::topLeft;
        case 1: return rive::Alignment::topCenter;
        case 2: return rive::Alignment::topRight;
        case 3: return rive::Alignment::centerLeft;
        case 4: return rive::Alignment::center;
        case 5: return rive::Alignment::centerRight;
        case 6: return rive::Alignment::bottomLeft;
        case 7: return rive::Alignment::bottomCenter;
        case 8: return rive::Alignment::bottomRight;
        default:
            LOGW("CommandServer: Invalid Alignment ordinal %d, defaulting to CENTER", ordinal);
            return rive::Alignment::center;
    }
}

// =============================================================================
// Phase C.2.6: Rendering Operations - Handler
// =============================================================================

void CommandServer::handleDraw(const Command& cmd)
{
    LOGI("CommandServer: Handling Draw command (requestID=%lld, artboard=%lld, sm=%lld, %dx%d)",
         static_cast<long long>(cmd.requestID),
         static_cast<long long>(cmd.artboardHandle),
         static_cast<long long>(cmd.smHandle),
         cmd.surfaceWidth, cmd.surfaceHeight);
    
    // Diagnostic: Log all draw parameters
    LOGD("CommandServer: Draw params - surfacePtr=%lld, renderTargetPtr=%lld, drawKey=%lld",
         static_cast<long long>(cmd.surfacePtr),
         static_cast<long long>(cmd.renderTargetPtr),
         static_cast<long long>(cmd.drawKey));
    LOGD("CommandServer: Draw params - fit=%d, alignment=%d, scaleFactor=%.2f, clearColor=0x%08X",
         cmd.fitMode, cmd.alignmentMode, cmd.scaleFactor, cmd.clearColor);

    // 1. Validate artboard handle
    auto artboardIt = m_artboards.find(cmd.artboardHandle);
    if (artboardIt == m_artboards.end()) {
        LOGW("CommandServer: Invalid artboard handle: %lld", static_cast<long long>(cmd.artboardHandle));

        Message msg(MessageType::DrawError, cmd.requestID);
        msg.error = "Invalid artboard handle";
        enqueueMessage(std::move(msg));
        return;
    }

    // 2. Validate state machine handle (optional - can be 0 for static artboards)
    rive::StateMachineInstance* sm = nullptr;
    if (cmd.smHandle != 0) {
        auto smIt = m_stateMachines.find(cmd.smHandle);
        if (smIt == m_stateMachines.end()) {
            LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.smHandle));

            Message msg(MessageType::DrawError, cmd.requestID);
            msg.error = "Invalid state machine handle";
            enqueueMessage(std::move(msg));
            return;
        }
        sm = smIt->second.get();
    }

    // Get ArtboardInstance from BindableArtboard
    auto* artboard = artboardIt->second->artboard();
    if (!artboard) {
        LOGW("CommandServer: BindableArtboard has no artboard instance: %lld", static_cast<long long>(cmd.artboardHandle));

        Message msg(MessageType::DrawError, cmd.requestID);
        msg.error = "BindableArtboard has no artboard instance";
        enqueueMessage(std::move(msg));
        return;
    }

    // 3. Check render context
    if (m_renderContext == nullptr) {
        LOGW("CommandServer: No render context available");

        Message msg(MessageType::DrawError, cmd.requestID);
        msg.error = "No render context available";
        enqueueMessage(std::move(msg));
        return;
    }

    // 4. Get RenderContext and validate Rive GPU RenderContext
    auto* renderContext = static_cast<rive_mp::RenderContext*>(m_renderContext);
    if (renderContext->riveContext == nullptr) {
        LOGW("CommandServer: Rive GPU RenderContext not initialized");

        Message msg(MessageType::DrawError, cmd.requestID);
        msg.error = "Rive GPU RenderContext not initialized";
        enqueueMessage(std::move(msg));
        return;
    }

    // 5. Get render target
    auto* renderTarget = reinterpret_cast<rive::gpu::RenderTargetGL*>(cmd.renderTargetPtr);
    if (renderTarget == nullptr) {
        LOGW("CommandServer: Invalid render target pointer");

        Message msg(MessageType::DrawError, cmd.requestID);
        msg.error = "Invalid render target pointer";
        enqueueMessage(std::move(msg));
        return;
    }

    // 6. Make EGL context current for this surface
    void* surfacePtr = reinterpret_cast<void*>(cmd.surfacePtr);
    LOGD("CommandServer: About to call beginFrame with surfacePtr=%p (from %lld)",
         surfacePtr, static_cast<long long>(cmd.surfacePtr));
    renderContext->beginFrame(surfacePtr);
    
    // Check GL errors after making context current
    GLenum glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        LOGW("CommandServer: GL error after beginFrame: 0x%04X", glErr);
    }

    // DIAGNOSTIC: Check riveContext state BEFORE beginFrame
    LOGW("CommandServer: DIAGNOSTIC - riveContext=%p, riveContext valid=%d",
         renderContext->riveContext.get(),
         renderContext->riveContext != nullptr);
    
    // 7. Begin Rive GPU frame with clear
    // Extract RGBA components from 0xAARRGGBB format
    uint32_t clearColor = cmd.clearColor;
    LOGD("CommandServer: Calling riveContext->beginFrame(%dx%d, clearColor=0x%08X)",
         cmd.surfaceWidth, cmd.surfaceHeight, clearColor);
    renderContext->riveContext->beginFrame({
        .renderTargetWidth = static_cast<uint32_t>(cmd.surfaceWidth),
        .renderTargetHeight = static_cast<uint32_t>(cmd.surfaceHeight),
        .loadAction = rive::gpu::LoadAction::clear,
        .clearColor = clearColor
    });
    
    // Check GL errors after Rive beginFrame
    glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        LOGW("CommandServer: GL error after riveContext->beginFrame: 0x%04X", glErr);
    }

    // 8. Create renderer and apply fit/alignment transformation
    auto renderer = rive::RiveRenderer(renderContext->riveContext.get());

    // Convert ordinals to rive enums
    rive::Fit fit = getFitFromOrdinal(cmd.fitMode);
    rive::Alignment alignment = getAlignmentFromOrdinal(cmd.alignmentMode);

    // Save renderer state before transformation
    renderer.save();

    // DIAGNOSTIC: Try different align configurations
    rive::AABB surfaceBounds(0, 0,
                             static_cast<float>(cmd.surfaceWidth),
                             static_cast<float>(cmd.surfaceHeight));
    
    LOGW("CommandServer: DIAGNOSTIC - artboard bounds: (%.1f, %.1f, %.1f, %.1f)",
         artboard->bounds().minX, artboard->bounds().minY, 
         artboard->bounds().maxX, artboard->bounds().maxY);
    LOGW("CommandServer: DIAGNOSTIC - surface bounds: (%.1f, %.1f, %.1f, %.1f)",
         surfaceBounds.minX, surfaceBounds.minY, surfaceBounds.maxX, surfaceBounds.maxY);
    LOGW("CommandServer: DIAGNOSTIC - fit=%d, alignment=%d, scaleFactor=%.2f",
         cmd.fitMode, cmd.alignmentMode, cmd.scaleFactor);
    
    renderer.align(fit,
                   alignment,
                   surfaceBounds,
                   artboard->bounds(),
                   cmd.scaleFactor);

    // NOTE: artboard->advance() is now called by advanceAndApply() in handleAdvanceStateMachine()
    // This ensures proper synchronization between state machine and artboard updates.
    // Previously, we called artboard->advance(0) here which was overwriting animation state.
    
    // 9. Draw the artboard
    // =========================================================================
    // DIAGNOSTIC: Log artboard content details
    // =========================================================================
    LOGW("CommandServer: ARTBOARD DIAGNOSTIC - name='%s', size=%.0fx%.0f",
         artboard->name().c_str(), artboard->width(), artboard->height());
    LOGW("CommandServer: ARTBOARD DIAGNOSTIC - objectCount=%zu, animationCount=%zu, stateMachineCount=%zu",
         artboard->objects().size(), artboard->animationCount(), artboard->stateMachineCount());
    
    // Log first few objects in the artboard
    auto& objects = artboard->objects();
    size_t objectsToLog = std::min(objects.size(), static_cast<size_t>(10));
    for (size_t i = 0; i < objectsToLog; i++) {
        auto* obj = objects[i];
        if (obj != nullptr) {
            LOGW("CommandServer: ARTBOARD DIAGNOSTIC - object[%zu]: coreType=%u", i, obj->coreType());
        }
    }
    if (objects.size() > 10) {
        LOGW("CommandServer: ARTBOARD DIAGNOSTIC - ... and %zu more objects", objects.size() - 10);
    }
    // =========================================================================
    
    LOGD("CommandServer: Drawing artboard '%s' (%.0fx%.0f)",
         artboard->name().c_str(), artboard->width(), artboard->height());
    artboard->draw(&renderer);

    // Restore renderer state
    renderer.restore();
    
    // Check GL errors after draw
    glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        LOGW("CommandServer: GL error after artboard->draw: 0x%04X", glErr);
    }

    // 10. Set viewport explicitly before flush (like original implementation)
    // This ensures Rive knows the correct target dimensions
    glViewport(0, 0, cmd.surfaceWidth, cmd.surfaceHeight);
    LOGD("CommandServer: Set viewport to (%d, %d)", cmd.surfaceWidth, cmd.surfaceHeight);
    
    // 11. Flush Rive GPU context to submit rendering commands
    LOGD("CommandServer: Flushing to renderTarget=%p (width=%d, height=%d)",
         renderTarget, renderTarget->width(), renderTarget->height());
    renderContext->riveContext->flush({.renderTarget = renderTarget});
    
    // Check GL errors after flush
    glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        LOGW("CommandServer: GL error after riveContext->flush: 0x%04X", glErr);
    }
    
    // Diagnostic: Check current framebuffer binding
    GLint currentFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
    LOGD("CommandServer: After flush, current FBO binding=%d", currentFBO);

    // =========================================================================
    // DIAGNOSTIC: Draw a bright red test quad AFTER flush to verify it appears
    // If you see RED now, it confirms flush was clearing our previous draws
    // =========================================================================
    {
        LOGW("CommandServer: DIAGNOSTIC - Drawing red test quad AFTER FLUSH");
        
        // Simple vertex shader
        const char* vertShader = R"(#version 300 es
            in vec2 aPos;
            void main() {
                gl_Position = vec4(aPos, 0.0, 1.0);
            }
        )";
        
        // Simple fragment shader - outputs bright red
        const char* fragShader = R"(#version 300 es
            precision mediump float;
            out vec4 fragColor;
            void main() {
                fragColor = vec4(1.0, 0.0, 0.0, 1.0); // BRIGHT RED
            }
        )";
        
        // Compile vertex shader
        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vertShader, nullptr);
        glCompileShader(vs);
        
        GLint compiled = 0;
        glGetShaderiv(vs, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            char log[512];
            glGetShaderInfoLog(vs, 512, nullptr, log);
            LOGE("CommandServer: DIAGNOSTIC - Vertex shader compile error: %s", log);
        }
        
        // Compile fragment shader
        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fragShader, nullptr);
        glCompileShader(fs);
        
        glGetShaderiv(fs, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            char log[512];
            glGetShaderInfoLog(fs, 512, nullptr, log);
            LOGE("CommandServer: DIAGNOSTIC - Fragment shader compile error: %s", log);
        }
        
        // Link program
        GLuint program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);
        
        GLint linked = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) {
            char log[512];
            glGetProgramInfoLog(program, 512, nullptr, log);
            LOGE("CommandServer: DIAGNOSTIC - Program link error: %s", log);
        }
        
        // Create a small quad in center of screen (NDC coordinates)
        // Small size (10% of screen) so Rive content is visible behind it
        float vertices[] = {
            -0.1f, -0.1f,  // Bottom-left
             0.1f, -0.1f,  // Bottom-right
             0.1f,  0.1f,  // Top-right
            -0.1f,  0.1f   // Top-left
        };
        unsigned int indices[] = {0, 1, 2, 0, 2, 3};
        
        GLuint vao, vbo, ebo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        
        glBindVertexArray(vao);
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        
        GLint posLoc = glGetAttribLocation(program, "aPos");
        glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(posLoc);
        
        // Set viewport and bind FBO 0 explicitly
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, cmd.surfaceWidth, cmd.surfaceHeight);
        
        // Draw the red quad
        glUseProgram(program);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        // Cleanup
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
        glDeleteProgram(program);
        glDeleteShader(vs);
        glDeleteShader(fs);
        
        glErr = glGetError();
        if (glErr != GL_NO_ERROR) {
            LOGE("CommandServer: DIAGNOSTIC - GL error after drawing red quad: 0x%04X", glErr);
        } else {
            LOGW("CommandServer: DIAGNOSTIC - Red quad drawn AFTER FLUSH successfully");
        }
    }
    // =========================================================================
    // END DIAGNOSTIC
    // =========================================================================

    // 11. Present the frame (swap buffers)
    LOGD("CommandServer: About to call present with surfacePtr=%p", surfacePtr);
    renderContext->present(surfacePtr);
    
    // Check GL errors after present
    glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        LOGW("CommandServer: GL error after present: 0x%04X", glErr);
    }

    // 12. Send success message
    LOGI("CommandServer: Draw command completed successfully (artboard=%s, %dx%d)",
         artboard->name().c_str(),
         static_cast<int>(artboard->width()),
         static_cast<int>(artboard->height()));

    Message msg(MessageType::DrawComplete, cmd.requestID);
    msg.handle = cmd.drawKey;  // Return the draw key for correlation
    enqueueMessage(std::move(msg));
}

} // namespace rive_android