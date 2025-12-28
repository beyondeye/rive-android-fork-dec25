package app.rive.mp.sprites

import androidx.compose.runtime.Immutable

/**
 * Defines the origin (pivot point) for a sprite's transformations.
 *
 * The origin determines the point around which the sprite is positioned, scaled, and rotated.
 * Pivot values are normalized where (0, 0) represents the top-left corner and (1, 1) represents
 * the bottom-right corner of the sprite.
 *
 * ## Use Cases
 *
 * - **[Center]**: Best for characters, projectiles, and objects that rotate around their center.
 *   When positioned at (100, 100), the sprite's center will be at that coordinate.
 *
 * - **[TopLeft]**: Best for UI elements, tiles, and grid-based layouts.
 *   When positioned at (100, 100), the sprite's top-left corner will be at that coordinate.
 *
 * - **[Custom]**: For specific pivot points like character feet, weapon handles, or door hinges.
 *   For example, `Custom(0.5f, 1.0f)` places the pivot at the bottom-center (feet position).
 */
@Immutable
sealed class SpriteOrigin {
    /**
     * The normalized X coordinate of the pivot point (0 = left, 1 = right).
     */
    abstract val pivotX: Float

    /**
     * The normalized Y coordinate of the pivot point (0 = top, 1 = bottom).
     */
    abstract val pivotY: Float

    /**
     * Pivot at the center of the sprite (0.5, 0.5).
     *
     * Use this for characters, projectiles, rotating objects, or any sprite that should
     * be positioned by its center point.
     */
    data object Center : SpriteOrigin() {
        override val pivotX: Float = 0.5f
        override val pivotY: Float = 0.5f
    }

    /**
     * Pivot at the top-left corner of the sprite (0, 0).
     *
     * Use this for UI elements, tiles, grid-based layouts, or when you need
     * position to represent the top-left corner.
     */
    data object TopLeft : SpriteOrigin() {
        override val pivotX: Float = 0f
        override val pivotY: Float = 0f
    }

    /**
     * Custom pivot point with user-defined coordinates.
     *
     * Pivot values are normalized:
     * - (0, 0) = top-left corner
     * - (0.5, 0.5) = center
     * - (1, 1) = bottom-right corner
     *
     * ## Examples
     *
     * - `Custom(0.5f, 1.0f)` - Bottom-center, useful for character feet positioning
     * - `Custom(0.0f, 0.5f)` - Left-center, useful for side-scrolling alignment
     * - `Custom(0.0f, 0.5f)` - Left edge, useful for door hinge rotation
     * - `Custom(0.25f, 0.75f)` - Any arbitrary pivot point
     *
     * @param pivotX The normalized X coordinate (0 = left, 1 = right)
     * @param pivotY The normalized Y coordinate (0 = top, 1 = bottom)
     */
    data class Custom(
        override val pivotX: Float,
        override val pivotY: Float
    ) : SpriteOrigin() {
        init {
            require(pivotX in 0f..1f) { "pivotX must be between 0 and 1, was $pivotX" }
            require(pivotY in 0f..1f) { "pivotY must be between 0 and 1, was $pivotY" }
        }
    }
}
