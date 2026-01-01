/**
 * RiveFile - Common interface for Rive file management
 * 
 * Represents a loaded Rive (.riv) file containing one or more artboards.
 * 
 * Usage:
 * ```kotlin
 * val bytes = context.resources.openRawResource(R.raw.my_animation).readBytes()
 * val file = RiveFile.load(bytes)
 * 
 * // Get artboard
 * val artboard = file.artboard() // Get default artboard
 * // or
 * val artboard = file.artboard(0) // Get by index
 * // or
 * val artboard = file.artboard("MyArtboard") // Get by name
 * 
 * // Don't forget to dispose
 * file.dispose()
 * ```
 */
package app.rive.mp

/**
 * Represents a Rive file containing artboards and animations.
 * 
 * RiveFile instances must be disposed when no longer needed by calling [dispose].
 */
expect class RiveFile {
    
    /**
     * Number of artboards in this file.
     */
    val artboardCount: Int
    
    /**
     * Get the default artboard (usually the first artboard or the one marked as default).
     * 
     * @return Artboard instance, or throws exception if unavailable
     */
    fun artboard(): Artboard
    
    /**
     * Get an artboard by index.
     * 
     * @param index Artboard index (0-based)
     * @return Artboard instance, or throws exception if index is out of bounds
     */
    fun artboard(index: Int): Artboard
    
    /**
     * Get an artboard by name.
     * 
     * @param name Artboard name
     * @return Artboard instance, or throws exception if not found
     */
    fun artboard(name: String): Artboard
    
    /**
     * Release native resources.
     * 
     * After calling this method, the RiveFile instance should not be used.
     * This method is idempotent - it's safe to call multiple times.
     */
    fun dispose()
    
    companion object {
        /**
         * Load a Rive file from a byte array.
         * 
         * @param bytes Byte array containing the .riv file data
         * @return RiveFile instance
         * @throws RiveException if the file cannot be loaded
         */
        fun load(bytes: ByteArray): RiveFile
    }
}

/**
 * Exception thrown by Rive operations.
 */
class RiveException(message: String, cause: Throwable? = null) : Exception(message, cause)
