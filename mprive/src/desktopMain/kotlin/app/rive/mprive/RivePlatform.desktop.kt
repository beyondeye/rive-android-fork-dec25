package app.rive.mprive

/**
 * Desktop JVM implementation of the Rive platform interface.
 */
actual class RivePlatform actual constructor() {
    actual fun getPlatformName(): String {
        val osName = System.getProperty("os.name")
        return "Desktop ($osName)"
    }
    
    actual fun initialize(): Boolean {
        // TODO: Initialize Desktop-specific Rive runtime via JNI
        // Similar to Android, but for desktop platforms
        return true
    }
    
    // Native methods to be implemented in C++ (similar to Android)
    private external fun nativeInit(): Boolean
    
    companion object {
        init {
            try {
                // Load the native library
                // TODO: Implement proper library loading for desktop platforms
                // System.loadLibrary("mprive-desktop")
            } catch (e: UnsatisfiedLinkError) {
                // Library not yet built - this is expected in skeleton
                System.err.println("Native library not loaded: ${e.message}")
            }
        }
    }
}
