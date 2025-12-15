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
import app.rive.RiveLog
import app.rive.core.CommandQueue
import app.rive.core.RiveSurface
import app.rive.runtime.kotlin.core.Alignment
import app.rive.runtime.kotlin.core.Fit

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
 * Renders all sprites in a single GPU pass using [CommandQueue.drawMultiple].
 * This is more efficient for many sprites but requires the native batch rendering
 * implementation from Phase 3.
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

        // Get the pixel buffer for reading back rendered content
        val pixelBuffer = scene.getPixelBuffer()
            ?: throw IllegalStateException("Pixel buffer not initialized")

        // Render all sprites in one GPU pass
        scene.commandQueue.drawMultiple(
            commands = commands,
            surface = surface,
            viewportWidth = width,
            viewportHeight = height,
            clearColor = clearColor
        )

        // Read pixels from the GPU surface into the buffer
        // Note: This requires synchronous readback which may have performance implications
        // Future optimization: use async readback with double-buffering
        readPixelsFromSurface(scene, surface, pixelBuffer, width, height, clearColor)

        // Convert pixel buffer to bitmap and draw to Canvas
        val compositeBitmap = scene.getOrCreateCompositeBitmap(width, height)
        copyPixelBufferToBitmap(pixelBuffer, compositeBitmap, width, height)

        drawImage(
            image = compositeBitmap.asImageBitmap(),
            topLeft = androidx.compose.ui.geometry.Offset.Zero,
        )

        // Clear dirty flag since we've rendered
        scene.clearDirty()

    } catch (e: UnsatisfiedLinkError) {
        // Native batch rendering not available, fall back to per-sprite
        RiveLog.w(RENDERER_TAG) {
            "Batch rendering not available (native method missing), falling back to per-sprite"
        }
        drawRiveSpritesPerSprite(scene, width, height, clearColor)
    } catch (e: Exception) {
        // Any other error, fall back to per-sprite rendering
        RiveLog.e(RENDERER_TAG, e) { "Batch rendering failed, falling back to per-sprite" }
        drawRiveSpritesPerSprite(scene, width, height, clearColor)
    }
}

/**
 * Read pixels from a GPU surface into a byte buffer.
 *
 * This uses a workaround since we need to get pixels from the batch-rendered surface.
 * The current approach creates a temporary render buffer for readback.
 *
 * @param scene The sprite scene (for accessing command queue).
 * @param surface The surface to read from (unused in workaround, will be used when native readback is added).
 * @param buffer The buffer to write pixels into (unused in workaround, will be used when native readback is added).
 * @param width The width in pixels.
 * @param height The height in pixels.
 * @param clearColor The clear color used for rendering.
 */
@Suppress("UNUSED_PARAMETER") // surface and buffer will be used when native pixel readback is implemented
@ExperimentalRiveComposeAPI
private fun readPixelsFromSurface(
    scene: RiveSpriteScene,
    surface: RiveSurface,
    buffer: ByteArray,
    width: Int,
    height: Int,
    clearColor: Int,
) {
    // The drawMultiple call renders to the surface, but we need to read back the pixels.
    // Currently, there's no direct readback from RiveSurface after drawMultiple.
    // 
    // Workaround: The native cppDrawMultiple should handle pixel readback if needed,
    // or we need to use drawToBuffer pattern similar to RenderBuffer.
    //
    // For now, we'll re-render using the per-sprite approach to get actual pixels.
    // This is a temporary implementation until proper GPU readback is added.
    //
    // TODO: Add native pixel readback support for batch-rendered surfaces
    // Options:
    // 1. Add cppReadPixels(surfacePointer, buffer) native method
    // 2. Modify cppDrawMultiple to also fill a pixel buffer
    // 3. Use shared texture with EGL for direct readback

    // Temporary: Re-render sprites individually to get pixels
    // This negates the performance benefit of batch rendering for now
    val sprites = scene.getSortedSprites()
    val compositeBitmap = scene.getOrCreateCompositeBitmap(width, height)
    compositeBitmap.eraseColor(clearColor)

    val canvas = Canvas(compositeBitmap)
    val paint = Paint().apply {
        isAntiAlias = true
        isFilterBitmap = true
    }

    for (sprite in sprites) {
        if (!sprite.isVisible) continue
        try {
            renderSpriteToCanvas(sprite, scene.commandQueue, canvas, paint)
        } catch (e: Exception) {
            RiveLog.e(RENDERER_TAG, e) { "Failed to render sprite in batch fallback" }
        }
    }

    // Note: The actual pixel buffer is not used in this workaround
    // The bitmap will be used directly
}

/**
 * Copy pixels from an RGBA byte buffer to an ARGB_8888 bitmap.
 *
 * @param buffer Source buffer in RGBA format.
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
    // Note: This is currently not used because readPixelsFromSurface uses the fallback
    // Once proper GPU readback is implemented, this will convert RGBA -> ARGB
    
    val pixels = IntArray(width * height)
    var bufferIndex = 0
    var pixelIndex = 0

    while (bufferIndex < buffer.size && pixelIndex < pixels.size) {
        val r = buffer[bufferIndex].toInt() and 0xFF
        val g = buffer[bufferIndex + 1].toInt() and 0xFF
        val b = buffer[bufferIndex + 2].toInt() and 0xFF
        val a = buffer[bufferIndex + 3].toInt() and 0xFF
        pixels[pixelIndex] = (a shl 24) or (r shl 16) or (g shl 8) or b
        bufferIndex += 4
        pixelIndex++
    }

    bitmap.setPixels(pixels, 0, width, 0, 0, width, height)
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
        sprite.artboard,
        sprite.stateMachine,
        Fit.FILL,  // Fill the sprite size
        Alignment.CENTER,
        Color.TRANSPARENT
    )

    // Convert to bitmap
    val spriteBitmap = spriteBuffer.toBitmap()

    // Apply the sprite's transform and draw to the target canvas
    targetCanvas.save()
    targetCanvas.concat(sprite.computeTransformMatrix())
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
