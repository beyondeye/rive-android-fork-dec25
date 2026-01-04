package app.rive.mp.core

/**
 * Defines how content should be fit into its container.
 * 
 * This enum is used for artboard rendering to specify how the artboard should be scaled
 * and positioned within its rendering bounds.
 */
enum class Fit {
    /**
     * Fill the available space, stretching content if necessary to fill both dimensions.
     */
    FILL,
    
    /**
     * Contain the content within the available space, maintaining aspect ratio.
     */
    CONTAIN,
    
    /**
     * Cover the available space, maintaining aspect ratio and cropping if necessary.
     */
    COVER,
    
    /**
     * Fit the content to the width, maintaining aspect ratio.
     */
    FIT_WIDTH,
    
    /**
     * Fit the content to the height, maintaining aspect ratio.
     */
    FIT_HEIGHT,
    
    /**
     * Do not scale the content.
     */
    NONE,
    
    /**
     * Scale down to fit if content is larger, otherwise use original size.
     */
    SCALE_DOWN,
    
    /**
     * Use the layout fit from the artboard.
     */
    LAYOUT;

    companion object {
        /**
         * Returns the [Fit] associated to [index].
         *
         * @throws IllegalArgumentException If the index is out of bounds.
         */
        fun fromIndex(index: Int): Fit {
            val maxIndex = entries.size
            if (index < 0 || index > maxIndex) {
                throw IndexOutOfBoundsException("Invalid Fit index value $index. It must be between 0 and $maxIndex")
            }

            return entries[index]
        }
    }
}
