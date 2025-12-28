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
import app.rive.sprites.matrix.TransMatrixData
import app.rive.sprites.props.SpriteControlProp
import kotlinx.coroutines.flow.Flow
import kotlin.time.Duration

/**
 * Type alias for Android Matrix.
 * When moving to multiplatform, this will be replaced with expect/actual.
 */
typealias TransMatrix = Matrix
abstract class IRiveSprite(initialTags: Set<SpriteTag>) : AutoCloseable {
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
     * The effective display size, using artboard size if [size] is unspecified.
     *
     * Note: This requires artboard dimensions from native code. For now,
     * returns the specified size or a default. Full implementation will
     * query artboard dimensions.
     */
    val effectiveSize: Size
        get() = if (size.isSpecified) size else DEFAULT_SPRITE_SIZE
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

    // region ViewModel Property Methods

    /**
     * Set a number property value on the ViewModelInstance.
     *
     * This method automatically uses the appropriate mechanism:
     * - If the sprite has a ViewModelInstance, it uses ViewModel property binding
     * - If no ViewModelInstance, it falls back to legacy SMI (state machine input) mechanism
     *
     * This allows the same API to work with both modern Rive files (with ViewModels)
     * and legacy Rive files (without ViewModels, using direct state machine inputs).
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
     * This method automatically uses the appropriate mechanism:
     * - If the sprite has a ViewModelInstance, it uses ViewModel property binding
     * - If no ViewModelInstance, it falls back to legacy SMI (state machine input) mechanism
     *
     * @param propertyPath The path to the property (slash-delimited for nested) or input name
     * @param value The value to set
     */
    abstract fun setNumber(propertyPath: String, value: Float)

    /**
     * Get a Flow of number property values for reactive observation.
     *
     * @param prop The property descriptor
     * @return A Flow of Float values, or null if no ViewModelInstance
     */
    abstract fun getNumberFlow(prop: SpriteControlProp.Number): Flow<Float>?

    /**
     * Get a Flow of number property values by path.
     *
     * @param propertyPath The path to the property
     * @return A Flow of Float values, or null if no ViewModelInstance
     */
    abstract fun getNumberFlow(propertyPath: String): Flow<Float>?

    /**
     * Set a string property value on the ViewModelInstance.
     *
     * Note: String properties are only available via ViewModelInstance.
     * Legacy Rive files without ViewModels do not support string inputs.
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
     * Note: String properties are only available via ViewModelInstance.
     * Legacy Rive files without ViewModels do not support string inputs.
     *
     * @param propertyPath The path to the property
     * @param value The value to set
     */
    abstract fun setString(propertyPath: String, value: String)

    /**
     * Get a Flow of string property values for reactive observation.
     *
     * @param prop The property descriptor
     * @return A Flow of String values, or null if no ViewModelInstance
     */
    abstract fun getStringFlow(prop: SpriteControlProp.Text): Flow<String>?

    /**
     * Get a Flow of string property values by path.
     *
     * @param propertyPath The path to the property
     * @return A Flow of String values, or null if no ViewModelInstance
     */
    abstract fun getStringFlow(propertyPath: String): Flow<String>?

    /**
     * Set a boolean property value on the ViewModelInstance.
     *
     * This method automatically uses the appropriate mechanism:
     * - If the sprite has a ViewModelInstance, it uses ViewModel property binding
     * - If no ViewModelInstance, it falls back to legacy SMI (state machine input) mechanism
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
     * This method automatically uses the appropriate mechanism:
     * - If the sprite has a ViewModelInstance, it uses ViewModel property binding
     * - If no ViewModelInstance, it falls back to legacy SMI (state machine input) mechanism
     *
     * @param propertyPath The path to the property (slash-delimited for nested) or input name
     * @param value The value to set
     */
    abstract fun setBoolean(propertyPath: String, value: Boolean)

    /**
     * Get a Flow of boolean property values for reactive observation.
     *
     * @param prop The property descriptor
     * @return A Flow of Boolean values, or null if no ViewModelInstance
     */
    abstract fun getBooleanFlow(prop: SpriteControlProp.Toggle): Flow<Boolean>?

    /**
     * Get a Flow of boolean property values by path.
     *
     * @param propertyPath The path to the property
     * @return A Flow of Boolean values, or null if no ViewModelInstance
     */
    abstract fun getBooleanFlow(propertyPath: String): Flow<Boolean>?

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
    abstract fun setEnum(propertyPath: String, value: String)

    /**
     * Get a Flow of enum property values for reactive observation.
     *
     * @param prop The property descriptor
     * @return A Flow of String values, or null if no ViewModelInstance
     */
    abstract fun getEnumFlow(prop: SpriteControlProp.Choice): Flow<String>?
    /**
     * Get a Flow of enum property values by path.
     *
     * @param propertyPath The path to the property
     * @return A Flow of String values, or null if no ViewModelInstance
     */
    abstract fun getEnumFlow(propertyPath: String): Flow<String>?

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
    abstract fun setColor(propertyPath: String, @ColorInt value: Int)

    /**
     * Get a Flow of color property values for reactive observation.
     *
     * @param prop The property descriptor
     * @return A Flow of Int (ARGB) values, or null if no ViewModelInstance
     */
    abstract fun getColorFlow(prop: SpriteControlProp.Color): Flow<Int>?

    /**
     * Get a Flow of color property values by path.
     *
     * @param propertyPath The path to the property
     * @return A Flow of Int (ARGB) values, or null if no ViewModelInstance
     */
    abstract fun getColorFlow(propertyPath: String): Flow<Int>?

    /**
     * Fire a trigger property on the ViewModelInstance.
     *
     * This method automatically uses the appropriate mechanism:
     * - If the sprite has a ViewModelInstance, it uses ViewModel property binding
     * - If no ViewModelInstance, it falls back to legacy SMI (state machine input) mechanism
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
     * This method automatically uses the appropriate mechanism:
     * - If the sprite has a ViewModelInstance, it uses ViewModel property binding
     * - If no ViewModelInstance, it falls back to legacy SMI (state machine input) mechanism
     *
     * @param propertyPath The path to the trigger property or input name
     */
    abstract fun fireTrigger(propertyPath: String)

    /**
     * Get a Flow of trigger events for reactive observation.
     *
     * @param prop The trigger property descriptor
     * @return A Flow of Unit values (emits when trigger fires), or null if no ViewModelInstance
     */
    abstract fun getTriggerFlow(prop: SpriteControlProp.Trigger): Flow<Unit>?

    /**
     * Get a Flow of trigger events by path.
     *
     * @param propertyPath The path to the trigger property
     * @return A Flow of Unit values, or null if no ViewModelInstance
     */
    abstract fun getTriggerFlow(propertyPath: String): Flow<Unit>?

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
    abstract fun advance(deltaTime: Duration)

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
    abstract fun pointerDown(point: Offset, pointerID: Int = 0): Boolean

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
    abstract fun pointerMove(point: Offset, pointerID: Int = 0): Boolean

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
    abstract fun pointerUp(point: Offset, pointerID: Int = 0): Boolean
    /**
     * Send a pointer exit event to the state machine.
     *
     * This is used when the pointer leaves the sprite's area. Call this when
     * a pointer that was previously over this sprite moves away.
     *
     * @param pointerID The ID of the pointer (for multi-touch), defaults to 0.
     */
    abstract fun pointerExit(pointerID: Int = 0)

    // endregion

    // region Transform Methods

    /**
     * Compute transform array directly into provided buffer (zero allocation).
     *
     * This is an optimized version of [computeTransformArray] that writes directly
     * into a pre-allocated buffer, avoiding repeated FloatArray allocations.
     *
     * The affine transformation is computed directly without creating a Matrix object,
     * which eliminates both object allocation and matrix operation overhead.
     *
     * @param outArray The output buffer to write the transform into. Must be size 6.
     */
    internal fun computeTransformArrayInto(outArray: FloatArray) {
        //TODO: remove require check here for optimization?
        require(outArray.size == 6) { "Transform array must be size 6, got ${outArray.size}" }

        val displaySize = effectiveSize
        val pivotX = origin.pivotX * displaySize.width
        val pivotY = origin.pivotY * displaySize.height

        val scaleX = scale.scaleX
        val scaleY = scale.scaleY

        if (rotation == 0f) {
            // Fast path for non-rotated sprites (common case)
            outArray[0] = scaleX
            outArray[1] = 0f
            outArray[2] = 0f
            outArray[3] = scaleY
            outArray[4] = position.x - pivotX * scaleX
            outArray[5] = position.y - pivotY * scaleY
        } else {
            // Full affine transform with rotation
            val radians = Math.toRadians(rotation.toDouble())
            val cos = kotlin.math.cos(radians).toFloat()
            val sin = kotlin.math.sin(radians).toFloat()

            // Affine matrix: [a, b, c, d, tx, ty]
            // Combines: translate(-pivot), scale, rotate, translate(position)
            outArray[0] = scaleX * cos           // a: scaleX
            outArray[1] = scaleY * sin           // b: skewY
            outArray[2] = -scaleX * sin          // c: skewX
            outArray[3] = scaleY * cos           // d: scaleY
            outArray[4] = position.x - (pivotX * scaleX * cos - pivotY * scaleX * sin)  // tx
            outArray[5] = position.y - (pivotX * scaleY * sin + pivotY * scaleY * cos)  // ty
        }
    }

    /**
     * Compute the transformation matrix for rendering this sprite.
     *
     * This method uses an optimized single-pass calculation to compute the affine
     * transformation directly into a 9-element matrix, avoiding multiple matrix
     * multiplications and intermediate allocations.
     *
     * The transformation combines translation, rotation, and scale in the correct order
     * for pivot-based transformations:
     * 1. Translate pivot to origin (so transforms happen around the pivot point)
     * 2. Scale around origin (which is now the pivot)
     * 3. Rotate around origin (which is now the pivot)
     * 4. Translate to final position
     *
     * This ensures that both rotation and scale happen around the sprite's
     * origin (pivot point), which can be Center, TopLeft, or Custom.
     *
     * @return A [TransMatrixData] representing the full transformation. Use [TransMatrixData.asMatrix]
     *         to access platform-specific matrix operations when needed.
     */
    fun computeTransformMatrixData(): TransMatrixData {
        /*

        val matrix = Matrix()

        val displaySize = effectiveSize
        // Pivot in LOCAL unscaled coordinates
        val pivotX = origin.pivotX * displaySize.width
        val pivotY = origin.pivotY * displaySize.height

        // Apply transformations in order:
        // 1. Translate pivot to origin (so transforms happen around pivot)
        matrix.postTranslate(-pivotX, -pivotY)

        // 2. Scale around origin (which is now the pivot)
        if (scale != SpriteScale.Unscaled) {
            matrix.postScale(scale.scaleX, scale.scaleY)
        }

        // 3. Rotate around origin (which is now the pivot)
        if (rotation != 0f) {
            matrix.postRotate(rotation)
        }

        // 4. Translate to final position
        matrix.postTranslate(position.x, position.y)
         */

        // Allocate final 3x3 matrix directly (single allocation)
        val matrix9 = FloatArray(9)

        val displaySize = effectiveSize
        val pivotX = origin.pivotX * displaySize.width
        val pivotY = origin.pivotY * displaySize.height

        val scaleX = scale.scaleX
        val scaleY = scale.scaleY

        if (rotation == 0f) {
            // Fast path for non-rotated sprites (common case)
            // Row 1: [scaleX, skewX, transX]
            matrix9[0] = scaleX
            matrix9[1] = 0f
            matrix9[2] = position.x - pivotX * scaleX
            // Row 2: [skewY, scaleY, transY]
            matrix9[3] = 0f
            matrix9[4] = scaleY
            matrix9[5] = position.y - pivotY * scaleY
            // Row 3: [persp0, persp1, persp2] - identity for 2D
            matrix9[6] = 0f
            matrix9[7] = 0f
            matrix9[8] = 1f
        } else {
            // Full affine transform with rotation
            val radians = Math.toRadians(rotation.toDouble())
            val cos = kotlin.math.cos(radians).toFloat()
            val sin = kotlin.math.sin(radians).toFloat()

            // Row 1: [scaleX * cos, -scaleX * sin, transX]
            matrix9[0] = scaleX * cos
            matrix9[1] = -scaleX * sin
            matrix9[2] = position.x - (pivotX * scaleX * cos - pivotY * scaleX * sin)
            // Row 2: [scaleY * sin, scaleY * cos, transY]
            matrix9[3] = scaleY * sin
            matrix9[4] = scaleY * cos
            matrix9[5] = position.y - (pivotX * scaleY * sin + pivotY * scaleY * cos)
            // Row 3: [persp0, persp1, persp2] - identity for 2D
            matrix9[6] = 0f
            matrix9[7] = 0f
            matrix9[8] = 1f
        }

        return TransMatrixData(matrix9)
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
        val transform = computeTransformMatrixData()

        // Create the four corners of the untransformed sprite
        val corners = floatArrayOf(
            0f, 0f,                                    // Top-left
            displaySize.width, 0f,                     // Top-right
            displaySize.width, displaySize.height,    // Bottom-right
            0f, displaySize.height                    // Bottom-left
        )

        // Transform all corners using lazy-loaded matrix
        transform.asMatrix().mapPoints(corners)

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
     * Transform a point from DrawScope coordinates to local sprite coordinates.
     *
     * Used for hit testing - transforms a touch point to see if it falls
     * within the sprite's bounds.
     *
     * @param point The point in DrawScope coordinates.
     * @return The point in local sprite coordinates, or null if transform fails.
     */
    internal fun transformPointToLocal(point: Offset): Offset? {
        val transform = computeTransformMatrixData()
        val matrix = transform.asMatrix()
        val inverseMatrix = TransMatrix()

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
    abstract fun hitTest(point: Offset): Boolean

    // endregion
    companion object {
        /**
         * Default size used when size is unspecified and artboard size is unknown.
         */
        private val DEFAULT_SPRITE_SIZE = Size(100f, 100f)
    }
}