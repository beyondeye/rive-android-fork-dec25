package app.rive.sprites

import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.snapshots.SnapshotStateList
import androidx.compose.ui.geometry.Offset
import app.rive.RiveFile
import app.rive.RiveLog
import app.rive.core.CloseOnce
import app.rive.core.CommandQueue
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
     * @return The created [RiveSprite], which is already added to the scene.
     */
    fun createSprite(
        file: RiveFile,
        artboardName: String? = null,
        stateMachineName: String? = null
    ): RiveSprite {
        check(!closeOnce.closed) { "Cannot create sprite on a closed scene" }
        
        val sprite = RiveSprite.fromFile(file, artboardName, stateMachineName)
        _sprites.add(sprite)
        
        RiveLog.d(SPRITE_SCENE_TAG) { 
            "Created sprite (artboard=${artboardName ?: "default"}, " +
            "stateMachine=${stateMachineName ?: "default"}), total=${_sprites.size}" 
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
