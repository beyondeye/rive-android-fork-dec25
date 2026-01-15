/**
 * Command Queue Rendering Bindings
 * 
 * This file contains JNI bindings for:
 * - Pointer events (move, down, up, exit)
 * - Render target creation
 * - Draw key creation
 * - Single sprite drawing (to surface and to buffer)
 * - Multi-sprite batch rendering (to surface and to buffer)
 */

#include "bindings_command_queue.hpp"

using namespace rive_android;
using namespace rive_android::bindings;

extern "C"
{
    // =========================================================================
    // Pointer Events
    // =========================================================================

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppPointerMove(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong stateMachineHandle,
        jbyte jFit,
        jbyte jAlignment,
        jfloat layoutScale,
        jfloat surfaceWidth,
        jfloat surfaceHeight,
        jint pointerID,
        jfloat pointerX,
        jfloat pointerY)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        rive::CommandQueue::PointerEvent event{
            .fit = GetFit(static_cast<uint8_t>(jFit)),
            .alignment = GetAlignment(static_cast<uint8_t>(jAlignment)),
            .screenBounds = rive::Vec2D(static_cast<float_t>(surfaceWidth),
                                        static_cast<float_t>(surfaceHeight)),
            .position = rive::Vec2D(static_cast<float_t>(pointerX),
                                    static_cast<float_t>(pointerY)),
            .scaleFactor = static_cast<float>(layoutScale)};
        event.pointerId = static_cast<int>(pointerID);

        commandQueue->pointerMove(
            handleFromLong<rive::StateMachineHandle>(stateMachineHandle),
            event);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppPointerDown(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong stateMachineHandle,
        jbyte jFit,
        jbyte jAlignment,
        jfloat layoutScale,
        jfloat surfaceWidth,
        jfloat surfaceHeight,
        jint pointerID,
        jfloat pointerX,
        jfloat pointerY)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        rive::CommandQueue::PointerEvent event{
            .fit = GetFit(static_cast<uint8_t>(jFit)),
            .alignment = GetAlignment(static_cast<uint8_t>(jAlignment)),
            .screenBounds = rive::Vec2D(static_cast<float_t>(surfaceWidth),
                                        static_cast<float_t>(surfaceHeight)),
            .position = rive::Vec2D(static_cast<float_t>(pointerX),
                                    static_cast<float_t>(pointerY)),
            .scaleFactor = static_cast<float>(layoutScale)};
        event.pointerId = static_cast<int>(pointerID);

        commandQueue->pointerDown(
            handleFromLong<rive::StateMachineHandle>(stateMachineHandle),
            event);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppPointerUp(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong stateMachineHandle,
        jbyte jFit,
        jbyte jAlignment,
        jfloat layoutScale,
        jfloat surfaceWidth,
        jfloat surfaceHeight,
        jint pointerID,
        jfloat pointerX,
        jfloat pointerY)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        rive::CommandQueue::PointerEvent event{
            .fit = GetFit(static_cast<uint8_t>(jFit)),
            .alignment = GetAlignment(static_cast<uint8_t>(jAlignment)),
            .screenBounds = rive::Vec2D(static_cast<float_t>(surfaceWidth),
                                        static_cast<float_t>(surfaceHeight)),
            .position = rive::Vec2D(static_cast<float_t>(pointerX),
                                    static_cast<float_t>(pointerY)),
            .scaleFactor = static_cast<float>(layoutScale)};
        event.pointerId = static_cast<int>(pointerID);

        commandQueue->pointerUp(
            handleFromLong<rive::StateMachineHandle>(stateMachineHandle),
            event);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppPointerExit(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong stateMachineHandle,
        jbyte jFit,
        jbyte jAlignment,
        jfloat layoutScale,
        jfloat surfaceWidth,
        jfloat surfaceHeight,
        jint pointerID,
        jfloat pointerX,
        jfloat pointerY)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        rive::CommandQueue::PointerEvent event{
            .fit = GetFit(static_cast<uint8_t>(jFit)),
            .alignment = GetAlignment(static_cast<uint8_t>(jAlignment)),
            .screenBounds = rive::Vec2D(static_cast<float_t>(surfaceWidth),
                                        static_cast<float_t>(surfaceHeight)),
            .position = rive::Vec2D(static_cast<float_t>(pointerX),
                                    static_cast<float_t>(pointerY)),
            .scaleFactor = static_cast<float>(layoutScale)};
        event.pointerId = static_cast<int>(pointerID);

        commandQueue->pointerExit(
            handleFromLong<rive::StateMachineHandle>(stateMachineHandle),
            event);
    }

    // =========================================================================
    // Render Target and Draw Key
    // =========================================================================

    JNIEXPORT jlong JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppCreateRiveRenderTarget(
        JNIEnv*,
        jobject,
        jlong ref,
        jint width,
        jint height)
    {
        auto* commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);

        // Use a promise/future to make this synchronous
        auto promise = std::make_shared<std::promise<jlong>>();
        std::future<jlong> future = promise->get_future();

        // Use runOnce to execute on the command server thread where GL context
        // is active
        commandQueue->runOnce([width, height, promise](
                                  rive::CommandServer* server) {
            // Query sample count from the current GL context
            GLint actualSampleCount = 1;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glGetIntegerv(GL_SAMPLES, &actualSampleCount);
            RiveLogD(TAG_CQ,
                     "Creating render target on command server "
                     "(sample count: %d)",
                     actualSampleCount);

            auto renderTarget =
                new rive::gpu::FramebufferRenderTargetGL(width,
                                                         height,
                                                         0, // Framebuffer ID
                                                         actualSampleCount);
            promise->set_value(reinterpret_cast<jlong>(renderTarget));
        });

        // Wait for the result. Blocks the main thread until complete.
        return future.get();
    }

    JNIEXPORT jlong JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppCreateDrawKey(JNIEnv*,
                                                              jobject,
                                                              jlong ref)
    {
        auto commandQueue = reinterpret_cast<rive::CommandQueue*>(ref);
        auto drawKey = commandQueue->createDrawKey();
        return longFromHandle(drawKey);
    }

    // =========================================================================
    // Single Sprite Drawing
    // =========================================================================

    JNIEXPORT void JNICALL Java_app_rive_core_CommandQueueJNIBridge_cppDraw(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong renderContextRef,
        jlong surfaceRef,
        jlong drawKey,
        jlong artboardHandleRef,
        jlong stateMachineHandleRef,
        jlong renderTargetRef,
        jint width,
        jint height,
        jbyte jFit,
        jbyte jAlignment,
        jfloat jScaleFactor,
        jint jClearColor)
    {
        auto* commandQueue = reinterpret_cast<CommandQueueWithThread*>(ref);
        auto* renderContext =
            reinterpret_cast<RenderContext*>(renderContextRef);
        auto* nativeSurface = reinterpret_cast<void*>(surfaceRef);
        auto* renderTarget =
            reinterpret_cast<rive::gpu::RenderTargetGL*>(renderTargetRef);
        auto fit = GetFit(static_cast<uint8_t>(jFit));
        auto alignment = GetAlignment(static_cast<uint8_t>(jAlignment));
        auto scaleFactor = static_cast<float_t>(jScaleFactor);
        auto clearColor = static_cast<uint32_t>(jClearColor);

        auto drawWork = [commandQueue,
                         renderContext,
                         nativeSurface,
                         artboardHandleRef,
                         stateMachineHandleRef,
                         renderTarget,
                         width,
                         height,
                         fit,
                         alignment,
                         clearColor,
                         scaleFactor](rive::DrawKey drawKey,
                                      rive::CommandServer* server) {
            auto artboard = server->getArtboardInstance(
                handleFromLong<rive::ArtboardHandle>(artboardHandleRef));
            if (artboard == nullptr)
            {
                if (commandQueue->shouldLogArtboardNull(drawKey))
                {
                    RiveLogE(
                        TAG_CQ,
                        "Draw failed: Artboard instance is null (only reported once)");
                }
                return;
            }

            auto stateMachine = server->getStateMachineInstance(
                handleFromLong<rive::StateMachineHandle>(
                    stateMachineHandleRef));
            if (stateMachine == nullptr)
            {
                if (commandQueue->shouldLogStateMachineNull(drawKey))
                {
                    RiveLogE(
                        TAG_CQ,
                        "Draw failed: State machine instance is null (only reported once)");
                }
                return;
            }

            // Render backend specific - make the context current
            renderContext->beginFrame(nativeSurface);

            // Retrieve the Rive RenderContext from the CommandServer
            auto factory =
                reinterpret_cast<CommandServerFactory*>(server->factory());
            auto riveContext = factory->getRenderContext()->riveContext.get();

            riveContext->beginFrame(rive::gpu::RenderContext::FrameDescriptor{
                .renderTargetWidth = static_cast<uint32_t>(width),
                .renderTargetHeight = static_cast<uint32_t>(height),
                .loadAction = rive::gpu::LoadAction::clear,
                .clearColor = clearColor,
            });

            // Stack allocate a Rive Renderer
            auto renderer = rive::RiveRenderer(riveContext);

            // Draw the .riv
            renderer.align(fit,
                           alignment,
                           rive::AABB(0.0f,
                                      0.0f,
                                      static_cast<float_t>(width),
                                      static_cast<float_t>(height)),
                           artboard->bounds(),
                           scaleFactor);
            artboard->draw(&renderer);

            // Flush the draw commands
            riveContext->flush({
                .renderTarget = renderTarget,
            });

            // Render context specific - swap buffers
            renderContext->present(nativeSurface);
        };
        commandQueue->draw(handleFromLong<rive::DrawKey>(drawKey), drawWork);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppDrawToBuffer(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong renderContextRef,
        jlong surfaceRef,
        jlong drawKey,
        jlong artboardHandleRef,
        jlong stateMachineHandleRef,
        jlong renderTargetRef,
        jint jWidth,
        jint jHeight,
        jbyte jFit,
        jbyte jAlignment,
        jfloat jScaleFactor,
        jint jClearColor,
        jbyteArray jBuffer)
    {
        auto* commandQueue = reinterpret_cast<CommandQueueWithThread*>(ref);
        auto* renderContext =
            reinterpret_cast<RenderContext*>(renderContextRef);
        auto* nativeSurface = reinterpret_cast<void*>(surfaceRef);
        auto* renderTarget =
            reinterpret_cast<rive::gpu::RenderTargetGL*>(renderTargetRef);
        auto fit = GetFit(static_cast<uint8_t>(jFit));
        auto alignment = GetAlignment(static_cast<uint8_t>(jAlignment));
        auto scaleFactor = static_cast<float_t>(jScaleFactor);
        auto clearColor = static_cast<uint32_t>(jClearColor);
        auto width = static_cast<int>(jWidth);
        auto height = static_cast<int>(jHeight);
        auto* pixels = reinterpret_cast<uint8_t*>(
            env->GetByteArrayElements(jBuffer, nullptr));
        auto jExceptionClass =
            FindClass(env, "app/rive/RiveDrawToBufferException");
        if (pixels == nullptr)
        {
            RiveLogE(TAG_CQ,
                     "Failed to access pixel buffer for drawIntoBuffer");
            env->ThrowNew(jExceptionClass.get(),
                          "Failed to access pixel buffer for drawing");
            return;
        }

        // Success case plus any potential errors producing the drawn buffer
        enum class DrawResult
        {
            Success,
            ArtboardNull,
            StateMachineNull,
        };

        // Be sure all pathways signal completion with `set_value` before
        // returning to avoid deadlock.
        auto completionPromise = std::make_shared<std::promise<DrawResult>>();
        auto drawWork = [commandQueue,
                         renderContext,
                         nativeSurface,
                         artboardHandleRef,
                         stateMachineHandleRef,
                         renderTarget,
                         width,
                         height,
                         fit,
                         alignment,
                         scaleFactor,
                         clearColor,
                         pixels,
                         completionPromise](rive::DrawKey drawKey,
                                            rive::CommandServer* server) {
            auto artboard = server->getArtboardInstance(
                handleFromLong<rive::ArtboardHandle>(artboardHandleRef));
            if (artboard == nullptr)
            {
                if (commandQueue->shouldLogArtboardNull(drawKey))
                {
                    RiveLogE(
                        TAG_CQ,
                        "Draw failed: Artboard instance is null (only reported once)");
                }
                completionPromise->set_value(DrawResult::ArtboardNull);
                return;
            }

            auto stateMachine = server->getStateMachineInstance(
                handleFromLong<rive::StateMachineHandle>(
                    stateMachineHandleRef));
            if (stateMachine == nullptr)
            {
                if (commandQueue->shouldLogStateMachineNull(drawKey))
                {
                    RiveLogE(
                        TAG_CQ,
                        "Draw failed: State machine instance is null (only reported once)");
                }
                completionPromise->set_value(DrawResult::StateMachineNull);
                return;
            }

            renderContext->beginFrame(nativeSurface);

            // Retrieve the Rive RenderContext from the CommandServer
            auto factory =
                reinterpret_cast<CommandServerFactory*>(server->factory());
            auto riveContext = factory->getRenderContext()->riveContext.get();

            riveContext->beginFrame(rive::gpu::RenderContext::FrameDescriptor{
                .renderTargetWidth = static_cast<uint32_t>(width),
                .renderTargetHeight = static_cast<uint32_t>(height),
                .loadAction = rive::gpu::LoadAction::clear,
                .clearColor = clearColor,
            });

            auto renderer = rive::RiveRenderer(riveContext);

            renderer.align(fit,
                           alignment,
                           rive::AABB(0.0f,
                                      0.0f,
                                      static_cast<float_t>(width),
                                      static_cast<float_t>(height)),
                           artboard->bounds(),
                           scaleFactor);
            artboard->draw(&renderer);

            riveContext->flush({
                .renderTarget = renderTarget,
            });

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glFinish();
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glReadPixels(0,
                         0,
                         width,
                         height,
                         GL_RGBA,
                         GL_UNSIGNED_BYTE,
                         pixels);

            auto rowBytes = static_cast<size_t>(width) * 4;
            std::vector<uint8_t> row(rowBytes);
            auto* data = pixels;
            for (int y = 0; y < height / 2; ++y)
            {
                auto* top = data + (static_cast<size_t>(y) * rowBytes);
                auto* bottom =
                    data + (static_cast<size_t>(height - 1 - y) * rowBytes);
                std::memcpy(row.data(), top, rowBytes);
                std::memcpy(top, bottom, rowBytes);
                std::memcpy(bottom, row.data(), rowBytes);
            }

            renderContext->present(nativeSurface);
            completionPromise->set_value(DrawResult::Success);
        };

        commandQueue->draw(handleFromLong<rive::DrawKey>(drawKey), drawWork);
        auto result = completionPromise->get_future().get();
        env->ReleaseByteArrayElements(jBuffer,
                                      reinterpret_cast<jbyte*>(pixels),
                                      0);

        switch (result)
        {
            case DrawResult::Success:
                break;
            case DrawResult::ArtboardNull:
                env->ThrowNew(
                    jExceptionClass.get(),
                    "Failed to draw into buffer: Artboard instance is null");
                break;
            case DrawResult::StateMachineNull:
                env->ThrowNew(
                    jExceptionClass.get(),
                    "Failed to draw into buffer: State machine instance is null");
                break;
        }
    }

    // =========================================================================
    // Multi-Sprite Batch Rendering
    // =========================================================================

    /**
     * Draw multiple sprites to a buffer (synchronous).
     *
     * This method renders all sprites to the GPU surface and then reads the pixels
     * back into the provided buffer. It blocks until the rendering is complete.
     *
     * The buffer must be pre-allocated with size: width * height * 4 bytes (RGBA).
     * The pixel data is flipped vertically to match Android's coordinate system.
     *
     * @param ref CommandQueue pointer
     * @param renderContextRef RenderContext pointer
     * @param surfaceRef Surface pointer
     * @param drawKey Draw key handle
     * @param renderTargetRef Render target pointer
     * @param viewportWidth Viewport width in pixels
     * @param viewportHeight Viewport height in pixels
     * @param jClearColor Clear color (AARRGGBB)
     * @param jArtboardHandles Array of artboard handles
     * @param jStateMachineHandles Array of state machine handles
     * @param jTransforms Flattened transforms (6 floats per sprite)
     * @param jArtboardWidths Target widths
     * @param jArtboardHeights Target heights
     * @param count Number of sprites
     * @param jBuffer Output buffer for pixel data (RGBA, 4 bytes per pixel)
     */
    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppDrawMultipleToBuffer(
        JNIEnv* env,
        jobject,
        jlong ref,
        jlong renderContextRef,
        jlong surfaceRef,
        jlong drawKey,
        jlong renderTargetRef,
        jint viewportWidth,
        jint viewportHeight,
        jint jClearColor,
        jlongArray jArtboardHandles,
        jlongArray jStateMachineHandles,
        jfloatArray jTransforms,
        jfloatArray jArtboardWidths,
        jfloatArray jArtboardHeights,
        jint count,
        jbyteArray jBuffer)
    {
        auto* commandQueue = reinterpret_cast<CommandQueueWithThread*>(ref);
        auto* renderContext =
            reinterpret_cast<RenderContext*>(renderContextRef);
        auto* nativeSurface = reinterpret_cast<void*>(surfaceRef);
        auto* renderTarget =
            reinterpret_cast<rive::gpu::RenderTargetGL*>(renderTargetRef);
        auto clearColor = static_cast<uint32_t>(jClearColor);
        auto widthInt = static_cast<int>(viewportWidth);
        auto heightInt = static_cast<int>(viewportHeight);
        auto spriteCount = static_cast<int>(count);

        if (spriteCount <= 0)
        {
            return;
        }

        auto* pixels = reinterpret_cast<uint8_t*>(
            env->GetByteArrayElements(jBuffer, nullptr));
        if (pixels == nullptr)
        {
            RiveLogE(TAG_CQ,
                     "Failed to access pixel buffer for drawMultipleToBuffer");
            return;
        }

        // Extract Java arrays into C++ vectors for use in the draw lambda
        auto artboardHandles = std::vector<jlong>(spriteCount);
        auto stateMachineHandles = std::vector<jlong>(spriteCount);
        auto transforms = std::vector<float>(spriteCount * 6);
        auto artboardWidths = std::vector<float>(spriteCount);
        auto artboardHeights = std::vector<float>(spriteCount);

        env->GetLongArrayRegion(jArtboardHandles,
                                0,
                                spriteCount,
                                artboardHandles.data());
        env->GetLongArrayRegion(jStateMachineHandles,
                                0,
                                spriteCount,
                                stateMachineHandles.data());
        env->GetFloatArrayRegion(jTransforms,
                                 0,
                                 spriteCount * 6,
                                 transforms.data());
        env->GetFloatArrayRegion(jArtboardWidths,
                                 0,
                                 spriteCount,
                                 artboardWidths.data());
        env->GetFloatArrayRegion(jArtboardHeights,
                                 0,
                                 spriteCount,
                                 artboardHeights.data());

        // Use a promise to make this synchronous - we need to wait for pixels
        auto completionPromise = std::make_shared<std::promise<bool>>();

        auto drawWork = [commandQueue,
                         renderContext,
                         nativeSurface,
                         renderTarget,
                         widthInt,
                         heightInt,
                         clearColor,
                         spriteCount,
                         pixels,
                         completionPromise,
                         artboardHandles = std::move(artboardHandles),
                         stateMachineHandles = std::move(stateMachineHandles),
                         transforms = std::move(transforms),
                         artboardWidths = std::move(artboardWidths),
                         artboardHeights = std::move(artboardHeights)](
                            rive::DrawKey drawKey,
                            rive::CommandServer* server) {
            // Render backend specific - make the context current
            renderContext->beginFrame(nativeSurface);

            // Retrieve the Rive RenderContext from the CommandServer
            auto factory =
                reinterpret_cast<CommandServerFactory*>(server->factory());
            auto riveContext = factory->getRenderContext()->riveContext.get();

            riveContext->beginFrame(rive::gpu::RenderContext::FrameDescriptor{
                .renderTargetWidth = static_cast<uint32_t>(widthInt),
                .renderTargetHeight = static_cast<uint32_t>(heightInt),
                .loadAction = rive::gpu::LoadAction::clear,
                .clearColor = clearColor,
            });

            // Stack allocate a Rive Renderer
            auto renderer = rive::RiveRenderer(riveContext);

            // Draw each sprite with its transform
            for (int i = 0; i < spriteCount; ++i)
            {
                auto artboard = server->getArtboardInstance(
                    handleFromLong<rive::ArtboardHandle>(artboardHandles[i]));
                if (artboard == nullptr)
                {
                    RiveLogW(TAG_CQ,
                             "DrawMultipleToBuffer: Artboard %d is null, "
                             "skipping",
                             i);
                    continue;
                }

                // Extract the 6-element transform for this sprite
                int transformOffset = i * 6;
                rive::Mat2D spriteTransform(
                    transforms[transformOffset + 0],  // xx (scaleX)
                    transforms[transformOffset + 1],  // xy (skewY)
                    transforms[transformOffset + 2],  // yx (skewX)
                    transforms[transformOffset + 3],  // yy (scaleY)
                    transforms[transformOffset + 4],  // tx (translateX)
                    transforms[transformOffset + 5]   // ty (translateY)
                );

                // Calculate scale to fit artboard into the requested sprite
                // size
                float artboardWidth = artboard->width();
                float artboardHeight = artboard->height();
                float targetWidth = artboardWidths[i];
                float targetHeight = artboardHeights[i];

                float scaleX = targetWidth / artboardWidth;
                float scaleY = targetHeight / artboardHeight;

                rive::Mat2D scaleToFit = rive::Mat2D::fromScale(scaleX, scaleY);
                rive::Mat2D finalTransform = spriteTransform * scaleToFit;

                renderer.save();
                renderer.transform(finalTransform);
                artboard->draw(&renderer);
                renderer.restore();
            }

            // Flush the draw commands
            riveContext->flush({
                .renderTarget = renderTarget,
            });

            // Read pixels from the framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glFinish();
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            
            // Read pixels as RGBA (OpenGL default format)
            // Note: Bitmap.copyPixelsFromBuffer() expects RGBA format, NOT BGRA!
            // So we do NOT convert the colors - just flip vertically.
            glReadPixels(0, 0, widthInt, heightInt, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            
            // Flip image vertically to convert from OpenGL coordinates (origin at bottom-left)
            // to Android coordinates (origin at top-left)
            flipImageVertically(pixels, widthInt, heightInt);

            // Present and signal completion
            renderContext->present(nativeSurface);
            completionPromise->set_value(true);
        };

        commandQueue->draw(handleFromLong<rive::DrawKey>(drawKey), drawWork);

        // Wait for completion - this makes the call synchronous
        completionPromise->get_future().wait();

        // Release the buffer back to Java
        env->ReleaseByteArrayElements(jBuffer,
                                      reinterpret_cast<jbyte*>(pixels),
                                      0);
    }

    JNIEXPORT void JNICALL
    Java_app_rive_core_CommandQueueJNIBridge_cppDrawMultiple(JNIEnv* env,
                                                    jobject,
                                                    jlong ref,
                                                    jlong renderContextRef,
                                                    jlong surfaceRef,
                                                    jlong drawKey,
                                                    jlong renderTargetRef,
                                                    jint viewportWidth,
                                                    jint viewportHeight,
                                                    jint jClearColor,
                                                    jlongArray jArtboardHandles,
                                                    jlongArray jStateMachineHandles,
                                                    jfloatArray jTransforms,
                                                    jfloatArray jArtboardWidths,
                                                    jfloatArray jArtboardHeights,
                                                    jint count)
    {
        auto* commandQueue = reinterpret_cast<CommandQueueWithThread*>(ref);
        auto* renderContext =
            reinterpret_cast<RenderContext*>(renderContextRef);
        auto* nativeSurface = reinterpret_cast<void*>(surfaceRef);
        auto* renderTarget =
            reinterpret_cast<rive::gpu::RenderTargetGL*>(renderTargetRef);
        auto clearColor = static_cast<uint32_t>(jClearColor);
        auto widthInt = static_cast<int>(viewportWidth);
        auto heightInt = static_cast<int>(viewportHeight);
        auto spriteCount = static_cast<int>(count);

        if (spriteCount <= 0)
        {
            return;
        }

        // Extract Java arrays into C++ vectors for use in the draw lambda
        // We need to copy these because the lambda will be executed asynchronously
        auto artboardHandles = std::vector<jlong>(spriteCount);
        auto stateMachineHandles = std::vector<jlong>(spriteCount);
        auto transforms = std::vector<float>(spriteCount * 6);
        auto artboardWidths = std::vector<float>(spriteCount);
        auto artboardHeights = std::vector<float>(spriteCount);

        env->GetLongArrayRegion(jArtboardHandles,
                                0,
                                spriteCount,
                                artboardHandles.data());
        env->GetLongArrayRegion(jStateMachineHandles,
                                0,
                                spriteCount,
                                stateMachineHandles.data());
        env->GetFloatArrayRegion(jTransforms,
                                 0,
                                 spriteCount * 6,
                                 transforms.data());
        env->GetFloatArrayRegion(jArtboardWidths,
                                 0,
                                 spriteCount,
                                 artboardWidths.data());
        env->GetFloatArrayRegion(jArtboardHeights,
                                 0,
                                 spriteCount,
                                 artboardHeights.data());

        auto drawWork = [commandQueue,
                         renderContext,
                         nativeSurface,
                         renderTarget,
                         widthInt,
                         heightInt,
                         clearColor,
                         spriteCount,
                         artboardHandles = std::move(artboardHandles),
                         stateMachineHandles = std::move(stateMachineHandles),
                         transforms = std::move(transforms),
                         artboardWidths = std::move(artboardWidths),
                         artboardHeights = std::move(artboardHeights)](
                            rive::DrawKey drawKey,
                            rive::CommandServer* server) {
            // Render backend specific - make the context current
            renderContext->beginFrame(nativeSurface);

            // Retrieve the Rive RenderContext from the CommandServer
            auto factory =
                reinterpret_cast<CommandServerFactory*>(server->factory());
            auto riveContext = factory->getRenderContext()->riveContext.get();

            riveContext->beginFrame(rive::gpu::RenderContext::FrameDescriptor{
                .renderTargetWidth = static_cast<uint32_t>(widthInt),
                .renderTargetHeight = static_cast<uint32_t>(heightInt),
                .loadAction = rive::gpu::LoadAction::clear,
                .clearColor = clearColor,
            });

            // Stack allocate a Rive Renderer
            auto renderer = rive::RiveRenderer(riveContext);

            // Draw each sprite with its transform
            for (int i = 0; i < spriteCount; ++i)
            {
                auto artboard = server->getArtboardInstance(
                    handleFromLong<rive::ArtboardHandle>(artboardHandles[i]));
                if (artboard == nullptr)
                {
                    RiveLogW(TAG_CQ,
                             "DrawMultiple: Artboard %d is null, skipping",
                             i);
                    continue;
                }

                // Extract the 6-element transform for this sprite
                // Format: [xx, xy, yx, yy, tx, ty] = [scaleX, skewY, skewX, scaleY, translateX, translateY]
                int transformOffset = i * 6;
                rive::Mat2D spriteTransform(
                    transforms[transformOffset + 0],  // xx (scaleX)
                    transforms[transformOffset + 1],  // xy (skewY)
                    transforms[transformOffset + 2],  // yx (skewX)
                    transforms[transformOffset + 3],  // yy (scaleY)
                    transforms[transformOffset + 4],  // tx (translateX)
                    transforms[transformOffset + 5]   // ty (translateY)
                );

                // Calculate scale to fit artboard into the requested sprite size
                float artboardWidth = artboard->width();
                float artboardHeight = artboard->height();
                float targetWidth = artboardWidths[i];
                float targetHeight = artboardHeights[i];

                // Compute scale factors to fit artboard into target size
                float scaleX = targetWidth / artboardWidth;
                float scaleY = targetHeight / artboardHeight;

                // Create a scaling transform to fit the artboard to sprite size
                rive::Mat2D scaleToFit = rive::Mat2D::fromScale(scaleX, scaleY);

                // Combine: first scale the artboard to fit, then apply sprite transform
                rive::Mat2D finalTransform = spriteTransform * scaleToFit;

                // Save renderer state, apply transform, draw, restore
                renderer.save();
                renderer.transform(finalTransform);
                artboard->draw(&renderer);
                renderer.restore();
            }

            // Flush the draw commands
            riveContext->flush({
                .renderTarget = renderTarget,
            });

            // Render context specific - swap buffers
            renderContext->present(nativeSurface);
        };

        commandQueue->draw(handleFromLong<rive::DrawKey>(drawKey), drawWork);
    }
}