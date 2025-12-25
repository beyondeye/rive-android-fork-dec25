package app.rive.mprive

/**
 * Wasm/JS implementation of the Rive platform interface.
 * This is a stub implementation - native code support will be added later.
 */
actual class RivePlatform actual constructor() {
    actual fun getPlatformName(): String {
        return "Wasm/JS"
    }
    
    actual fun initialize(): Boolean {
        // TODO: Initialize Wasm/JS-specific Rive runtime
        // This will require compiling Rive C++ to WebAssembly using Emscripten
        println("RivePlatform.initialize() called on Wasm/JS - not yet implemented")
        return true
    }
}
