package app.rive.mp.sprites

import androidx.compose.runtime.Immutable

/**
 * Represents the scale of a sprite with independent X and Y scale factors.
 *
 * Similar to [androidx.compose.ui.layout.ScaleFactor], this allows non-uniform scaling
 * where the sprite can be stretched or squashed in either dimension independently.
 *
 * @param scaleX The horizontal scale factor. 1.0 means no scaling.
 * @param scaleY The vertical scale factor. 1.0 means no scaling.
 */
@Immutable
data class SpriteScale(val scaleX: Float, val scaleY: Float) {
    companion object {
        /**
         * No scaling applied - represents the original size (1x in both dimensions).
         */
        val Unscaled = SpriteScale(1f, 1f)
    }

    /**
     * Returns true if this scale represents uniform scaling (scaleX == scaleY).
     */
    val isUniform: Boolean
        get() = scaleX == scaleY

    /**
     * Returns the uniform scale value if [isUniform] is true, otherwise returns the average.
     */
    val uniformScale: Float
        get() = (scaleX + scaleY) / 2f

    operator fun times(other: SpriteScale): SpriteScale =
        SpriteScale(scaleX * other.scaleX, scaleY * other.scaleY)

    operator fun times(factor: Float): SpriteScale =
        SpriteScale(scaleX * factor, scaleY * factor)

    operator fun div(factor: Float): SpriteScale =
        SpriteScale(scaleX / factor, scaleY / factor)
}

/**
 * Creates a [SpriteScale] with uniform scaling in both dimensions.
 *
 * @param scale The uniform scale factor to apply to both X and Y dimensions.
 * @return A [SpriteScale] with equal scaleX and scaleY values.
 */
fun SpriteScale(scale: Float): SpriteScale = SpriteScale(scale, scale)
