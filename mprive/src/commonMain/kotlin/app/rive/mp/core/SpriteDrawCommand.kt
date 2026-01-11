package app.rive.mp.core

import app.rive.mp.ArtboardHandle
import app.rive.mp.StateMachineHandle

/**
 * A command for drawing a sprite as part of a batch rendering operation.
 *
 * Used with [CommandQueue.drawMultiple] for efficient batch rendering of sprites.
 * Each command specifies an artboard/state machine pair along with a transform
 * to apply during rendering.
 *
 * The transform is a 6-element affine transform in the format:
 * `[scaleX, skewY, skewX, scaleY, translateX, translateY]`
 *
 * This corresponds to the matrix:
 * ```
 * | scaleX  skewX   translateX |
 * | skewY   scaleY  translateY |
 * | 0       0       1          |
 * ```
 *
 * @param artboardHandle The handle of the artboard to draw.
 * @param stateMachineHandle The handle of the state machine associated with the artboard.
 * @param transform 6-element affine transform array.
 * @param artboardWidth The width of the artboard in pixels (used for scaling calculations).
 * @param artboardHeight The height of the artboard in pixels (used for scaling calculations).
 */
data class SpriteDrawCommand(
    var artboardHandle: ArtboardHandle,
    var stateMachineHandle: StateMachineHandle,
    val transform: FloatArray,  // Keep as val - it is a reference to pre-allocated buffer
    var artboardWidth: Float,
    var artboardHeight: Float,
) {
    init {
        require(transform.size == 6) {
            "Transform must have exactly 6 elements, got ${transform.size}"
        }
    }

    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (other !is SpriteDrawCommand) return false
        return artboardHandle == other.artboardHandle &&
               stateMachineHandle == other.stateMachineHandle &&
               transform.contentEquals(other.transform) &&
               artboardWidth == other.artboardWidth &&
               artboardHeight == other.artboardHeight
    }

    override fun hashCode(): Int {
        var result = artboardHandle.hashCode()
        result = 31 * result + stateMachineHandle.hashCode()
        result = 31 * result + transform.contentHashCode()
        result = 31 * result + artboardWidth.hashCode()
        result = 31 * result + artboardHeight.hashCode()
        return result
    }

    override fun toString(): String {
        return "SpriteDrawCommand(artboard=$artboardHandle, stateMachine=$stateMachineHandle, " +
               "size=${artboardWidth}x$artboardHeight, transform=[${transform.joinToString()}])"
    }
}
