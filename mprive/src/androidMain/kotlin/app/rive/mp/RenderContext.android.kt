package app.rive.mp

import android.graphics.SurfaceTexture
import android.opengl.EGL14
import android.opengl.EGLConfig
import android.opengl.EGLContext
import android.opengl.EGLDisplay
import android.opengl.EGLSurface
import android.view.Surface
import app.rive.mp.core.UniquePointer

/**
 * EGL error code to string mapping for debugging.
 */
internal object EGLError {
    private val errorMessages = mapOf(
        EGL14.EGL_SUCCESS to "EGL_SUCCESS",
        EGL14.EGL_NOT_INITIALIZED to "EGL_NOT_INITIALIZED",
        EGL14.EGL_BAD_ACCESS to "EGL_BAD_ACCESS",
        EGL14.EGL_BAD_ALLOC to "EGL_BAD_ALLOC",
        EGL14.EGL_BAD_ATTRIBUTE to "EGL_BAD_ATTRIBUTE",
        EGL14.EGL_BAD_CONTEXT to "EGL_BAD_CONTEXT",
        EGL14.EGL_BAD_CONFIG to "EGL_BAD_CONFIG",
        EGL14.EGL_BAD_CURRENT_SURFACE to "EGL_BAD_CURRENT_SURFACE",
        EGL14.EGL_BAD_DISPLAY to "EGL_BAD_DISPLAY",
        EGL14.EGL_BAD_SURFACE to "EGL_BAD_SURFACE",
        EGL14.EGL_BAD_MATCH to "EGL_BAD_MATCH",
        EGL14.EGL_BAD_PARAMETER to "EGL_BAD_PARAMETER",
        EGL14.EGL_BAD_NATIVE_PIXMAP to "EGL_BAD_NATIVE_PIXMAP",
        EGL14.EGL_BAD_NATIVE_WINDOW to "EGL_BAD_NATIVE_WINDOW",
        EGL14.EGL_CONTEXT_LOST to "EGL_CONTEXT_LOST"
    )

    fun errorString(errorCode: Int): String {
        return errorMessages[errorCode] ?: "Unknown EGL error (0x${errorCode.toString(16)})"
    }
}

/**
 * OpenGL ES rendering context implementation for Android.
 *
 * Creates and manages an EGL display, config, and context for rendering with OpenGL ES 2.0.
 * The C++ RenderContextGL is created with the EGL display and context handles.
 *
 * As it contains native resources, it implements [CheckableAutoCloseable] and should be
 * closed when no longer needed.
 *
 * @param display The EGL display.
 * @param config The EGL config.
 * @param context The EGL context.
 * @throws RuntimeException If unable to create or initialize any EGL resources.
 */
internal class RenderContextGL(
    val display: EGLDisplay = createDisplay(),
    val config: EGLConfig = createConfig(display),
    val context: EGLContext = createContext(display, config)
) : RenderContext() {
    
    private external fun cppConstructor(displayHandle: Long, contextHandle: Long): Long
    private external fun cppDelete(pointer: Long)

    companion object {
        const val TAG = "Rive/MP/RenderContextGL"

        /**
         * Gets and initializes the EGL display.
         *
         * @throws RuntimeException If unable to get or initialize the EGL display.
         */
        private fun createDisplay(): EGLDisplay {
            RiveLog.d(TAG) { "Getting EGL display" }
            val display = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY)
            if (display == EGL14.EGL_NO_DISPLAY) {
                val error = EGLError.errorString(EGL14.eglGetError())
                RiveLog.e(TAG) { "eglGetDisplay failed with error: $error" }
                throw RuntimeException("Unable to get EGL display: $error")
            }

            RiveLog.d(TAG) { "Initializing EGL" }
            val majorVersion = IntArray(1)
            val minorVersion = IntArray(1)
            if (!EGL14.eglInitialize(display, majorVersion, 0, minorVersion, 0)) {
                val error = EGLError.errorString(EGL14.eglGetError())
                RiveLog.e(TAG) { "eglInitialize failed with error: $error" }
                throw RuntimeException("Unable to initialize EGL: $error")
            }
            RiveLog.d(TAG) { "EGL initialized with version ${majorVersion[0]}.${minorVersion[0]}" }

            return display
        }

        /**
         * Chooses an EGL config for OpenGL ES 2.0 windowed rendering with 8 bits per channel RGBA
         * and 8 bits for the stencil buffer.
         *
         * @param display The EGL display.
         * @throws RuntimeException If unable to find a suitable EGL config.
         */
        private fun createConfig(display: EGLDisplay): EGLConfig {
            val configAttributes = intArrayOf(
                // We want OpenGL ES 2.0
                EGL14.EGL_RENDERABLE_TYPE, EGL14.EGL_OPENGL_ES2_BIT,
                // Request both window and pbuffer surfaces
                EGL14.EGL_SURFACE_TYPE, EGL14.EGL_WINDOW_BIT or EGL14.EGL_PBUFFER_BIT,
                // 8 bits per channel RGBA
                EGL14.EGL_RED_SIZE, 8,
                EGL14.EGL_GREEN_SIZE, 8,
                EGL14.EGL_BLUE_SIZE, 8,
                EGL14.EGL_ALPHA_SIZE, 8,
                // No depth buffer
                EGL14.EGL_DEPTH_SIZE, 0,
                // 8 bits for the stencil buffer
                EGL14.EGL_STENCIL_SIZE, 8,
                EGL14.EGL_NONE
            )

            RiveLog.d(TAG) { "Choosing EGL config" }
            val numConfigs = IntArray(1)
            val configs = arrayOfNulls<EGLConfig>(1)
            val success = EGL14.eglChooseConfig(
                display,
                configAttributes,
                0,
                configs,
                0,
                configs.size,
                numConfigs,
                0
            )
            if (!success) {
                val error = EGLError.errorString(EGL14.eglGetError())
                RiveLog.e(TAG) { "eglChooseConfig failed with error: $error" }
                throw RuntimeException("EGL config creation failed: $error")
            } else if (numConfigs[0] <= 0 || configs[0] == null) {
                RiveLog.e(TAG) { "eglChooseConfig could not find a suitable config" }
                throw RuntimeException("Unable to find a suitable EGL config")
            } else {
                val chosenConfig = configs[0]!!
                fun attr(name: Int): Int {
                    val value = IntArray(1)
                    EGL14.eglGetConfigAttrib(display, chosenConfig, name, value, 0)
                    return value[0]
                }

                RiveLog.d(TAG) {
                    "EGL config chosen successfully:\n" +
                            "  R=${attr(EGL14.EGL_RED_SIZE)}\n" +
                            "  G=${attr(EGL14.EGL_GREEN_SIZE)}\n" +
                            "  B=${attr(EGL14.EGL_BLUE_SIZE)}\n" +
                            "  A=${attr(EGL14.EGL_ALPHA_SIZE)}\n" +
                            "  Depth=${attr(EGL14.EGL_DEPTH_SIZE)}\n" +
                            "  Stencil=${attr(EGL14.EGL_STENCIL_SIZE)}"
                }

                return chosenConfig
            }
        }

        /**
         * Creates an EGL context for OpenGL ES 2.0 rendering.
         *
         * @param display The EGL display.
         * @param config The EGL config.
         * @throws RuntimeException If unable to create the EGL context.
         */
        private fun createContext(display: EGLDisplay, config: EGLConfig): EGLContext {
            val contextAttributes = intArrayOf(
                EGL14.EGL_CONTEXT_CLIENT_VERSION, 2,
                EGL14.EGL_NONE
            )
            RiveLog.d(TAG) { "Creating EGL context" }
            val context = EGL14.eglCreateContext(
                display,
                config,
                EGL14.EGL_NO_CONTEXT,
                contextAttributes,
                0
            )
            if (context == EGL14.EGL_NO_CONTEXT) {
                val error = EGLError.errorString(EGL14.eglGetError())
                RiveLog.e(TAG) { "eglCreateContext failed with error: $error" }
                throw RuntimeException("Unable to create EGL context: $error")
            }

            return context
        }
    }

    /** The native pointer to the C++ RenderContextGL, held in a unique pointer. */
    private val cppPointer = UniquePointer(
        cppConstructor(display.nativeHandle, context.nativeHandle),
        TAG,
        ::dispose
    )

    override val nativeObjectPointer: Long
        get() = cppPointer.pointer

    override var closed: Boolean = false
        private set

    /**
     * Disposes of the EGL context and display, and deletes the native RenderContextGL object.
     */
    private fun dispose(address: Long) {
        RiveLog.d(TAG) { "Destroying EGL context" }
        val destroyed = EGL14.eglDestroyContext(display, context)
        if (!destroyed) {
            val error = EGLError.errorString(EGL14.eglGetError())
            RiveLog.e(TAG) { "eglDestroyContext failed with error: $error" }
        }

        RiveLog.d(TAG) { "Terminating EGL display" }
        val terminated = EGL14.eglTerminate(display)
        if (!terminated) {
            val error = EGLError.errorString(EGL14.eglGetError())
            RiveLog.e(TAG) { "eglTerminate failed with error: $error" }
        }

        RiveLog.d(TAG) { "Deleting RenderContextGL native object" }
        cppDelete(address)
    }

    override fun close() {
        if (!closed) {
            cppPointer.close()
            closed = true
        }
    }

    /**
     * Creates an EGL surface from the given SurfaceTexture for rendering.
     *
     * @param drawKey The key used to uniquely identify the draw operation.
     * @param commandQueue The owning command queue, used to create render targets.
     * @return The created [RiveSurface].
     */
    override fun createSurface(
        drawKey: DrawKey,
        commandQueue: CommandQueue
    ): RiveSurface {
        // This requires a SurfaceTexture from the Android UI layer
        // For now, throw an exception - the caller should use the SurfaceTexture overload
        throw UnsupportedOperationException(
            "Use createSurface(SurfaceTexture, DrawKey, CommandQueue) on Android"
        )
    }

    /**
     * Creates an EGL surface from the given Android SurfaceTexture.
     *
     * @param surfaceTexture The Android SurfaceTexture to render against.
     * @param drawKey The key used to uniquely identify the draw operation.
     * @param commandQueue The owning command queue.
     * @return The created [RiveEGLSurface].
     */
    fun createSurface(
        surfaceTexture: SurfaceTexture,
        drawKey: DrawKey,
        commandQueue: CommandQueue
    ): RiveEGLSurface {
        RiveLog.d(TAG) { "Creating Android Surface" }
        val surface = Surface(surfaceTexture)
        if (!surface.isValid) {
            throw RuntimeException("Unable to create Android Surface from SurfaceTexture")
        }

        RiveLog.d(TAG) { "Creating EGL surface" }
        val eglSurface = EGL14.eglCreateWindowSurface(
            display,
            config,
            surface,
            intArrayOf(EGL14.EGL_NONE),
            0
        )
        if (eglSurface == EGL14.EGL_NO_SURFACE) {
            val error = EGLError.errorString(EGL14.eglGetError())
            RiveLog.e(TAG) { "eglCreateWindowSurface failed with error: $error" }
            throw RuntimeException("Unable to create EGL surface: $error")
        }

        // The EGLSurface holds a reference to the underlying ANativeWindow
        surface.release()

        val dimensions = IntArray(2)
        EGL14.eglQuerySurface(display, eglSurface, EGL14.EGL_WIDTH, dimensions, 0)
        EGL14.eglQuerySurface(display, eglSurface, EGL14.EGL_HEIGHT, dimensions, 1)
        val width = dimensions[0]
        val height = dimensions[1]
        RiveLog.d(TAG) { "Created EGL surface ($width x $height)" }

        // TODO: Create render target via CommandQueue when C.2.3 is implemented
        val renderTargetPointer = 0L // Placeholder

        return RiveEGLSurface(
            surfaceTexture,
            eglSurface,
            display,
            renderTargetPointer,
            drawKey,
            width,
            height
        )
    }

    /**
     * Creates an off-screen EGL PBuffer surface for image capture.
     *
     * @param width The width of the surface in pixels.
     * @param height The height of the surface in pixels.
     * @param drawKey The key used to uniquely identify the draw operation.
     * @param commandQueue The owning command queue.
     * @return The created [RiveEGLPBufferSurface].
     */
    override fun createImageSurface(
        width: Int,
        height: Int,
        drawKey: DrawKey,
        commandQueue: CommandQueue
    ): RiveSurface {
        require(width > 0 && height > 0) { "Image surfaces require a positive width and height." }
        RiveLog.d(TAG) { "Creating EGL PBuffer surface ($width x $height)" }
        val attrs = intArrayOf(
            EGL14.EGL_WIDTH, width,
            EGL14.EGL_HEIGHT, height,
            EGL14.EGL_NONE
        )
        val eglSurface = EGL14.eglCreatePbufferSurface(display, config, attrs, 0)
        if (eglSurface == EGL14.EGL_NO_SURFACE) {
            val error = EGLError.errorString(EGL14.eglGetError())
            RiveLog.e(TAG) { "eglCreatePbufferSurface failed with error: $error" }
            throw RuntimeException("Unable to create EGL PBuffer surface: $error")
        }

        // TODO: Create render target via CommandQueue when C.2.3 is implemented
        val renderTargetPointer = 0L // Placeholder

        return RiveEGLPBufferSurface(
            eglSurface,
            display,
            renderTargetPointer,
            drawKey,
            width,
            height
        )
    }
}

/**
 * Android EGL surface for rendering to a TextureView.
 */
class RiveEGLSurface(
    private val surfaceTexture: SurfaceTexture,
    private val eglSurface: EGLSurface,
    private val display: EGLDisplay,
    renderTargetPointer: Long,
    drawKey: DrawKey,
    width: Int,
    height: Int
) : RiveSurface(renderTargetPointer, drawKey, width, height) {
    
    companion object {
        const val TAG = "Rive/MP/EGLSurface"
    }

    override val surfaceNativePointer: Long
        get() = eglSurface.nativeHandle

    override fun dispose(renderTargetPointer: Long) {
        RiveLog.d(TAG) { "Destroying EGL surface" }
        val destroyed = EGL14.eglDestroySurface(display, eglSurface)
        if (!destroyed) {
            RiveLog.e(TAG) { "Unable to destroy EGL surface" }
        }

        RiveLog.d(TAG) { "Releasing SurfaceTexture" }
        surfaceTexture.release()

        super.dispose(renderTargetPointer)
    }
}

/**
 * Android EGL PBuffer surface for off-screen rendering.
 */
class RiveEGLPBufferSurface(
    private val eglSurface: EGLSurface,
    private val display: EGLDisplay,
    renderTargetPointer: Long,
    drawKey: DrawKey,
    width: Int,
    height: Int
) : RiveSurface(renderTargetPointer, drawKey, width, height) {
    
    companion object {
        const val TAG = "Rive/MP/EGLPBufferSurface"
    }

    override val surfaceNativePointer: Long
        get() = eglSurface.nativeHandle

    override fun dispose(renderTargetPointer: Long) {
        RiveLog.d(TAG) { "Destroying EGL PBuffer surface" }
        val destroyed = EGL14.eglDestroySurface(display, eglSurface)
        if (!destroyed) {
            RiveLog.e(TAG) { "Unable to destroy EGL PBuffer surface" }
        }

        super.dispose(renderTargetPointer)
    }
}

/**
 * Creates the default Android RenderContext (OpenGL ES via EGL).
 */
actual fun createDefaultRenderContext(): RenderContext {
    RiveLog.d("Rive/MP/RenderContext") { "Creating Android RenderContextGL" }
    return RenderContextGL()
}
