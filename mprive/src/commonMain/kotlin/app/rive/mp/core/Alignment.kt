package app.rive.mp.core

/**
 * Defines how content should be aligned within its container.
 * 
 * This enum is used for artboard rendering to specify how the artboard should be positioned
 * within its rendering bounds when using fit modes that maintain aspect ratio.
 */
enum class Alignment {
    /**
     * Align to the top-left corner.
     */
    TOP_LEFT,
    
    /**
     * Align to the top-center.
     */
    TOP_CENTER,
    
    /**
     * Align to the top-right corner.
     */
    TOP_RIGHT,
    
    /**
     * Align to the center-left.
     */
    CENTER_LEFT,
    
    /**
     * Align to the center (both horizontally and vertically).
     */
    CENTER,
    
    /**
     * Align to the center-right.
     */
    CENTER_RIGHT,
    
    /**
     * Align to the bottom-left corner.
     */
    BOTTOM_LEFT,
    
    /**
     * Align to the bottom-center.
     */
    BOTTOM_CENTER,
    
    /**
     * Align to the bottom-right corner.
     */
    BOTTOM_RIGHT;

    companion object {
        /**
         * Returns the [Alignment] associated to [index].
         *
         * @throws IllegalArgumentException If the index is out of bounds.
         */
        fun fromIndex(index: Int): Alignment {
            val maxIndex = entries.size
            if (index < 0 || index > maxIndex) {
                throw IndexOutOfBoundsException("Invalid Alignment index value $index. It must be between 0 and $maxIndex")
            }

            return entries[index]
        }
    }
}
