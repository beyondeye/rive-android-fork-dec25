package app.rive.mp

import app.rive.mp.core.CheckableAutoCloseable

/**
 * A backend agnostic rendering context base class. Implementers contain the necessary state for
 * Rive to render, both in Kotlin and with associated native objects.
 *
 * As it contains native resources, it implements [CheckableAutoCloseable] and should be
 * [closed][CheckableAutoCloseable.close] when no longer needed.
 */
abstract class RenderContext : CheckableAutoCloseable {
    /**
     * The native pointer to the backend-specific RenderContext object.
     */
    abstract val nativeObjectPointer: Long

    /**
     * Creates a backend-specific [RiveSurface].
     *
     * @param drawKey The key used to uniquely identify the draw operation in the CommandQueue.
     * @param commandQueue The owning command queue, used to create render targets on the command
     *    server thread.
     * @return The created [RiveSurface].
     */
    abstract fun createSurface(
        drawKey: DrawKey,
        commandQueue: CommandQueue
    ): RiveSurface

    /**
     * Creates an off-screen [RiveSurface] that renders into a pixel buffer.
     *
     * @param width The width of the surface in pixels.
     * @param height The height of the surface in pixels.
     * @param drawKey The key used to uniquely identify the draw operation in the CommandQueue.
     * @param commandQueue The owning command queue, used to create render targets on the command
     *    server thread.
     * @return The created [RiveSurface].
     */
    abstract fun createImageSurface(
        width: Int,
        height: Int,
        drawKey: DrawKey,
        commandQueue: CommandQueue
    ): RiveSurface
}

/**
 * Platform-specific expect function to create the default RenderContext for the platform.
 * On Android: Creates an OpenGL ES RenderContext
 * On Desktop: Creates an OpenGL RenderContext
 */
expect fun createDefaultRenderContext(): RenderContext
