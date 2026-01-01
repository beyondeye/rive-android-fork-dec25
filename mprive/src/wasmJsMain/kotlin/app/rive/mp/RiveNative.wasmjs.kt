/**
 * RiveNative - wasmJS stub implementation
 * 
 * This is a stub implementation to allow the project to compile.
 * Full WebAssembly support will be implemented in a future phase.
 */
package app.rive.mp

/**
 * wasmJS stub implementation of RiveNative.
 * 
 * This implementation throws NotImplementedError for all methods.
 * Full WebAssembly support (with PLS/WebGL or SkiaWasm renderer) will be
 * implemented in Phase 4+ of the implementation plan.
 */
actual object RiveNative {
    
    /**
     * Initialize the native library.
     * 
     * @throws NotImplementedError WebAssembly support not yet implemented
     */
    actual fun nativeInit(context: Any) {
        throw NotImplementedError("WebAssembly support is not yet implemented. This will be added in a future release.")
    }
    
    /**
     * Get the native library version string.
     * 
     * @throws NotImplementedError WebAssembly support not yet implemented
     */
    actual fun nativeGetVersion(): String {
        throw NotImplementedError("WebAssembly support is not yet implemented. This will be added in a future release.")
    }
    
    /**
     * Get detailed platform information.
     * 
     * @throws NotImplementedError WebAssembly support not yet implemented
     */
    actual fun nativeGetPlatformInfo(): String {
        throw NotImplementedError("WebAssembly support is not yet implemented. This will be added in a future release.")
    }
}
