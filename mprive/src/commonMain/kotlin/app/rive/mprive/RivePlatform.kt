package app.rive.mprive

/**
 * Platform-specific Rive runtime interface.
 * Each platform provides its own implementation.
 */
expect class RivePlatform() {
    /**
     * Returns the name of the current platform.
     */
    fun getPlatformName(): String
    
    /**
     * Initialize the Rive runtime for this platform.
     */
    fun initialize(): Boolean
}

/**
 * Get the current platform instance.
 */
fun getRivePlatform(): RivePlatform = RivePlatform()
