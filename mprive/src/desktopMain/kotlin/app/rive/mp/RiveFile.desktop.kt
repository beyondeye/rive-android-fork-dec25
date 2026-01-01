/**
 * RiveFile - Desktop implementation
 * 
 * Desktop-specific implementation using JNI to native Rive runtime.
 */
package app.rive.mp

/**
 * Desktop implementation of RiveFile.
 * 
 * Manages native Rive file resources via JNI bindings.
 * Implementation is identical to Android since both use JNI.
 */
actual class RiveFile private constructor(private var nativePtr: Long) {
    
    /**
     * Number of artboards in this file.
     */
    actual val artboardCount: Int
        get() {
            checkNotDisposed()
            return nativeGetArtboardCount(nativePtr)
        }
    
    /**
     * Get the default artboard.
     */
    actual fun artboard(): Artboard {
        checkNotDisposed()
        val artboardPtr = nativeGetDefaultArtboard(nativePtr)
        if (artboardPtr == 0L) {
            throw RiveException("Failed to get default artboard")
        }
        return Artboard(artboardPtr)
    }
    
    /**
     * Get an artboard by index.
     */
    actual fun artboard(index: Int): Artboard {
        checkNotDisposed()
        val artboardPtr = nativeGetArtboard(nativePtr, index)
        if (artboardPtr == 0L) {
            throw RiveException("Failed to get artboard at index $index")
        }
        return Artboard(artboardPtr)
    }
    
    /**
     * Get an artboard by name.
     */
    actual fun artboard(name: String): Artboard {
        checkNotDisposed()
        val artboardPtr = nativeGetArtboardByName(nativePtr, name)
        if (artboardPtr == 0L) {
            throw RiveException("Artboard not found: $name")
        }
        return Artboard(artboardPtr)
    }
    
    /**
     * Release native resources.
     */
    actual fun dispose() {
        if (nativePtr != 0L) {
            nativeRelease(nativePtr)
            nativePtr = 0L
        }
    }
    
    /**
     * Check if this file has been disposed.
     */
    private fun checkNotDisposed() {
        if (nativePtr == 0L) {
            throw IllegalStateException("RiveFile has been disposed")
        }
    }
    
    /**
     * Ensure resources are released when garbage collected.
     */
    protected fun finalize() {
        dispose()
    }
    
    actual companion object {
        /**
         * Load a Rive file from a byte array.
         */
        actual fun load(bytes: ByteArray): RiveFile {
            val nativePtr = nativeLoadFile(bytes)
            if (nativePtr == 0L) {
                throw RiveException("Failed to load Rive file")
            }
            return RiveFile(nativePtr)
        }
        
        // Native methods
        @JvmStatic
        private external fun nativeLoadFile(bytes: ByteArray): Long
    }
    
    // Instance native methods
    private external fun nativeGetArtboardCount(filePtr: Long): Int
    private external fun nativeGetArtboard(filePtr: Long, index: Int): Long
    private external fun nativeGetArtboardByName(filePtr: Long, name: String): Long
    private external fun nativeGetDefaultArtboard(filePtr: Long): Long
    private external fun nativeRelease(filePtr: Long)
}
