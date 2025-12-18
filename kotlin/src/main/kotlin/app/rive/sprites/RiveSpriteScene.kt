package app.rive.sprites

import android.graphics.Bitmap
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.snapshots.SnapshotStateList
import androidx.annotation.ColorInt
import androidx.compose.ui.geometry.Offset
import androidx.core.graphics.createBitmap
import app.rive.RiveFile
import app.rive.RiveLog
import app.rive.core.CloseOnce
import app.rive.core.CommandQueue
import app.rive.core.RiveSurface
import app.rive.core.SpriteDrawCommand
import app.rive.sprites.props.SpriteControlProp
import kotlin.time.Duration

private const val SPRITE_SCENE_TAG = "Rive/SpriteScene"

/**
 * A scene that manages multiple [RiveSprite] instances for game engine integration.
 *
 * The scene provides:
 * - Sprite creation and lifecycle management
 * - Batch animation advancement
 * - Z-order sorted rendering (via [getSortedSprites])
 * - Hit testing with z-order awareness
 * - Shared GPU surface for batch rendering
 *
 * ## Surface Ownership
 *
 * The scene owns the shared GPU surface used for batch rendering. This design choice
 * provides simpler lifecycle management (surface tied to scene lifetime).
 *
 * **Current approach (Scene-owned surface):**
 * - Pros:
 *   - Simpler lifecycle management (surface tied to scene)
 *   - Single source of truth for surface state
 *   - Easy to clean up (closed with scene)
 * - Cons:
 *   - Surface size may not match if scene is used with multiple canvases
 *   - Less flexible for advanced use cases
 *
 * **Alternative (Renderer-owned surface):**
 * - Pros:
 *   - Surface size always matches the Canvas it's drawn to
 *   - Better for multi-canvas scenarios
 *   - More flexible composition
 * - Cons:
 *   - More complex lifecycle (need DisposableEffect tracking)
 *   - Need to pass surface around or use holders
 *
 * See Phase 4.3 in the implementation plan for migrating to renderer-owned surface.
 *
 * ## Usage
 *
 * ```kotlin
 * val scene = RiveSpriteScene(commandQueue)
 *
 * // Create sprites
 * val enemy = scene.createSprite(enemyFile, stateMachineName = "combat")
 * enemy.position = Offset(100f, 200f)
 * enemy.size = Size(64f, 64f)
 *
 * // Game loop
 * LaunchedEffect(Unit) {
 *     while (isActive) {
 *         withFrameNanos { frameTime ->
 *             scene.advance(deltaTime)
 *         }
 *     }
 * }
 *
 * // Hit testing
 * val hitSprite = scene.hitTest(touchPoint)
 * hitSprite?.fire("attack")
 *
 * // Cleanup
 * scene.close()
 * ```
 *
 * @param commandQueue The command queue for GPU operations. The scene acquires a reference
 *   and releases it when closed.
 */
class RiveSpriteScene(
    val commandQueue: CommandQueue
) : AutoCloseable {

    private val closeOnce = CloseOnce(SPRITE_SCENE_TAG) {
        RiveLog.d(SPRITE_SCENE_TAG) { "Closing scene with ${_sprites.size} sprites" }
        
        // Close all sprites
        _sprites.forEach { sprite ->
            try {
                sprite.close()
            } catch (e: Exception) {
                RiveLog.e(SPRITE_SCENE_TAG, e) { "Error closing sprite" }
            }
        }
        _sprites.clear()
        
        // Release cached composite bitmap
        cachedCompositeBitmap?.recycle()
        cachedCompositeBitmap = null
        cachedBitmapWidth = 0
        cachedBitmapHeight = 0
        
        // Release shared surface for batch rendering
        sharedSurface?.close()
        sharedSurface = null
        sharedSurfaceWidth = 0
        sharedSurfaceHeight = 0
        pixelBuffer = null
        
        // Release command queue reference
        commandQueue.release(SPRITE_SCENE_TAG, "Scene closed")
    }

    init {
        RiveLog.d(SPRITE_SCENE_TAG) { "Creating sprite scene" }
        commandQueue.acquire(SPRITE_SCENE_TAG)
    }

    /**
     * The internal mutable list of sprites managed by this scene.
     * Uses [SnapshotStateList] for Compose state integration.
     */
    private val _sprites: SnapshotStateList<RiveSprite> = mutableStateListOf()

    // region Composite Bitmap Cache

    /**
     * Cached composite bitmap for rendering all sprites.
     * Reused across frames to avoid expensive bitmap allocations.
     * Only recreated when the canvas size changes.
     */
    private var cachedCompositeBitmap: Bitmap? = null
    private var cachedBitmapWidth: Int = 0
    private var cachedBitmapHeight: Int = 0

    /**
     * Get or create a composite bitmap for rendering sprites.
     *
     * The bitmap is cached and reused across frames. It is only recreated when
     * the required size changes.
     *
     * @param width The required width in pixels.
     * @param height The required height in pixels.
     * @return A bitmap sized to the requested dimensions.
     */
    internal fun getOrCreateCompositeBitmap(width: Int, height: Int): Bitmap {
        val existingBitmap = cachedCompositeBitmap
        if (existingBitmap != null && 
            cachedBitmapWidth == width && 
            cachedBitmapHeight == height) {
            return existingBitmap
        }

        // Size changed or no bitmap exists - create a new one
        existingBitmap?.recycle()
        
        RiveLog.d(SPRITE_SCENE_TAG) { 
            "Creating composite bitmap ${width}x$height " +
            "(was ${cachedBitmapWidth}x$cachedBitmapHeight)" 
        }

        val newBitmap = createBitmap(width, height, Bitmap.Config.ARGB_8888)
        cachedCompositeBitmap = newBitmap
        cachedBitmapWidth = width
        cachedBitmapHeight = height
        return newBitmap
    }

    // endregion

    // region Shared Surface for Batch Rendering

    /**
     * Shared GPU surface for batch rendering all sprites.
     *
     * This surface is used when [SpriteRenderMode.BATCH] is selected. All sprites
     * are rendered to this single surface in one GPU pass, which is more efficient
     * than creating individual surfaces per sprite.
     *
     * The surface is created lazily when batch rendering is first requested and
     * is recreated if the viewport size changes.
     */
    private var sharedSurface: RiveSurface? = null
    private var sharedSurfaceWidth: Int = 0
    private var sharedSurfaceHeight: Int = 0

    /**
     * Pixel buffer for reading rendered content from the shared surface.
     *
     * This buffer is used to transfer GPU-rendered content to a CPU-accessible
     * bitmap for display in Compose Canvas.
     */
    private var pixelBuffer: ByteArray? = null

    /**
     * Get or create the shared GPU surface for batch rendering.
     *
     * The surface is cached and reused across frames. It is only recreated when
     * the viewport size changes.
     *
     * @param width The required width in pixels.
     * @param height The required height in pixels.
     * @return A [RiveSurface] sized to the requested dimensions.
     * @throws RuntimeException If the surface cannot be created.
     */
    internal fun getOrCreateSharedSurface(width: Int, height: Int): RiveSurface {
        check(!closeOnce.closed) { "Cannot get surface from a closed scene" }

        val existingSurface = sharedSurface
        if (existingSurface != null &&
            sharedSurfaceWidth == width &&
            sharedSurfaceHeight == height
        ) {
            return existingSurface
        }

        // Size changed or no surface exists - create a new one
        existingSurface?.close()

        RiveLog.d(SPRITE_SCENE_TAG) {
            "Creating shared surface ${width}x$height " +
                    "(was ${sharedSurfaceWidth}x$sharedSurfaceHeight)"
        }

        val newSurface = commandQueue.createImageSurface(width, height)
        sharedSurface = newSurface
        sharedSurfaceWidth = width
        sharedSurfaceHeight = height

        // Also resize the pixel buffer
        val bufferSize = width * height * 4 // RGBA
        if (pixelBuffer == null || pixelBuffer!!.size != bufferSize) {
            pixelBuffer = ByteArray(bufferSize)
        }

        return newSurface
    }

    /**
     * Get the pixel buffer for reading from the shared surface.
     *
     * @return The pixel buffer, or null if the shared surface hasn't been created.
     */
    internal fun getPixelBuffer(): ByteArray? = pixelBuffer

    /**
     * Build a list of [SpriteDrawCommand] for batch rendering.
     *
     * This converts all visible sprites into draw commands that can be passed
     * to [CommandQueue.drawMultiple].
     *
     * @return List of draw commands, sorted by z-index.
     */
    internal fun buildDrawCommands(): List<SpriteDrawCommand> {
        return getSortedSprites().map { sprite ->
            val effectiveSize = sprite.effectiveSize
            SpriteDrawCommand(
                artboardHandle = sprite.artboardHandle,
                stateMachineHandle = sprite.stateMachineHandle,
                transform = sprite.computeTransformArray(),
                artboardWidth = effectiveSize.width,
                artboardHeight = effectiveSize.height
            )
        }
    }

    // endregion

    // region Dirty Tracking

    /**
     * Flag indicating whether the scene needs to be re-rendered.
     *
     * This is set to true when:
     * - Sprites are added, removed, or reordered
     * - Sprite visibility changes
     * - (Note: transform changes are handled by Compose state)
     *
     * Used by the renderer to skip re-rendering when the scene hasn't changed
     * and animations haven't advanced.
     */
    @Volatile
    internal var isDirty: Boolean = true
        private set

    /**
     * Mark the scene as dirty, requiring re-render.
     */
    internal fun markDirty() {
        isDirty = true
    }

    /**
     * Clear the dirty flag after rendering.
     */
    internal fun clearDirty() {
        isDirty = false
    }

    // endregion

    /**
     * Read-only view of all sprites in this scene.
     *
     * Note: The order is not guaranteed to be sorted by z-index. Use [getSortedSprites]
     * for rendering order.
     */
    val sprites: List<RiveSprite>
        get() = _sprites

    /**
     * The number of sprites currently in the scene.
     */
    val spriteCount: Int
        get() = _sprites.size

    // region Sprite Management

    /**
     * Create a new sprite from a [RiveFile] and add it to the scene.
     *
     * The sprite is created with default transform values:
     * - position: (0, 0)
     * - size: Unspecified (uses artboard size)
     * - scale: 1.0
     * - rotation: 0
     * - zIndex: 0
     * - origin: Center
     *
     * @param file The Rive file to create the sprite from.
     * @param artboardName The name of the artboard to use, or null for the default.
     * @param stateMachineName The name of the state machine to use, or null for the default.
     * @param viewModelConfig Configuration for ViewModelInstance creation (default: None).
     * @param tags Initial tags for grouping this sprite.
     * @return The created [RiveSprite], which is already added to the scene.
     */
    fun createSprite(
        file: RiveFile,
        artboardName: String? = null,
        stateMachineName: String? = null,
        viewModelConfig: SpriteViewModelConfig = SpriteViewModelConfig.None,
        tags: Set<SpriteTag> = emptySet()
    ): RiveSprite {
        check(!closeOnce.closed) { "Cannot create sprite on a closed scene" }
        
        val sprite = RiveSprite.fromFile(
            file = file,
            artboardName = artboardName,
            stateMachineName = stateMachineName,
            viewModelConfig = viewModelConfig,
            tags = tags
        )
        _sprites.add(sprite)
        markDirty()
        
        RiveLog.d(SPRITE_SCENE_TAG) { 
            "Created sprite (artboard=${artboardName ?: "default"}, " +
            "stateMachine=${stateMachineName ?: "default"}, " +
            "tags=$tags), total=${_sprites.size}" 
        }
        
        return sprite
    }

    /**
     * Add an existing sprite to the scene.
     *
     * Note: The scene takes ownership of the sprite's lifecycle. It will be closed
     * when the scene is closed or when [removeSprite] is called.
     *
     * @param sprite The sprite to add.
     * @throws IllegalStateException If the scene is closed.
     * @throws IllegalArgumentException If the sprite is already in the scene.
     */
    fun addSprite(sprite: RiveSprite) {
        check(!closeOnce.closed) { "Cannot add sprite to a closed scene" }
        require(sprite !in _sprites) { "Sprite is already in this scene" }
        
        _sprites.add(sprite)
        markDirty()
        RiveLog.d(SPRITE_SCENE_TAG) { "Added sprite, total=${_sprites.size}" }
    }

    /**
     * Remove a sprite from the scene and close it.
     *
     * @param sprite The sprite to remove.
     * @return True if the sprite was found and removed, false otherwise.
     */
    fun removeSprite(sprite: RiveSprite): Boolean {
        val removed = _sprites.remove(sprite)
        if (removed) {
            sprite.close()
            markDirty()
            RiveLog.d(SPRITE_SCENE_TAG) { "Removed sprite, total=${_sprites.size}" }
        }
        return removed
    }

    /**
     * Remove a sprite from the scene without closing it.
     *
     * Use this when you want to transfer ownership of a sprite to another scene
     * or manage its lifecycle manually.
     *
     * @param sprite The sprite to detach.
     * @return True if the sprite was found and detached, false otherwise.
     */
    fun detachSprite(sprite: RiveSprite): Boolean {
        val removed = _sprites.remove(sprite)
        if (removed) {
            markDirty()
            RiveLog.d(SPRITE_SCENE_TAG) { "Detached sprite, total=${_sprites.size}" }
        }
        return removed
    }

    /**
     * Remove all sprites from the scene and close them.
     */
    fun clearSprites() {
        RiveLog.d(SPRITE_SCENE_TAG) { "Clearing ${_sprites.size} sprites" }
        _sprites.forEach { it.close() }
        _sprites.clear()
        markDirty()
    }

    // endregion

    // region Tag-Based Sprite Selection

    /**
     * Get all sprites that have a specific tag.
     *
     * @param tag The tag to filter by.
     * @return A list of sprites with the specified tag.
     */
    fun getSpritesWithTag(tag: SpriteTag): List<RiveSprite> =
        _sprites.filter { it.hasTag(tag) }

    /**
     * Get all sprites that have any of the specified tags.
     *
     * @param tags The tags to filter by.
     * @return A list of sprites that have at least one of the specified tags.
     */
    fun getSpritesWithAnyTag(tags: List<SpriteTag>): List<RiveSprite> =
        _sprites.filter { sprite -> tags.any { sprite.hasTag(it) } }

    /**
     * Get all sprites that have all of the specified tags.
     *
     * @param tags The tags to filter by.
     * @return A list of sprites that have all of the specified tags.
     */
    fun getSpritesWithAllTags(tags: List<SpriteTag>): List<RiveSprite> =
        _sprites.filter { sprite -> tags.all { sprite.hasTag(it) } }

    // endregion

    // region Batch Property Operations

    /**
     * Fire a trigger on all sprites with the specified tag.
     *
     * @param tag The tag to filter sprites by.
     * @param prop The trigger property descriptor.
     */
    fun fireTrigger(tag: SpriteTag, prop: SpriteControlProp.Trigger) {
        getSpritesWithTag(tag).forEach { it.fireTrigger(prop) }
    }

    /**
     * Fire a trigger on all sprites with the specified tag using a property path.
     *
     * @param tag The tag to filter sprites by.
     * @param propertyPath The path to the trigger property.
     */
    fun fireTrigger(tag: SpriteTag, propertyPath: String) {
        getSpritesWithTag(tag).forEach { it.fireTrigger(propertyPath) }
    }

    /**
     * Set a number property on all sprites with the specified tag.
     *
     * @param tag The tag to filter sprites by.
     * @param prop The number property descriptor.
     * @param value The value to set.
     */
    fun setNumber(tag: SpriteTag, prop: SpriteControlProp.Number, value: Float) {
        getSpritesWithTag(tag).forEach { it.setNumber(prop, value) }
    }

    /**
     * Set a number property on all sprites with the specified tag using a property path.
     *
     * @param tag The tag to filter sprites by.
     * @param propertyPath The path to the property.
     * @param value The value to set.
     */
    fun setNumber(tag: SpriteTag, propertyPath: String, value: Float) {
        getSpritesWithTag(tag).forEach { it.setNumber(propertyPath, value) }
    }

    /**
     * Set a boolean property on all sprites with the specified tag.
     *
     * @param tag The tag to filter sprites by.
     * @param prop The boolean property descriptor.
     * @param value The value to set.
     */
    fun setBoolean(tag: SpriteTag, prop: SpriteControlProp.Toggle, value: Boolean) {
        getSpritesWithTag(tag).forEach { it.setBoolean(prop, value) }
    }

    /**
     * Set a boolean property on all sprites with the specified tag using a property path.
     *
     * @param tag The tag to filter sprites by.
     * @param propertyPath The path to the property.
     * @param value The value to set.
     */
    fun setBoolean(tag: SpriteTag, propertyPath: String, value: Boolean) {
        getSpritesWithTag(tag).forEach { it.setBoolean(propertyPath, value) }
    }

    /**
     * Set a string property on all sprites with the specified tag.
     *
     * @param tag The tag to filter sprites by.
     * @param prop The string property descriptor.
     * @param value The value to set.
     */
    fun setString(tag: SpriteTag, prop: SpriteControlProp.Text, value: String) {
        getSpritesWithTag(tag).forEach { it.setString(prop, value) }
    }

    /**
     * Set a string property on all sprites with the specified tag using a property path.
     *
     * @param tag The tag to filter sprites by.
     * @param propertyPath The path to the property.
     * @param value The value to set.
     */
    fun setString(tag: SpriteTag, propertyPath: String, value: String) {
        getSpritesWithTag(tag).forEach { it.setString(propertyPath, value) }
    }

    /**
     * Set an enum property on all sprites with the specified tag.
     *
     * @param tag The tag to filter sprites by.
     * @param prop The enum property descriptor.
     * @param value The value to set.
     */
    fun setEnum(tag: SpriteTag, prop: SpriteControlProp.Choice, value: String) {
        getSpritesWithTag(tag).forEach { it.setEnum(prop, value) }
    }

    /**
     * Set an enum property on all sprites with the specified tag using a property path.
     *
     * @param tag The tag to filter sprites by.
     * @param propertyPath The path to the property.
     * @param value The value to set.
     */
    fun setEnum(tag: SpriteTag, propertyPath: String, value: String) {
        getSpritesWithTag(tag).forEach { it.setEnum(propertyPath, value) }
    }

    /**
     * Set a color property on all sprites with the specified tag.
     *
     * @param tag The tag to filter sprites by.
     * @param prop The color property descriptor.
     * @param value The color value as ARGB integer.
     */
    fun setColor(tag: SpriteTag, prop: SpriteControlProp.Color, @ColorInt value: Int) {
        getSpritesWithTag(tag).forEach { it.setColor(prop, value) }
    }

    /**
     * Set a color property on all sprites with the specified tag using a property path.
     *
     * @param tag The tag to filter sprites by.
     * @param propertyPath The path to the property.
     * @param value The color value as ARGB integer.
     */
    fun setColor(tag: SpriteTag, propertyPath: String, @ColorInt value: Int) {
        getSpritesWithTag(tag).forEach { it.setColor(propertyPath, value) }
    }

    // endregion

    // region Animation

    /**
     * Advance all sprites' state machines by the given time delta.
     *
     * Only visible sprites are advanced to optimize performance.
     *
     * @param deltaTime The time to advance by.
     */
    fun advance(deltaTime: Duration) {
        check(!closeOnce.closed) { "Cannot advance a closed scene" }
        
        _sprites.forEach { sprite ->
            if (sprite.isVisible) {
                sprite.advance(deltaTime)
            }
        }
    }

    // endregion

    // region Hit Testing

    /**
     * Find the topmost sprite at the given point.
     *
     * Sprites are tested in reverse z-order (highest z-index first), so the
     * visually topmost sprite is returned.
     *
     * @param point The point to test in DrawScope coordinates.
     * @return The topmost sprite that contains the point, or null if none found.
     */
    fun hitTest(point: Offset): RiveSprite? {
        check(!closeOnce.closed) { "Cannot hit test a closed scene" }
        
        // Test in reverse z-order (top to bottom)
        return getSortedSprites()
            .asReversed()
            .firstOrNull { sprite -> sprite.hitTest(point) }
    }

    /**
     * Find all sprites at the given point, sorted from topmost to bottommost.
     *
     * @param point The point to test in DrawScope coordinates.
     * @return A list of all sprites that contain the point, sorted by z-index descending.
     */
    fun hitTestAll(point: Offset): List<RiveSprite> {
        check(!closeOnce.closed) { "Cannot hit test a closed scene" }
        
        return getSortedSprites()
            .asReversed()
            .filter { sprite -> sprite.hitTest(point) }
    }

    // endregion

    // region Pointer Event Routing

    /**
     * Send a pointer down event to the topmost sprite at the given point.
     *
     * This is a convenience method that combines hit testing and event forwarding.
     * The event is only sent to the topmost hit sprite (highest z-index).
     *
     * ## Usage
     *
     * ```kotlin
     * Canvas(modifier = Modifier
     *     .fillMaxSize()
     *     .pointerInput(Unit) {
     *         detectTapGestures(
     *             onPress = { offset ->
     *                 scene.pointerDown(offset)
     *             }
     *         )
     *     }
     * ) {
     *     drawRiveSprites(scene)
     * }
     * ```
     *
     * @param point The pointer position in DrawScope coordinates.
     * @param pointerID The ID of the pointer (for multi-touch), defaults to 0.
     * @return The sprite that received the event, or null if no sprite was hit.
     */
    fun pointerDown(point: Offset, pointerID: Int = 0): RiveSprite? {
        check(!closeOnce.closed) { "Cannot send pointer event to a closed scene" }
        
        val hitSprite = hitTest(point)
        if (hitSprite != null) {
            hitSprite.pointerDown(point, pointerID)
        }
        return hitSprite
    }

    /**
     * Send a pointer move event to a specific sprite or the topmost sprite at the point.
     *
     * If [targetSprite] is provided, the event is sent to that sprite regardless of
     * hit testing. This is useful for drag operations where you want to track the
     * pointer even when it moves outside the sprite's bounds.
     *
     * If [targetSprite] is null, the event is sent to the topmost sprite at the point.
     *
     * ## Usage for Drag
     *
     * ```kotlin
     * var dragTarget: RiveSprite? = null
     *
     * Canvas(modifier = Modifier
     *     .fillMaxSize()
     *     .pointerInput(Unit) {
     *         detectDragGestures(
     *             onDragStart = { offset ->
     *                 dragTarget = scene.pointerDown(offset)
     *             },
     *             onDrag = { change, _ ->
     *                 scene.pointerMove(change.position, targetSprite = dragTarget)
     *             },
     *             onDragEnd = {
     *                 dragTarget?.pointerUp(...)
     *                 dragTarget = null
     *             }
     *         )
     *     }
     * )
     * ```
     *
     * @param point The pointer position in DrawScope coordinates.
     * @param pointerID The ID of the pointer (for multi-touch), defaults to 0.
     * @param targetSprite If provided, send the event to this sprite. Otherwise, hit test.
     * @return The sprite that received the event, or null if no sprite was hit/targeted.
     */
    fun pointerMove(point: Offset, pointerID: Int = 0, targetSprite: RiveSprite? = null): RiveSprite? {
        check(!closeOnce.closed) { "Cannot send pointer event to a closed scene" }
        
        val sprite = targetSprite ?: hitTest(point)
        sprite?.pointerMove(point, pointerID)
        return sprite
    }

    /**
     * Send a pointer up event to a specific sprite or the topmost sprite at the point.
     *
     * If [targetSprite] is provided, the event is sent to that sprite regardless of
     * hit testing. This is useful for completing drag operations or ensuring the
     * up event goes to the same sprite that received the down event.
     *
     * @param point The pointer position in DrawScope coordinates.
     * @param pointerID The ID of the pointer (for multi-touch), defaults to 0.
     * @param targetSprite If provided, send the event to this sprite. Otherwise, hit test.
     * @return The sprite that received the event, or null if no sprite was hit/targeted.
     */
    fun pointerUp(point: Offset, pointerID: Int = 0, targetSprite: RiveSprite? = null): RiveSprite? {
        check(!closeOnce.closed) { "Cannot send pointer event to a closed scene" }
        
        val sprite = targetSprite ?: hitTest(point)
        sprite?.pointerUp(point, pointerID)
        return sprite
    }

    /**
     * Send a pointer exit event to a sprite.
     *
     * This should be called when a pointer that was previously over a sprite
     * moves away. Use this to properly trigger hover-out transitions in Rive.
     *
     * @param sprite The sprite to send the exit event to.
     * @param pointerID The ID of the pointer (for multi-touch), defaults to 0.
     */
    fun pointerExit(sprite: RiveSprite, pointerID: Int = 0) {
        check(!closeOnce.closed) { "Cannot send pointer event to a closed scene" }
        sprite.pointerExit(pointerID)
    }

    // endregion

    // region Rendering Support

    /**
     * Get all visible sprites sorted by z-index for rendering.
     *
     * Sprites with lower z-index values are rendered first (at the back),
     * and sprites with higher z-index values are rendered last (on top).
     *
     * Sprites with the same z-index maintain their relative insertion order
     * (stable sort).
     *
     * @return A list of visible sprites sorted by z-index ascending.
     */
    fun getSortedSprites(): List<RiveSprite> {
        return _sprites
            .filter { it.isVisible }
            .sortedBy { it.zIndex }
    }

    /**
     * Get all sprites (including invisible) sorted by z-index.
     *
     * @return A list of all sprites sorted by z-index ascending.
     */
    fun getAllSortedSprites(): List<RiveSprite> {
        return _sprites.sortedBy { it.zIndex }
    }

    // endregion

    // region Lifecycle

    /**
     * Check if the scene has been closed.
     */
    val isClosed: Boolean
        get() = closeOnce.closed

    /**
     * Close the scene and release all resources.
     *
     * This closes all sprites and releases the command queue reference.
     * The scene cannot be used after calling this method.
     */
    override fun close() {
        closeOnce.close()
    }

    // endregion
}
