/**
 * Artboard - Android implementation
 * 
 * Minimal placeholder implementation for Step 2.2.
 * Full implementation will be added in Step 2.3.
 */
package app.rive.mp

/**
 * Android implementation of Artboard.
 * 
 * This is a minimal placeholder. Full implementation (bounds, advance, animations, etc.)
 * will be added in Step 2.3: Artboard bindings.
 */
actual class Artboard internal constructor(private var nativePtr: Long) {
    
    /**
     * Release native resources.
     */
    actual fun dispose() {
        if (nativePtr != 0L) {
            // TODO: Implement nativeRelease in Step 2.3
            // nativeRelease(nativePtr)
            nativePtr = 0L
        }
    }
    
    /**
     * Ensure resources are released when garbage collected.
     */
    protected fun finalize() {
        dispose()
    }
}
