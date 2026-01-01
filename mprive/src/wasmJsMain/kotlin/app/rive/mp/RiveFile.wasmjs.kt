/**
 * RiveFile - wasmJS stub implementation
 * 
 * This is a stub implementation to allow the project to compile.
 * Full WebAssembly support will be implemented in a future phase.
 */
package app.rive.mp

/**
 * wasmJS stub implementation of RiveFile.
 * 
 * This implementation throws NotImplementedError for all methods.
 * Full WebAssembly support (with PLS/WebGL or SkiaWasm renderer) will be
 * implemented in Phase 4+ of the implementation plan.
 */
actual class RiveFile private constructor() {
    
    actual val artboardCount: Int
        get() = throw NotImplementedError("WebAssembly support is not yet implemented. This will be added in a future release.")
    
    actual fun artboard(): Artboard {
        throw NotImplementedError("WebAssembly support is not yet implemented. This will be added in a future release.")
    }
    
    actual fun artboard(index: Int): Artboard {
        throw NotImplementedError("WebAssembly support is not yet implemented. This will be added in a future release.")
    }
    
    actual fun artboard(name: String): Artboard {
        throw NotImplementedError("WebAssembly support is not yet implemented. This will be added in a future release.")
    }
    
    actual fun dispose() {
        throw NotImplementedError("WebAssembly support is not yet implemented. This will be added in a future release.")
    }
    
    actual companion object {
        actual fun load(bytes: ByteArray): RiveFile {
            throw NotImplementedError("WebAssembly support is not yet implemented. This will be added in a future release.")
        }
    }
}
