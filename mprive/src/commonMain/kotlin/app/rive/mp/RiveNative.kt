/**
 * RiveNative - Native library initialization and platform information.
 * 
 * This object provides the Kotlin interface to the native JNI initialization
 * methods implemented in bindings_init.cpp.
 * 
 * Usage:
 * ```kotlin
 * // Initialize the native library (optional - JNI_OnLoad runs automatically)
 * RiveNative.nativeInit(context)
 * 
 * // Get version information
 * val version = RiveNative.nativeGetVersion()
 * println("Native library version: $version")
 * 
 * // Get platform information
 * val platform = RiveNative.nativeGetPlatformInfo()
 * println("Platform: $platform")
 * ```
 */
package app.rive.mp

/**
 * Native library interface for mprive.
 * 
 * This object provides access to native initialization and platform information
 * methods. The native library is automatically loaded when this object is first
 * accessed.
 */
expect object RiveNative {
    /**
     * Initialize the native library.
     * 
     * This method performs explicit initialization of the native library,
     * including setting up the JNI class loader for cross-thread class access.
     * 
     * While JNI_OnLoad handles basic initialization automatically, this method
     * should be called early in your application lifecycle to ensure proper
     * setup, especially for multi-threaded scenarios.
     * 
     * @param context Platform-specific context object (Android Context, etc.)
     *                Can be Any type to support different platforms.
     */
    fun nativeInit(context: Any)
    
    /**
     * Get the native library version string.
     * 
     * Returns a version string in the format: "mprive-<version>-<platform>"
     * For example: "mprive-1.0.0-android" or "mprive-1.0.0-linux"
     * 
     * Useful for debugging and ensuring the correct native library is loaded.
     * 
     * @return Version string
     */
    fun nativeGetVersion(): String
    
    /**
     * Get detailed platform information.
     * 
     * Returns information about the current platform, including:
     * - Platform name (Android, iOS, Linux, etc.)
     * - Technology (JNI or No JNI)
     * - Category (Mobile, Desktop, Web)
     * 
     * For example: "Android (JNI, Mobile)" or "Linux (JNI, Desktop)"
     * 
     * Useful for runtime platform detection and debugging.
     * 
     * @return Platform information string
     */
    fun nativeGetPlatformInfo(): String
}
