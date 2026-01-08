package app.rive.mp.test.rendering

import android.content.Context
import android.content.Intent
import android.graphics.Bitmap
import android.opengl.EGL14
import app.rive.mp.ArtboardHandle
import app.rive.mp.CommandQueue
import app.rive.mp.RiveEGLPBufferSurface
import app.rive.mp.RiveLog
import app.rive.mp.RiveSurface
import app.rive.mp.StateMachineHandle
import app.rive.mp.core.Alignment
import app.rive.mp.core.Fit
import kotlinx.coroutines.delay
import java.nio.ByteBuffer
import java.nio.ByteOrder
import kotlin.time.Duration
import kotlin.time.Duration.Companion.seconds

private const val TAG = "Rive/DebugPreview"

/**
 * Debug utility for visualizing rendered content during tests.
 * 
 * This utility allows tests to optionally display rendered PBuffer content
 * in a visible UI for debugging purposes. When disabled (default), tests
 * run without any visual output, which is ideal for CI/CD environments.
 * 
 * ## Usage
 * 
 * ```kotlin
 * @Test
 * fun myRenderingTest() = runTest {
 *     val testUtil = AndroidRenderTestUtil(this)
 *     try {
 *         // ... render to surface ...
 *         
 *         // Optional: Show preview if enabled
 *         if (DebugRenderPreview.isEnabled) {
 *             val bitmap = DebugRenderPreview.readPixels(surface)
 *             DebugRenderPreview.showBitmap(context, bitmap)
 *         }
 *     } finally {
 *         testUtil.cleanup()
 *     }
 * }
 * ```
 * 
 * To enable debug preview, set [isEnabled] to true before running tests.
 */
object DebugRenderPreview {
    
    /**
     * Whether debug preview is enabled.
     * Set to true to display rendered content in a visible UI.
     * Default is false for CI/CD compatibility.
     */
    var isEnabled = false
    
    /**
     * How long to display static images before auto-closing.
     */
    var staticDisplayDuration: Duration = 3.seconds
    
    /**
     * Read pixels from a PBuffer surface into a Bitmap.
     * 
     * This method reads the current framebuffer content from an EGL PBuffer
     * surface and converts it to an Android Bitmap.
     * 
     * **Note**: Must be called on a thread where the EGL context is current.
     * 
     * @param surface The PBuffer surface to read pixels from.
     * @return A [Bitmap] containing the rendered content.
     */
    fun readPixels(surface: RiveEGLPBufferSurface): Bitmap {
        val width = surface.width
        val height = surface.height
        
        RiveLog.d(TAG) { "Reading pixels from PBuffer (${width}x${height})" }
        
        // Allocate buffer for RGBA pixels
        val buffer = ByteBuffer.allocateDirect(width * height * 4)
            .order(ByteOrder.nativeOrder())
        
        // Read pixels from framebuffer
        // Note: glReadPixels reads from the current framebuffer
        android.opengl.GLES20.glReadPixels(
            0, 0, width, height,
            android.opengl.GLES20.GL_RGBA,
            android.opengl.GLES20.GL_UNSIGNED_BYTE,
            buffer
        )
        
        // Check for errors
        val error = android.opengl.GLES20.glGetError()
        if (error != android.opengl.GLES20.GL_NO_ERROR) {
            RiveLog.w(TAG) { "glReadPixels error: $error" }
        }
        
        buffer.rewind()
        
        // Create bitmap
        val bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
        bitmap.copyPixelsFromBuffer(buffer)
        
        // OpenGL reads bottom-to-top, so flip the bitmap vertically
        val flipped = flipVertically(bitmap)
        bitmap.recycle()
        
        return flipped
    }
    
    /**
     * Flip a bitmap vertically.
     * OpenGL reads pixels bottom-to-top, so we need to flip for correct orientation.
     */
    private fun flipVertically(bitmap: Bitmap): Bitmap {
        val matrix = android.graphics.Matrix()
        matrix.preScale(1f, -1f)
        return Bitmap.createBitmap(bitmap, 0, 0, bitmap.width, bitmap.height, matrix, true)
    }
    
    /**
     * Show a bitmap in a debug preview activity.
     * 
     * @param context Android context for launching the activity.
     * @param bitmap The bitmap to display.
     * @param title Optional title for the preview window.
     */
    fun showBitmap(context: Context, bitmap: Bitmap, title: String = "Render Preview") {
        if (!isEnabled) {
            RiveLog.d(TAG) { "Debug preview disabled, skipping display" }
            return
        }
        
        RiveLog.d(TAG) { "Showing bitmap preview: $title (${bitmap.width}x${bitmap.height})" }
        
        // Store bitmap for the activity to retrieve
        DebugRenderPreviewActivity.pendingBitmap = bitmap
        DebugRenderPreviewActivity.pendingTitle = title
        DebugRenderPreviewActivity.pendingDuration = staticDisplayDuration
        DebugRenderPreviewActivity.animationLoop = null
        
        // Launch preview activity
        val intent = Intent(context, DebugRenderPreviewActivity::class.java)
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        context.startActivity(intent)
    }
    
    /**
     * Show an animation loop in a debug preview activity.
     * 
     * This continuously renders frames and displays them in real-time,
     * allowing visual debugging of animations.
     * 
     * @param context Android context for launching the activity.
     * @param renderTestUtil The render test utility with CommandQueue and surfaces.
     * @param artboardHandle Handle to the artboard to render.
     * @param smHandle Handle to the state machine for animation.
     * @param surface The surface to render to.
     * @param duration How long to run the animation preview.
     * @param fit Fit mode for rendering.
     * @param alignment Alignment for rendering.
     * @param title Optional title for the preview window.
     */
    fun showAnimationLoop(
        context: Context,
        renderTestUtil: AndroidRenderTestUtil,
        artboardHandle: ArtboardHandle,
        smHandle: StateMachineHandle,
        surface: RiveEGLPBufferSurface,
        duration: Duration = 5.seconds,
        fit: Fit = Fit.CONTAIN,
        alignment: Alignment = Alignment.CENTER,
        title: String = "Animation Preview"
    ) {
        if (!isEnabled) {
            RiveLog.d(TAG) { "Debug preview disabled, skipping animation display" }
            return
        }
        
        RiveLog.d(TAG) { "Showing animation preview: $title for $duration" }
        
        // Store animation loop info for the activity
        DebugRenderPreviewActivity.pendingBitmap = null
        DebugRenderPreviewActivity.pendingTitle = title
        DebugRenderPreviewActivity.pendingDuration = duration
        DebugRenderPreviewActivity.animationLoop = AnimationLoopConfig(
            renderTestUtil = renderTestUtil,
            artboardHandle = artboardHandle,
            smHandle = smHandle,
            surface = surface,
            fit = fit,
            alignment = alignment
        )
        
        // Launch preview activity
        val intent = Intent(context, DebugRenderPreviewActivity::class.java)
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        context.startActivity(intent)
    }
}

/**
 * Configuration for animation loop preview.
 */
data class AnimationLoopConfig(
    val renderTestUtil: AndroidRenderTestUtil,
    val artboardHandle: ArtboardHandle,
    val smHandle: StateMachineHandle,
    val surface: RiveEGLPBufferSurface,
    val fit: Fit,
    val alignment: Alignment
)
