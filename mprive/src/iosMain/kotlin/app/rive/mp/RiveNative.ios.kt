/**
 * RiveNative - iOS stub implementation
 * 
 * This is a stub implementation to allow the project to compile.
 * Full iOS support will be implemented in a future phase.
 */
package app.rive.mp

/**
 * iOS stub implementation of RiveNative.
 * 
 * This implementation throws NotImplementedError for all methods.
 * Full iOS support (with Kotlin/Native and PLS/Metal renderer) will be
 * implemented in Phase 4+ of the implementation plan.
 */
actual object RiveNative {
    
    /**
     * Initialize the native library.
     * 
     * @throws NotImplementedError iOS support not yet implemented
     */
    actual fun nativeInit(context: Any) {
        throw NotImplementedError("iOS support is not yet implemented. This will be added in a future release.")
    }
    
    /**
     * Get the native library version string.
     * 
     * @throws NotImplementedError iOS support not yet implemented
     */
    actual fun nativeGetVersion(): String {
        throw NotImplementedError("iOS support is not yet implemented. This will be added in a future release.")
    }
    
    /**
     * Get detailed platform information.
     * 
     * @throws NotImplementedError iOS support not yet implemented
     */
    actual fun nativeGetPlatformInfo(): String {
        throw NotImplementedError("iOS support is not yet implemented. This will be added in a future release.")
    }
}
