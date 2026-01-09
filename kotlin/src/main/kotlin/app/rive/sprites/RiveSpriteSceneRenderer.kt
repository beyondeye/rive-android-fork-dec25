package app.rive.sprites

import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.remember
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.unit.IntSize
import androidx.core.graphics.createBitmap
import app.rive.ExperimentalRiveComposeAPI
import app.rive.RenderBuffer
import app.rive.Alignment
import app.rive.Fit
import app.rive.RiveLog
import app.rive.core.CommandQueue
import app.rive.core.RiveSurface

private const val RENDERER_TAG = "Rive/SpriteRenderer"

/**
 * Remember and manage a [RiveSpriteScene] with proper lifecycle handling.
 *
 * The scene is created when the composable enters composition and is automatically
 * closed when it leaves composition. The command queue reference is acquired when
 * the scene is created and released when the scene is closed.
 *
 * ## Example
 *
 * ```kotlin
 * val commandQueue = rememberCommandQueue()
 * val scene = rememberRiveSpriteScene(commandQueue)
 *
 * // Create sprites
 * val sprite = scene.createSprite(riveFile, stateMachine = "combat")
 *
 * // In your Canvas
 * Canvas(modifier = Modifier.fillMaxSize()) {
 *     drawRiveSprites(scene)
 * }
 * ```
 *
 * @param commandQueue The command queue to use for GPU operations.
 * @return A [RiveSpriteScene] that is automatically managed by the composable lifecycle.
 */
@ExperimentalRiveComposeAPI
@Composable
fun rememberRiveSpriteScene(commandQueue: CommandQueue): RiveSpriteScene {
    val scene = remember(commandQueue) {
        RiveLog.d(RENDERER_TAG) { "Creating RiveSpriteScene" }
        RiveSpriteScene(commandQueue)
    }

    DisposableEffect(scene) {
        onDispose {
            RiveLog.d(RENDERER_TAG) { "Disposing RiveSpriteScene" }
            scene.close()
        }
    }

    return scene
}

/**
 * A buffer manager for rendering sprites to an off-screen bitmap.
 *
 * This class manages:
 * - An off-screen GPU surface for Rive rendering
 * - A bitmap for compositing and displaying in Compose
 * - Size tracking to recreate resources when the viewport changes
 *
 * @param commandQueue The command queue for creating GPU surfaces.
 */
internal class SpriteSceneBuffer(
    private val commandQueue: CommandQueue,
) : AutoCloseable {
    
    private var surface: RiveSurface? = null
    private var renderBuffer: RenderBuffer? = null
    private var compositeBitmap: Bitmap? = null
    private var currentWidth: Int = 0
    private var currentHeight: Int = 0

    /**
     * Ensure the buffer is sized correctly for the given dimensions.
     *
     * If the size has changed, existing resources are released and new ones are created.
     *
     * @param width The required width in pixels.
     * @param height The required height in pixels.
     * @return True if the buffer is ready for rendering, false if creation failed.
     */
    fun ensureSize(width: Int, height: Int): Boolean {
        if (width <= 0 || height <= 0) {
            RiveLog.w(RENDERER_TAG) { "Invalid buffer size: ${width}x$height" }
            return false
        }

        if (width == currentWidth && height == currentHeight && compositeBitmap != null) {
            return true
        }

        RiveLog.d(RENDERER_TAG) { "Resizing buffer from ${currentWidth}x$currentHeight to ${width}x$height" }

        // Release old resources
        releaseResources()

        return try {
            // Create new resources
            renderBuffer = RenderBuffer(width, height, commandQueue)
            compositeBitmap = createBitmap(width, height, Bitmap.Config.ARGB_8888)
            currentWidth = width
            currentHeight = height
            true
        } catch (e: Exception) {
            RiveLog.e(RENDERER_TAG, e) { "Failed to create buffer resources" }
            releaseResources()
            false
        }
    }

    /**
     * Get the composite bitmap for drawing to Compose Canvas.
     *
     * @return The bitmap, or null if the buffer is not initialized.
     */
    fun getBitmap(): Bitmap? = compositeBitmap

    /**
     * Get the render buffer for off-screen GPU rendering.
     *
     * @return The render buffer, or null if the buffer is not initialized.
     */
    fun getRenderBuffer(): RenderBuffer? = renderBuffer

    /**
     * Clear the composite bitmap with a transparent background.
     */
    fun clearBitmap() {
        compositeBitmap?.eraseColor(Color.TRANSPARENT)
    }

    private fun releaseResources() {
        renderBuffer?.close()
        renderBuffer = null
        compositeBitmap?.recycle()
        compositeBitmap = null
        currentWidth = 0
        currentHeight = 0
    }

    override fun close() {
        releaseResources()
    }
}

/**
 * A holder for managing the sprite scene buffer within a Compose context.
 *
 * This wrapper provides state management and ensures proper resource cleanup.
 */
internal class SpriteSceneBufferHolder {
    private var buffer: SpriteSceneBuffer? = null

    fun getOrCreate(commandQueue: CommandQueue): SpriteSceneBuffer {
        return buffer ?: SpriteSceneBuffer(commandQueue).also { buffer = it }
    }

    fun close() {
        buffer?.close()
        buffer = null
    }
}

/**
 * Draw all sprites in a [RiveSpriteScene] to this [DrawScope].
 *
 * Sprites are rendered in z-order (lowest z-index first, highest last), so sprites
 * with higher z-index values appear on top.
 *
 * ## Rendering Modes
 *
 * - [SpriteRenderMode.PER_SPRITE]: Renders each sprite individually (default, more compatible)
 * - [SpriteRenderMode.BATCH]: Renders all sprites in a single GPU pass (better performance)
 *
 * ## Performance Notes
 *
 * For best performance with many sprites (20+), use [SpriteRenderMode.BATCH]. The batch
 * mode uses a single shared GPU surface and renders all sprites in one native call.
 *
 * For debugging or small numbers of sprites, [SpriteRenderMode.PER_SPRITE] is more
 * resilient and easier to debug.
 *
 * ## Transform Application
 *
 * Each sprite's transform (position, scale, rotation, origin) is applied during rendering.
 * The sprite's `size` property determines the display size in DrawScope units.
 *
 * ## Example
 *
 * ```kotlin
 * Canvas(modifier = Modifier.fillMaxSize()) {
 *     // Draw background
 *     drawRect(Color.DarkGray)
 *     
 *     // Draw all Rive sprites (using default per-sprite mode)
 *     drawRiveSprites(scene)
 *     
 *     // Or use batch mode for better performance
 *     drawRiveSprites(scene, renderMode = SpriteRenderMode.BATCH)
 *     
 *     // Draw foreground UI
 *     drawText(textMeasurer, "Score: 100", topLeft = Offset(10f, 10f))
 * }
 * ```
 *
 * @param scene The sprite scene containing the sprites to render.
 * @param renderMode The rendering mode to use. Defaults to [SpriteRenderMode.DEFAULT].
 * @param clearColor The color to clear the render buffer with before drawing sprites.
 *   Defaults to transparent.
 */
@ExperimentalRiveComposeAPI
fun DrawScope.drawRiveSprites(
    scene: RiveSpriteScene,
    renderMode: SpriteRenderMode = SpriteRenderMode.DEFAULT,
    clearColor: Int = Color.TRANSPARENT,
) {
    val sprites = scene.getSortedSprites()
    if (sprites.isEmpty()) {
        return
    }

    val width = size.width.toInt()
    val height = size.height.toInt()

    if (width <= 0 || height <= 0) {
        RiveLog.w(RENDERER_TAG) { "DrawScope has invalid size: ${size.width}x${size.height}" }
        return
    }

    when (renderMode) {
        SpriteRenderMode.BATCH -> drawRiveSpritesBatch(scene, width, height, clearColor)
        SpriteRenderMode.PER_SPRITE -> drawRiveSpritesPerSprite(scene, width, height, clearColor)
    }
}

/**
 * Internal implementation of per-sprite rendering mode.
 *
 * Renders each sprite individually to its own buffer, then composites onto a shared bitmap.
 */
@ExperimentalRiveComposeAPI
private fun DrawScope.drawRiveSpritesPerSprite(
    scene: RiveSpriteScene,
    width: Int,
    height: Int,
    clearColor: Int,
) {
    val sprites = scene.getSortedSprites()

    // Get or create the cached composite bitmap from the scene
    // This avoids creating a new bitmap every frame
    val compositeBitmap = scene.getOrCreateCompositeBitmap(width, height)

    // Clear the bitmap before drawing
    compositeBitmap.eraseColor(clearColor)

    val canvas = Canvas(compositeBitmap)

    // Create a paint for compositing sprites
    val paint = Paint().apply {
        isAntiAlias = true
        isFilterBitmap = true
    }

    // Render each sprite using its cached render buffer
    for (sprite in sprites) {
        if (!sprite.isVisible) continue

        try {
            renderSpriteToCanvas(sprite, scene.commandQueue, canvas, paint)
        } catch (e: Exception) {
            RiveLog.e(RENDERER_TAG, e) { "Failed to render sprite" }
        }
    }

    // Draw the composite bitmap to the Compose Canvas
    // Note: The bitmap is NOT recycled - it's cached in the scene for reuse
    drawImage(
        image = compositeBitmap.asImageBitmap(),
        topLeft = androidx.compose.ui.geometry.Offset.Zero,
    )

    // Clear dirty flag since we've rendered
    scene.clearDirty()
}

/**
 * Internal implementation of batch rendering mode.
 *
 * Renders all sprites in a single GPU pass using [CommandQueue.drawMultipleToBuffer].
 * This is more efficient for many sprites as it:
 * 1. Batches all sprites into a single native call
 * 2. Renders all sprites to a shared GPU surface
 * 3. Reads pixels back synchronously for display
 *
 * The method blocks until rendering is complete (~1-5ms typically), which is
 * acceptable for Compose Canvas drawing as it's already a blocking operation.
 *
 * On any error, silently falls back to per-sprite rendering mode.
 */
@ExperimentalRiveComposeAPI
private fun DrawScope.drawRiveSpritesBatch(
    scene: RiveSpriteScene,
    width: Int,
    height: Int,
    clearColor: Int,
) {
    try {
        // Get or create the shared surface for batch rendering
        val surface = scene.getOrCreateSharedSurface(width, height)

        // Build draw commands from all visible sprites
        val commands = scene.buildDrawCommands()

        if (commands.isEmpty()) {
            return
        }

        // Get the pixel buffer for receiving rendered pixels
        val pixelBuffer = scene.getPixelBuffer()
            ?: throw IllegalStateException("Pixel buffer not initialized")

        // Render all sprites in one GPU pass with synchronous pixel readback
        // This is the Phase 4.4 implementation - true batch rendering
        scene.commandQueue.drawMultipleToBuffer(
            commands = commands,
            surface = surface,
            buffer = pixelBuffer,
            viewportWidth = width,
            viewportHeight = height,
            clearColor = clearColor
        )

        // Get the composite bitmap for final output
        val compositeBitmap = scene.getOrCreateCompositeBitmap(width, height)

        // Convert pixel buffer (RGBA) to bitmap (ARGB_8888)
        copyPixelBufferToBitmap(pixelBuffer, compositeBitmap, width, height)

        drawImage(
            image = compositeBitmap.asImageBitmap(),
            topLeft = androidx.compose.ui.geometry.Offset.Zero,
        )

        // Clear dirty flag since we've rendered
        scene.clearDirty()

    } catch (e: UnsatisfiedLinkError) {
        // Native batch rendering not available, fall back to per-sprite
        RiveLog.d(RENDERER_TAG) {
            "Batch rendering not available (native method missing), falling back to per-sprite"
        }
        drawRiveSpritesPerSprite(scene, width, height, clearColor)
    } catch (e: Exception) {
        // Any other error, silently fall back to per-sprite rendering
        RiveLog.d(RENDERER_TAG) { "Batch rendering failed, using per-sprite fallback" }
        drawRiveSpritesPerSprite(scene, width, height, clearColor)
    }
}

/**
 * Reusable ByteBuffer for pixel buffer copies (Step 4 optimization).
 * Cached to avoid creating a wrapper object every frame.
 */
private var reusablePixelByteBuffer: java.nio.ByteBuffer? = null

/**
 * Copy pixels from a BGRA byte buffer to an ARGB_8888 bitmap (OPTIMIZED).
 *
 * The native code outputs BGRA format which matches Android's ARGB_8888 byte order
 * on little-endian systems. This allows direct buffer copy without per-pixel conversion.
 *
 * **Optimization (Step 4):** This method reuses a cached ByteBuffer instead of creating
 * a new wrapper object every frame, reducing allocations and CPU overhead.
 *
 * @param buffer Source buffer in BGRA format (4 bytes per pixel).
 * @param bitmap Destination bitmap in ARGB_8888 format.
 * @param width Width in pixels.
 * @param height Height in pixels.
 */
private fun copyPixelBufferToBitmap(
    buffer: ByteArray,
    bitmap: Bitmap,
    width: Int,
    height: Int,
) {
    // Check if we need to recreate the buffer (size changed)
    val requiredCapacity = width * height * 4
    if (reusablePixelByteBuffer?.capacity() != requiredCapacity) {
        reusablePixelByteBuffer = null
    }
    
    // Get or create the reusable ByteBuffer
    val byteBuffer = reusablePixelByteBuffer ?: java.nio.ByteBuffer.wrap(buffer).also {
        reusablePixelByteBuffer = it
    }
    
    // Update buffer contents and copy to bitmap
    byteBuffer.rewind()
    byteBuffer.put(buffer)
    byteBuffer.rewind()
    bitmap.copyPixelsFromBuffer(byteBuffer)
}

/**
 * Original implementation - creates new ByteBuffer wrapper every frame.
 *
 * **This method is kept for reference and potential fallback.**
 * 
 * @deprecated Use [copyPixelBufferToBitmap] instead for better performance (Step 4 optimization).
 * The optimized version reuses a cached ByteBuffer to avoid allocations.
 */
@Deprecated(
    message = "Use copyPixelBufferToBitmap instead (optimized version with buffer reuse)",
    replaceWith = ReplaceWith("copyPixelBufferToBitmap(buffer, bitmap, width, height)")
)
@Suppress("UNUSED")
private fun copyPixelBufferToBitmapOriginal(
    buffer: ByteArray,
    bitmap: Bitmap,
    width: Int,
    height: Int,
) {
    // Native code outputs BGRA which matches ARGB_8888 byte order on little-endian
    // Use direct buffer copy for optimal performance
    val byteBuffer = java.nio.ByteBuffer.wrap(buffer)
    bitmap.copyPixelsFromBuffer(byteBuffer)
}

/**
 * Render a single sprite to an Android Canvas with its transform applied.
 *
 * This method uses the sprite's cached render buffer to avoid expensive GPU
 * resource allocation on every frame. The buffer is reused across frames and
 * only recreated when the sprite's size changes.
 *
 * @param sprite The sprite to render.
 * @param commandQueue The command queue for GPU operations (unused, kept for API compatibility).
 * @param targetCanvas The canvas to draw the sprite to.
 * @param paint The paint to use for drawing.
 */
@Suppress("UNUSED_PARAMETER") // commandQueue kept for API consistency, may be used in future optimizations
private fun renderSpriteToCanvas(
    sprite: RiveSprite,
    commandQueue: CommandQueue,
    targetCanvas: Canvas,
    paint: Paint,
) {
    // Use the sprite's cached render buffer instead of creating a new one each frame
    val spriteBuffer = sprite.getOrCreateRenderBuffer()

    // Render the sprite's artboard to the buffer
    spriteBuffer.snapshot(
        artboard = sprite.artboard,
        stateMachine = sprite.stateMachine,
        fit = Fit.Fill,  // Fill the sprite size
        clearColor = Color.TRANSPARENT
    )

    // Convert to bitmap
    val spriteBitmap = spriteBuffer.toBitmap()

    // Apply the sprite's transform and draw to the target canvas
    targetCanvas.save()
    targetCanvas.concat(sprite.computeTransformMatrixData().asMatrix())
    targetCanvas.drawBitmap(spriteBitmap, 0f, 0f, paint)
    targetCanvas.restore()

    // Clean up the sprite bitmap (the render buffer is NOT closed - it's cached)
    spriteBitmap.recycle()
}

/**
 * Draw sprites from a scene to this DrawScope with caching support.
 *
 * This overload accepts a buffer holder for caching the composite bitmap across frames,
 * which can improve performance by avoiding repeated bitmap allocations.
 *
 * ## Example
 *
 * ```kotlin
 * val bufferHolder = remember { SpriteSceneBufferHolder() }
 *
 * DisposableEffect(Unit) {
 *     onDispose { bufferHolder.close() }
 * }
 *
 * Canvas(modifier = Modifier.fillMaxSize()) {
 *     drawRiveSprites(scene, bufferHolder)
 * }
 * ```
 *
 * @param scene The sprite scene containing the sprites to render.
 * @param bufferHolder A holder for caching the render buffer.
 * @param clearColor The color to clear the render buffer with before drawing sprites.
 */
@ExperimentalRiveComposeAPI
internal fun DrawScope.drawRiveSprites(
    scene: RiveSpriteScene,
    bufferHolder: SpriteSceneBufferHolder,
    clearColor: Int = Color.TRANSPARENT,
) {
    val sprites = scene.getSortedSprites()
    if (sprites.isEmpty()) {
        return
    }

    val width = size.width.toInt()
    val height = size.height.toInt()

    if (width <= 0 || height <= 0) {
        RiveLog.w(RENDERER_TAG) { "DrawScope has invalid size: ${size.width}x${size.height}" }
        return
    }

    val buffer = bufferHolder.getOrCreate(scene.commandQueue)
    if (!buffer.ensureSize(width, height)) {
        RiveLog.e(RENDERER_TAG) { "Failed to ensure buffer size" }
        return
    }

    val bitmap = buffer.getBitmap() ?: return
    buffer.clearBitmap()

    val canvas = Canvas(bitmap)
    canvas.drawColor(clearColor)

    val paint = Paint().apply {
        isAntiAlias = true
        isFilterBitmap = true
    }

    for (sprite in sprites) {
        if (!sprite.isVisible) continue

        try {
            renderSpriteToCanvas(sprite, scene.commandQueue, canvas, paint)
        } catch (e: Exception) {
            RiveLog.e(RENDERER_TAG, e) { "Failed to render sprite" }
        }
    }

    drawImage(
        image = bitmap.asImageBitmap(),
        topLeft = androidx.compose.ui.geometry.Offset.Zero,
    )
}

/**
 * Extension property to get the size of the DrawScope as an [IntSize].
 */
private val DrawScope.intSize: IntSize
    get() = IntSize(size.width.toInt(), size.height.toInt())

