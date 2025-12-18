package app.rive.sprites

import android.graphics.Matrix
import androidx.annotation.ColorInt
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
import app.rive.RenderBuffer
import app.rive.RiveFile
import app.rive.RiveLog
import app.rive.StateMachine
import app.rive.ViewModelInstance
import app.rive.ViewModelSource
import app.rive.core.ArtboardHandle
import app.rive.core.CommandQueue
import app.rive.core.StateMachineHandle
import app.rive.runtime.kotlin.core.Alignment
import app.rive.runtime.kotlin.core.Fit
import app.rive.sprites.props.SpriteControlProp
import kotlinx.coroutines.flow.Flow
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
 * ## ViewModel Property Interaction
 *
 * Sprites can have a [ViewModelInstance] for data binding. Use the property methods
 * to interact with ViewModel properties:
 *
 * - [setNumber], [setString], [setBoolean], [setEnum], [setColor]: Set property values
 * - [fireTrigger]: Fire a trigger property
 * - [getNumberFlow], [getStringFlow], etc.: Observe property changes reactively
 *
 * ## Tagging for Batch Operations
 *
 * Sprites can be tagged for grouping and batch operations via [RiveSpriteScene]:
 * - [tags]: Set of tags assigned to this sprite
 * - [hasTag], [hasAnyTag], [hasAllTags]: Check tag membership
 *
 * ## Example
 *
 * ```kotlin
 * // Define property descriptors
 * object EnemyProps {
 *     val Health = SpriteControlProp.Number("health")
 *     val Attack = SpriteControlProp.Trigger("attack")
 * }
 *
 * // Create sprite with auto-binding
 * val enemy = scene.createSprite(
 *     file = enemyFile,
 *     stateMachineName = "combat",
 *     viewModelConfig = SpriteViewModelConfig.AutoBind,
 *     tags = setOf(SpriteTag("enemy"))
 * )
 *
 * // Set properties
 * enemy.setNumber(EnemyProps.Health, 100f)
 * enemy.fireTrigger(EnemyProps.Attack)
 *
 * // Or use path strings
 * enemy.setNumber("health", 50f)
 * enemy.fireTrigger("attack")
 * ```
 *
 * @param artboard The Rive artboard instance for this sprite.
 * @param stateMachine The Rive state machine instance for this sprite.
 * @param commandQueue The command queue for GPU operations.
 * @param viewModelInstance Optional ViewModelInstance for data binding.
 * @param initialTags Initial tags for grouping this sprite.
 * @param ownsViewModelInstance Whether this sprite owns (and should close) the VMI.
 */
class RiveSprite internal constructor(
    val artboard: Artboard,
    val stateMachine: StateMachine,
    internal val commandQueue: CommandQueue,
    private val viewModelInstance: ViewModelInstance? = null,
    initialTags: Set<SpriteTag> = emptySet(),
    private val ownsViewModelInstance: Boolean = true,
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

    // region Tags for Grouping

    /**
     * Tags assigned to this sprite for grouping and batch operations.
     *
     * Tags can be used with [RiveSpriteScene] batch methods to perform
     * operations on multiple sprites at once.
     *
     * @see SpriteTag
     */
    var tags: Set<SpriteTag> by mutableStateOf(initialTags)

    /**
     * Check if this sprite has a specific tag.
     *
     * @param tag The tag to check for
     * @return True if this sprite has the tag
     */
    fun hasTag(tag: SpriteTag): Boolean = tags.contains(tag)

    /**
     * Check if this sprite has any of the given tags.
     *
     * @param checkTags The tags to check for
     * @return True if this sprite has at least one of the tags
     */
    fun hasAnyTag(checkTags: List<SpriteTag>): Boolean = checkTags.any { hasTag(it) }

    /**
     * Check if this sprite has all of the given tags.
     *
     * @param checkTags The tags to check for
     * @return True if this sprite has all of the tags
     */
    fun hasAllTags(checkTags: List<SpriteTag>): Boolean = checkTags.all { hasTag(it) }

    // endregion

    // region Render Buffer Cache

    /**
     * Cached render buffer for this sprite.
     * The buffer is reused across frames to avoid expensive GPU resource allocation.
     * Only recreated when the sprite's effective size changes.
     */
    private var cachedRenderBuffer: RenderBuffer? = null
    private var cachedBufferWidth: Int = 0
    private var cachedBufferHeight: Int = 0

    /**
     * Get or create a render buffer for this sprite.
     *
     * The buffer is cached and reused across frames. It is only recreated when
     * the sprite's effective size changes.
     *
     * @return A render buffer sized to this sprite's effective size.
     */
    internal fun getOrCreateRenderBuffer(): RenderBuffer {
        val currentSize = effectiveSize
        val requiredWidth = currentSize.width.toInt().coerceAtLeast(1)
        val requiredHeight = currentSize.height.toInt().coerceAtLeast(1)

        val existingBuffer = cachedRenderBuffer
        if (existingBuffer != null &&
            cachedBufferWidth == requiredWidth &&
            cachedBufferHeight == requiredHeight
        ) {
            return existingBuffer
        }

        // Size changed or no buffer exists - create a new one
        existingBuffer?.close()

        RiveLog.d(RIVE_SPRITE_TAG) {
            "Creating render buffer ${requiredWidth}x$requiredHeight " +
                "(was ${cachedBufferWidth}x$cachedBufferHeight)"
        }

        val newBuffer = RenderBuffer(requiredWidth, requiredHeight, commandQueue)
        cachedRenderBuffer = newBuffer
        cachedBufferWidth = requiredWidth
        cachedBufferHeight = requiredHeight
        return newBuffer
    }

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

    /**
     * Whether this sprite has a ViewModelInstance for data binding.
     */
    val hasViewModel: Boolean
        get() = viewModelInstance != null

    // endregion

    // region ViewModel Property Methods

    /**
     * Set a number property value on the ViewModelInstance.
     *
     * If the sprite has no ViewModelInstance or the property doesn't exist,
     * this method does nothing (optionally logs a warning if [logPropertyWarnings] is true).
     *
     * @param prop The property descriptor
     * @param value The value to set
     */
    fun setNumber(prop: SpriteControlProp.Number, value: Float) {
        setNumber(prop.name, value)
    }

    /**
     * Set a number property value by path.
     *
     * @param propertyPath The path to the property (slash-delimited for nested)
     * @param value The value to set
     */
    fun setNumber(propertyPath: String, value: Float) {
        viewModelInstance?.setNumber(propertyPath, value)
            ?: logPropertyWarning("setNumber", propertyPath)
    }

    /**
     * Get a Flow of number property values for reactive observation.
     *
     * @param prop The property descriptor
     * @return A Flow of Float values, or null if no ViewModelInstance
     */
    fun getNumberFlow(prop: SpriteControlProp.Number): Flow<Float>? =
        viewModelInstance?.getNumberFlow(prop.name)

    /**
     * Get a Flow of number property values by path.
     *
     * @param propertyPath The path to the property
     * @return A Flow of Float values, or null if no ViewModelInstance
     */
    fun getNumberFlow(propertyPath: String): Flow<Float>? =
        viewModelInstance?.getNumberFlow(propertyPath)

    /**
     * Set a string property value on the ViewModelInstance.
     *
     * @param prop The property descriptor
     * @param value The value to set
     */
    fun setString(prop: SpriteControlProp.Text, value: String) {
        setString(prop.name, value)
    }

    /**
     * Set a string property value by path.
     *
     * @param propertyPath The path to the property
     * @param value The value to set
     */
    fun setString(propertyPath: String, value: String) {
        viewModelInstance?.setString(propertyPath, value)
            ?: logPropertyWarning("setString", propertyPath)
    }

    /**
     * Get a Flow of string property values for reactive observation.
     *
     * @param prop The property descriptor
     * @return A Flow of String values, or null if no ViewModelInstance
     */
    fun getStringFlow(prop: SpriteControlProp.Text): Flow<String>? =
        viewModelInstance?.getStringFlow(prop.name)

    /**
     * Get a Flow of string property values by path.
     *
     * @param propertyPath The path to the property
     * @return A Flow of String values, or null if no ViewModelInstance
     */
    fun getStringFlow(propertyPath: String): Flow<String>? =
        viewModelInstance?.getStringFlow(propertyPath)

    /**
     * Set a boolean property value on the ViewModelInstance.
     *
     * @param prop The property descriptor
     * @param value The value to set
     */
    fun setBoolean(prop: SpriteControlProp.Toggle, value: Boolean) {
        setBoolean(prop.name, value)
    }

    /**
     * Set a boolean property value by path.
     *
     * @param propertyPath The path to the property
     * @param value The value to set
     */
    fun setBoolean(propertyPath: String, value: Boolean) {
        viewModelInstance?.setBoolean(propertyPath, value)
            ?: logPropertyWarning("setBoolean", propertyPath)
    }

    /**
     * Get a Flow of boolean property values for reactive observation.
     *
     * @param prop The property descriptor
     * @return A Flow of Boolean values, or null if no ViewModelInstance
     */
    fun getBooleanFlow(prop: SpriteControlProp.Toggle): Flow<Boolean>? =
        viewModelInstance?.getBooleanFlow(prop.name)

    /**
     * Get a Flow of boolean property values by path.
     *
     * @param propertyPath The path to the property
     * @return A Flow of Boolean values, or null if no ViewModelInstance
     */
    fun getBooleanFlow(propertyPath: String): Flow<Boolean>? =
        viewModelInstance?.getBooleanFlow(propertyPath)

    /**
     * Set an enum property value on the ViewModelInstance.
     *
     * @param prop The property descriptor
     * @param value The enum value as a string
     */
    fun setEnum(prop: SpriteControlProp.Choice, value: String) {
        setEnum(prop.name, value)
    }

    /**
     * Set an enum property value by path.
     *
     * @param propertyPath The path to the property
     * @param value The enum value as a string
     */
    fun setEnum(propertyPath: String, value: String) {
        viewModelInstance?.setEnum(propertyPath, value)
            ?: logPropertyWarning("setEnum", propertyPath)
    }

    /**
     * Get a Flow of enum property values for reactive observation.
     *
     * @param prop The property descriptor
     * @return A Flow of String values, or null if no ViewModelInstance
     */
    fun getEnumFlow(prop: SpriteControlProp.Choice): Flow<String>? =
        viewModelInstance?.getEnumFlow(prop.name)

    /**
     * Get a Flow of enum property values by path.
     *
     * @param propertyPath The path to the property
     * @return A Flow of String values, or null if no ViewModelInstance
     */
    fun getEnumFlow(propertyPath: String): Flow<String>? =
        viewModelInstance?.getEnumFlow(propertyPath)

    /**
     * Set a color property value on the ViewModelInstance.
     *
     * @param prop The property descriptor
     * @param value The color as ARGB integer
     */
    fun setColor(prop: SpriteControlProp.Color, @ColorInt value: Int) {
        setColor(prop.name, value)
    }

    /**
     * Set a color property value by path.
     *
     * @param propertyPath The path to the property
     * @param value The color as ARGB integer
     */
    fun setColor(propertyPath: String, @ColorInt value: Int) {
        viewModelInstance?.setColor(propertyPath, value)
            ?: logPropertyWarning("setColor", propertyPath)
    }

    /**
     * Get a Flow of color property values for reactive observation.
     *
     * @param prop The property descriptor
     * @return A Flow of Int (ARGB) values, or null if no ViewModelInstance
     */
    fun getColorFlow(prop: SpriteControlProp.Color): Flow<Int>? =
        viewModelInstance?.getColorFlow(prop.name)

    /**
     * Get a Flow of color property values by path.
     *
     * @param propertyPath The path to the property
     * @return A Flow of Int (ARGB) values, or null if no ViewModelInstance
     */
    fun getColorFlow(propertyPath: String): Flow<Int>? =
        viewModelInstance?.getColorFlow(propertyPath)

    /**
     * Fire a trigger property on the ViewModelInstance.
     *
     * Triggers are one-shot events that cause state transitions without
     * maintaining a value. Use for events like "attack", "jump", "die".
     *
     * @param prop The trigger property descriptor
     */
    fun fireTrigger(prop: SpriteControlProp.Trigger) {
        fireTrigger(prop.name)
    }

    /**
     * Fire a trigger property by path.
     *
     * @param propertyPath The path to the trigger property
     */
    fun fireTrigger(propertyPath: String) {
        viewModelInstance?.fireTrigger(propertyPath)
            ?: logPropertyWarning("fireTrigger", propertyPath)
    }

    /**
     * Get a Flow of trigger events for reactive observation.
     *
     * @param prop The trigger property descriptor
     * @return A Flow of Unit values (emits when trigger fires), or null if no ViewModelInstance
     */
    fun getTriggerFlow(prop: SpriteControlProp.Trigger): Flow<Unit>? =
        viewModelInstance?.getTriggerFlow(prop.name)

    /**
     * Get a Flow of trigger events by path.
     *
     * @param propertyPath The path to the trigger property
     * @return A Flow of Unit values, or null if no ViewModelInstance
     */
    fun getTriggerFlow(propertyPath: String): Flow<Unit>? =
        viewModelInstance?.getTriggerFlow(propertyPath)

    private fun logPropertyWarning(operation: String, propertyPath: String) {
        if (logPropertyWarnings) {
            RiveLog.w(RIVE_SPRITE_TAG) {
                "$operation('$propertyPath') skipped: no ViewModelInstance"
            }
        }
    }

    // endregion

    // region Animation Control

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

    // region Pointer Events

    /**
     * Send a pointer down event to the state machine.
     *
     * This is used for interactive Rive animations that respond to touch/click.
     * The point should be in DrawScope coordinates (same as [position]).
     *
     * The method transforms the point from DrawScope coordinates to artboard
     * coordinates before forwarding to the state machine.
     *
     * @param point The pointer position in DrawScope coordinates.
     * @param pointerID The ID of the pointer (for multi-touch), defaults to 0.
     * @return True if the point is within the sprite bounds and the event was sent.
     */
    fun pointerDown(point: Offset, pointerID: Int = 0): Boolean {
        val localPoint = transformPointToLocal(point) ?: return false
        val bounds = getArtboardBounds()

        if (!bounds.contains(localPoint)) {
            return false
        }

        // Forward to state machine using artboard coordinates
        // Use FILL fit and artboard size as surface size so coords pass through unchanged
        val displaySize = effectiveSize
        commandQueue.pointerDown(
            stateMachineHandle = stateMachineHandle,
            fit = Fit.FILL,
            alignment = Alignment.CENTER,
            surfaceWidth = displaySize.width,
            surfaceHeight = displaySize.height,
            pointerID = pointerID,
            pointerX = localPoint.x,
            pointerY = localPoint.y
        )
        return true
    }

    /**
     * Send a pointer move event to the state machine.
     *
     * This is used for hover effects and drag interactions in Rive animations.
     * The point should be in DrawScope coordinates (same as [position]).
     *
     * The method transforms the point from DrawScope coordinates to artboard
     * coordinates before forwarding to the state machine.
     *
     * @param point The pointer position in DrawScope coordinates.
     * @param pointerID The ID of the pointer (for multi-touch), defaults to 0.
     * @return True if the event was sent (even if outside bounds, for proper hover exit handling).
     */
    fun pointerMove(point: Offset, pointerID: Int = 0): Boolean {
        val localPoint = transformPointToLocal(point) ?: return false

        // Always send move events - the state machine handles in/out transitions
        val displaySize = effectiveSize
        commandQueue.pointerMove(
            stateMachineHandle = stateMachineHandle,
            fit = Fit.FILL,
            alignment = Alignment.CENTER,
            surfaceWidth = displaySize.width,
            surfaceHeight = displaySize.height,
            pointerID = pointerID,
            pointerX = localPoint.x,
            pointerY = localPoint.y
        )
        return true
    }

    /**
     * Send a pointer up event to the state machine.
     *
     * This is used to complete click/tap interactions in Rive animations.
     * The point should be in DrawScope coordinates (same as [position]).
     *
     * The method transforms the point from DrawScope coordinates to artboard
     * coordinates before forwarding to the state machine.
     *
     * @param point The pointer position in DrawScope coordinates.
     * @param pointerID The ID of the pointer (for multi-touch), defaults to 0.
     * @return True if the event was sent.
     */
    fun pointerUp(point: Offset, pointerID: Int = 0): Boolean {
        val localPoint = transformPointToLocal(point) ?: return false

        val displaySize = effectiveSize
        commandQueue.pointerUp(
            stateMachineHandle = stateMachineHandle,
            fit = Fit.FILL,
            alignment = Alignment.CENTER,
            surfaceWidth = displaySize.width,
            surfaceHeight = displaySize.height,
            pointerID = pointerID,
            pointerX = localPoint.x,
            pointerY = localPoint.y
        )
        return true
    }

    /**
     * Send a pointer exit event to the state machine.
     *
     * This is used when the pointer leaves the sprite's area. Call this when
     * a pointer that was previously over this sprite moves away.
     *
     * @param pointerID The ID of the pointer (for multi-touch), defaults to 0.
     */
    fun pointerExit(pointerID: Int = 0) {
        val displaySize = effectiveSize
        // Use a point outside the bounds to signal exit
        commandQueue.pointerExit(
            stateMachineHandle = stateMachineHandle,
            fit = Fit.FILL,
            alignment = Alignment.CENTER,
            surfaceWidth = displaySize.width,
            surfaceHeight = displaySize.height,
            pointerID = pointerID,
            pointerX = -1f,
            pointerY = -1f
        )
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
     * This closes the cached render buffer, artboard, state machine,
     * and optionally the ViewModelInstance.
     * The sprite should not be used after calling close.
     */
    override fun close() {
        // Close cached render buffer first
        cachedRenderBuffer?.close()
        cachedRenderBuffer = null
        cachedBufferWidth = 0
        cachedBufferHeight = 0

        // Close ViewModelInstance if we own it
        if (ownsViewModelInstance) {
            viewModelInstance?.close()
        }

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
         * Global setting to log property operation warnings.
         *
         * When true, warns when property operations are skipped because
         * the sprite has no ViewModelInstance. Useful during development.
         */
        var logPropertyWarnings: Boolean = false

        /**
         * Create a new [RiveSprite] from a [RiveFile].
         *
         * @param file The Rive file to create the sprite from.
         * @param artboardName The name of the artboard to use, or null for the default.
         * @param stateMachineName The name of the state machine to use, or null for the default.
         * @param viewModelConfig Configuration for ViewModelInstance creation.
         * @param tags Initial tags for grouping this sprite.
         * @return A new [RiveSprite] instance.
         */
        fun fromFile(
            file: RiveFile,
            artboardName: String? = null,
            stateMachineName: String? = null,
            viewModelConfig: SpriteViewModelConfig = SpriteViewModelConfig.None,
            tags: Set<SpriteTag> = emptySet()
        ): RiveSprite {
            val artboard = Artboard.fromFile(file, artboardName)
            val stateMachine = StateMachine.fromArtboard(artboard, stateMachineName)

            // Create ViewModelInstance based on config
            val (vmi, ownsVmi) = createViewModelInstance(file, artboard, viewModelConfig)

            // Bind VMI to state machine if available
            if (vmi != null) {
                try {
                    file.commandQueue.bindViewModelInstance(
                        stateMachine.stateMachineHandle,
                        vmi.instanceHandle
                    )
                } catch (e: Exception) {
                    RiveLog.w(RIVE_SPRITE_TAG) {
                        "Failed to bind ViewModelInstance: ${e.message}"
                    }
                }
            }

            return RiveSprite(
                artboard = artboard,
                stateMachine = stateMachine,
                commandQueue = file.commandQueue,
                viewModelInstance = vmi,
                initialTags = tags,
                ownsViewModelInstance = ownsVmi
            )
        }

        private fun createViewModelInstance(
            file: RiveFile,
            artboard: Artboard,
            config: SpriteViewModelConfig
        ): Pair<ViewModelInstance?, Boolean> {
            return when (config) {
                is SpriteViewModelConfig.None -> {
                    null to false
                }

                is SpriteViewModelConfig.AutoBind -> {
                    // Try to create default VMI for artboard
                    try {
                        val vmi = ViewModelInstance.fromFile(
                            file,
                            ViewModelSource.DefaultForArtboard(artboard).defaultInstance()
                        )
                        vmi to true
                    } catch (e: Exception) {
                        RiveLog.d(RIVE_SPRITE_TAG) {
                            "No default ViewModel for artboard: ${e.message}"
                        }
                        null to false
                    }
                }

                is SpriteViewModelConfig.Named -> {
                    val vmi = ViewModelInstance.fromFile(
                        file,
                        ViewModelSource.Named(config.viewModelName).defaultInstance()
                    )
                    vmi to true
                }

                is SpriteViewModelConfig.NamedInstance -> {
                    val vmi = ViewModelInstance.fromFile(
                        file,
                        ViewModelSource.Named(config.viewModelName)
                            .namedInstance(config.instanceName)
                    )
                    vmi to true
                }

                is SpriteViewModelConfig.External -> {
                    // External VMI - we don't own it
                    config.instance to false
                }
            }
        }
    }
}
