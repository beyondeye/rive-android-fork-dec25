/**
 * RiveFile - iOS stub implementation
 * 
 * This is a stub implementation to allow the project to compile.
 * Full iOS support will be implemented in a future phase.
 */
package app.rive.mp

/**
 * iOS stub implementation of RiveFile.
 * 
 * This implementation throws NotImplementedError for all methods.
 * Full iOS support (with PLS/Metal or Skia renderer) will be
 * implemented in Phase 4+ of the implementation plan.
 */
actual class RiveFile private constructor() {
    
    actual val artboardCount: Int
        get() = throw NotImplementedError("iOS support is not yet implemented. This will be added in a future release.")
    
    actual fun artboard(): Artboard {
        throw NotImplementedError("iOS support is not yet implemented. This will be added in a future release.")
    }
    
    actual fun artboard(index: Int): Artboard {
        throw NotImplementedError("iOS support is not yet implemented. This will be added in a future release.")
    }
    
    actual fun artboard(name: String): Artboard {
        throw NotImplementedError("iOS support is not yet implemented. This will be added in a future release.")
    }
    
    actual fun dispose() {
        throw NotImplementedError("iOS support is not yet implemented. This will be added in a future release.")
    }
    
    actual companion object {
        actual fun load(bytes: ByteArray): RiveFile {
            throw NotImplementedError("iOS support is not yet implemented. This will be added in a future release.")
        }
    }
}
