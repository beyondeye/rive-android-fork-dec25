package app.rive.sprites

import android.graphics.Matrix
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.geometry.isSpecified
import app.rive.Artboard
import app.rive.RiveFile
import app.rive.StateMachine
import app.rive.core.ArtboardHandle
import app.rive.core.CommandQueue
import app.rive.core.StateMachineHandle
import kotlin.time.Duration

private const val RIVE_SPRITE_TAG = "Rive/Sprite"

/**
 * A Rive sprite that can be positioned, scaled, and rotated within a [RiveSpriteScene].
 *
 * Each sprite wraps a Rive [Artboard] and [StateMachine], providing transform properties
 * for game engine integration. Sprites are rendered in a single GPU pass by the scene,
 * sorted by [zIndex].
 *
 * ## Transform Properties
 *
 * - [position]: The location of the sprite's origin in DrawScope coordinates
 * - [size]: The display size in DrawScope units (artboard is scaled to fit)
 * - [scale]: Additional scaling applied after size (supports non-uniform scaling)
 * - [rotation]: Rotation in degrees around the origin
 * - [origin]: The pivot point for transformations (default: center)
 * - [zIndex]: Rendering order (higher values render on top)
 *
 * ## State Machine Interaction
 *
 * - [fire]: Trigger a state machine input
 * - [setBoolean]: Set a boolean input value
 * - [setNumber]: Set a number input value
 * - [advance]: Advance the state machine by a time delta
 *
 * ## Example
 *
 * ```kotlin
 * val sprite = scene.createSprite(riveFile, stateMachine = "combat")
 * sprite.position = Offset(100f, 200f)
 * sprite.size = Size(64f, 64f)
 * sprite.rotation = 45f
 * sprite.fire("attack")
 * ```
 *
 * @param artboard The Rive artboard instance for this sprite.
 * @param stateMachine The Rive state machine instance for this sprite.
 * @param commandQueue The command queue for GPU operations.
 */
class RiveSprite internal constructor(
    val artboard: Artboard,
    val stateMachine: StateMachine,
    internal val commandQueue: CommandQueue,
) : AutoCloseable {

    // region Transform Properties

    /**
     * The position of the sprite's origin in DrawScope coordinates.
     *
     * The exact point represented by this position depends on [origin]:
     * - [SpriteOrigin.Center]: Position is the center of the sprite
     * - [SpriteOrigin.TopLeft]: Position is the top-left corner
     * - [SpriteOrigin.Custom]: Position is the custom pivot point
     */
    var position: Offset by mutableStateOf(Offset.Zero)

    /**
     * The display size of the sprite in DrawScope units.
     *
     * The artboard will be scaled to fit within this size. If not set,
     * defaults to the artboard's natural size.
     */
    var size: Size by mutableStateOf(Size.Unspecified)

    /**
     * Additional scale factor applied to the sprite.
     *
     * This scaling is applied after [size], allowing for dynamic scaling effects
     * like pulsing, growing, or shrinking animations.
     *
     * Supports non-uniform scaling with independent X and Y factors:
     * - `SpriteScale(2f)` - Uniform 2x scaling
     * - `SpriteScale(2f, 1f)` - Stretch horizontally only
     * - `SpriteScale.Unscaled` - No additional scaling (default)
     */
    var scale: SpriteScale by mutableStateOf(SpriteScale.Unscaled)

    /**
     * Rotation of the sprite in degrees, clockwise.
     *
     * The rotation is applied around the [origin] point.
     */
    var rotation: Float by mutableFloatStateOf(0f)

    /**
     * The origin (pivot point) for positioning, scaling, and rotation.
     *
     * Determines which point of the sprite is placed at [position] and
     * serves as the center for [rotation] and [scale] transformations.
     *
     * @see SpriteOrigin
     */
    var origin: SpriteOrigin by mutableStateOf(SpriteOrigin.Center)

    /**
     * The z-index for rendering order.
     *
     * Sprites with higher z-index values are rendered on top of sprites
     * with lower values. Sprites with the same z-index are rendered in
     * insertion order.
     */
    var zIndex: Int by mutableIntStateOf(0)

    /**
     * Whether the sprite is visible and should be rendered.
     *
     * Invisible sprites are skipped during rendering but still maintain
     * their state and can receive [advance] calls.
     */
    var isVisible: Boolean by mutableStateOf(true)

    // endregion

    // region Computed Properties

    /**
     * The artboard handle for native operations.
     */
    val artboardHandle: ArtboardHandle
        get() = artboard.artboardHandle

    /**
     * The state machine handle for native operations.
     */
    val stateMachineHandle: StateMachineHandle
        get() = stateMachine.stateMachineHandle

    /**
     * The effective display size, using artboard size if [size] is unspecified.
     *
     * Note: This requires artboard dimensions from native code. For now,
     * returns the specified size or a default. Full implementation will
     * query artboard dimensions.
     */
    val effectiveSize: Size
        get() = if (size.isSpecified) size else DEFAULT_SPRITE_SIZE

    // endregion

    // region State Machine Methods

    /**
     * Fire a trigger input on the state machine.
     *
     * Triggers are one-shot inputs that cause state transitions without
     * maintaining a value. Use this for events like "jump", "attack", "die".
     *
     * @param triggerName The name of the trigger input as defined in the Rive editor.
     * @throws IllegalArgumentException If the trigger name doesn't exist.
     */
    fun fire(triggerName: String) {
        // TODO: Implement when CommandQueue.fireStateMachineTrigger is added
        // For now, this is a placeholder that will be implemented in Phase 2
        // commandQueue.fireStateMachineTrigger(stateMachineHandle, triggerName)
    }

    /**
     * Set a boolean input value on the state machine.
     *
     * Boolean inputs are used for on/off states like "isRunning", "isGrounded",
     * "hasWeapon".
     *
     * @param name The name of the boolean input as defined in the Rive editor.
     * @param value The boolean value to set.
     * @throws IllegalArgumentException If the input name doesn't exist or isn't a boolean.
     */
    fun setBoolean(name: String, value: Boolean) {
        // TODO: Implement when CommandQueue.setStateMachineBoolean is added
        // For now, this is a placeholder that will be implemented in Phase 2
        // commandQueue.setStateMachineBoolean(stateMachineHandle, name, value)
    }

    /**
     * Set a number input value on the state machine.
     *
     * Number inputs are used for continuous values like "speed", "health",
     * "direction".
     *
     * @param name The name of the number input as defined in the Rive editor.
     * @param value The float value to set.
     * @throws IllegalArgumentException If the input name doesn't exist or isn't a number.
     */
    fun setNumber(name: String, value: Float) {
        // TODO: Implement when CommandQueue.setStateMachineNumber is added
        // For now, this is a placeholder that will be implemented in Phase 2
        // commandQueue.setStateMachineNumber(stateMachineHandle, name, value)
    }

    /**
     * Advance the state machine by the given time delta.
     *
     * This progresses all animations and state transitions. Should be called
     * once per frame with the time elapsed since the last frame.
     *
     * @param deltaTime The time to advance by.
     */
    fun advance(deltaTime: Duration) {
        stateMachine.advance(deltaTime)
    }

    // endregion

    // region Transform Methods

    /**
     * Compute the transformation matrix for rendering this sprite.
     *
     * The matrix combines translation, rotation, and scale in the correct order:
     * 1. Translate to position
     * 2. Rotate around origin
     * 3. Scale around origin
     * 4. Translate by origin offset (to position the pivot correctly)
     *
     * @return A [Matrix] representing the full transformation.
     */
    fun computeTransformMatrix(): Matrix {
        val matrix = Matrix()

        val displaySize = effectiveSize
        val pivotX = origin.pivotX * displaySize.width * scale.scaleX
        val pivotY = origin.pivotY * displaySize.height * scale.scaleY

        // Apply transformations in order:
        // 1. Translate to position
        matrix.postTranslate(position.x, position.y)

        // 2. Rotate around the position (which is the pivot point)
        if (rotation != 0f) {
            matrix.postRotate(rotation, position.x, position.y)
        }

        // 3. Scale around the position
        if (scale != SpriteScale.Unscaled) {
            matrix.postScale(scale.scaleX, scale.scaleY, position.x, position.y)
        }

        // 4. Offset by pivot to position correctly
        matrix.postTranslate(-pivotX, -pivotY)

        return matrix
    }

    /**
     * Compute the 6-element affine transform array for native rendering.
     *
     * The array format is [scaleX, skewY, skewX, scaleY, translateX, translateY],
     * compatible with the native sprite batch rendering.
     *
     * @return A FloatArray with 6 elements representing the affine transform.
     */
    fun computeTransformArray(): FloatArray {
        val matrix = computeTransformMatrix()
        val values = FloatArray(9)
        matrix.getValues(values)

        // Matrix values are in row-major order:
        // [scaleX, skewX, transX]
        // [skewY, scaleY, transY]
        // [persp0, persp1, persp2]
        return floatArrayOf(
            values[Matrix.MSCALE_X],  // a: scaleX
            values[Matrix.MSKEW_Y],   // b: skewY
            values[Matrix.MSKEW_X],   // c: skewX
            values[Matrix.MSCALE_Y],  // d: scaleY
            values[Matrix.MTRANS_X],  // tx: translateX
            values[Matrix.MTRANS_Y]   // ty: translateY
        )
    }

    /**
     * Get the axis-aligned bounding box of the sprite in DrawScope coordinates.
     *
     * This accounts for all transformations and returns the smallest rectangle
     * that fully contains the transformed sprite.
     *
     * @return The bounding [Rect] in DrawScope coordinates.
     */
    fun getBounds(): Rect {
        val displaySize = effectiveSize
        val matrix = computeTransformMatrix()

        // Create the four corners of the untransformed sprite
        val corners = floatArrayOf(
            0f, 0f,                                    // Top-left
            displaySize.width, 0f,                     // Top-right
            displaySize.width, displaySize.height,    // Bottom-right
            0f, displaySize.height                    // Bottom-left
        )

        // Transform all corners
        matrix.mapPoints(corners)

        // Find the bounding box
        var minX = corners[0]
        var maxX = corners[0]
        var minY = corners[1]
        var maxY = corners[1]

        for (i in corners.indices step 2) {
            val x = corners[i]
            val y = corners[i + 1]
            if (x < minX) minX = x
            if (x > maxX) maxX = x
            if (y < minY) minY = y
            if (y > maxY) maxY = y
        }

        return Rect(minX, minY, maxX, maxY)
    }

    /**
     * Get the artboard bounds in local (untransformed) coordinates.
     *
     * Used internally for hit testing to determine if a point falls within
     * the sprite's artboard area.
     *
     * @return The artboard [Rect] with origin at (0, 0).
     */
    internal fun getArtboardBounds(): Rect {
        val displaySize = effectiveSize
        return Rect(0f, 0f, displaySize.width, displaySize.height)
    }

    /**
     * Transform a point from DrawScope coordinates to local sprite coordinates.
     *
     * Used for hit testing - transforms a touch point to see if it falls
     * within the sprite's bounds.
     *
     * @param point The point in DrawScope coordinates.
     * @return The point in local sprite coordinates, or null if transform fails.
     */
    internal fun transformPointToLocal(point: Offset): Offset? {
        val matrix = computeTransformMatrix()
        val inverseMatrix = Matrix()

        if (!matrix.invert(inverseMatrix)) {
            return null
        }

        val pts = floatArrayOf(point.x, point.y)
        inverseMatrix.mapPoints(pts)

        return Offset(pts[0], pts[1])
    }

    /**
     * Check if a point in DrawScope coordinates hits this sprite.
     *
     * @param point The point to test in DrawScope coordinates.
     * @return True if the point is within the sprite's bounds.
     */
    fun hitTest(point: Offset): Boolean {
        if (!isVisible) return false

        val localPoint = transformPointToLocal(point) ?: return false
        val bounds = getArtboardBounds()

        return bounds.contains(localPoint)
    }

    // endregion

    // region Lifecycle

    /**
     * Release the resources held by this sprite.
     *
     * This closes the underlying artboard and state machine. The sprite
     * should not be used after calling close.
     */
    override fun close() {
        stateMachine.close()
        artboard.close()
    }

    // endregion

    companion object {
        /**
         * Default size used when size is unspecified and artboard size is unknown.
         */
        private val DEFAULT_SPRITE_SIZE = Size(100f, 100f)

        /**
         * Create a new [RiveSprite] from a [RiveFile].
         *
         * @param file The Rive file to create the sprite from.
         * @param artboardName The name of the artboard to use, or null for the default.
         * @param stateMachineName The name of the state machine to use, or null for the default.
         * @return A new [RiveSprite] instance.
         */
        fun fromFile(
            file: RiveFile,
            artboardName: String? = null,
            stateMachineName: String? = null
        ): RiveSprite {
            val artboard = Artboard.fromFile(file, artboardName)
            val stateMachine = StateMachine.fromArtboard(artboard, stateMachineName)

            return RiveSprite(
                artboard = artboard,
                stateMachine = stateMachine,
                commandQueue = file.commandQueue
            )
        }
    }
}
