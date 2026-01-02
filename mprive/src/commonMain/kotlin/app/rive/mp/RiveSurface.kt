package app.rive.mp

import app.rive.mp.core.CheckableAutoCloseable
import app.rive.mp.core.UniquePointer

/**
 * A backend agnostic collection of surface properties needed for rendering.
 * - A Rive render target, created natively which renders to the GL framebuffer
 * - A draw key, which uniquely identifies draw operations in the CommandQueue
 *
 * It also stores the width and height of the surface.
 *
 * This class assumes ownership of all resources and should be [closed][RiveSurface.close] when no
 * longer needed.
 *
 * @param renderTargetPointer The native pointer to the Rive render target.
 * @param drawKey The key used to uniquely identify the draw operation in the CommandQueue.
 * @param width The width of the surface in pixels.
 * @param height The height of the surface in pixels.
 */
abstract class RiveSurface(
    renderTargetPointer: Long,
    val drawKey: DrawKey,
    val width: Int,
    val height: Int
) : CheckableAutoCloseable {
    private external fun cppDeleteRenderTarget(pointer: Long)

    /**
     * Closes the render target unique pointer, which in turn disposes the RiveSurface at large.
     *
     * ⚠️ Do not call this directly from the main thread. It is meant to be called on the command
     * server thread as a scheduled close using [CommandQueue.destroyRiveSurface]. This ensures that
     * any draw calls in flight have a valid surface until completed.
     */
    override fun close() = renderTargetPointer.close()
    override val closed: Boolean
        get() = renderTargetPointer.closed

    /**
     * Deletes the native render target.
     *
     * Sub-classes should override this method to dispose of any additional resources, calling
     * `super.dispose(pointer)` at the end.
     *
     * Called from the [render target's unique pointer][renderTargetPointer]. Runs on the command
     * server thread.
     *
     * @param renderTargetPointer The native pointer to the Rive render target.
     */
    protected open fun dispose(renderTargetPointer: Long) {
        RiveLog.d("Rive/RenderTarget") { "Deleting Rive render target" }
        cppDeleteRenderTarget(renderTargetPointer)
    }

    /** The native pointer to the Rive render target, held in a unique pointer. */
    val renderTargetPointer: UniquePointer =
        UniquePointer(renderTargetPointer, "Rive/RenderTarget", ::dispose)

    /** The native pointer to the backend-specific surface, e.g. EGLSurface for OpenGL. */
    abstract val surfaceNativePointer: Long
}
