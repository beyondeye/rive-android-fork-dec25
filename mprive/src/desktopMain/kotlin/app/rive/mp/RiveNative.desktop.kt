/**
 * RiveNative - Desktop implementation
 * 
 * This file provides the Desktop-specific implementation of the RiveNative interface,
 * including native library loading and JNI method declarations.
 */
package app.rive.mp

/**
 * Desktop implementation of RiveNative.
 * 
 * The native library (libmprive-desktop.so) is automatically loaded when this
 * object is first accessed. The library loading triggers JNI_OnLoad in the
 * native code, which performs initial setup.
 * 
 * Note: The native library is precompiled and checked into source control at:
 *   mprive/src/desktopMain/resources/jnilibs/linux-x86-64/libmprive-desktop.so
 */
actual object RiveNative {
    
    /**
     * Load the native library.
     * 
     * This is called automatically when the object is first accessed.
     * The library name "mprive-desktop" corresponds to "libmprive-desktop.so"
     * which is precompiled and packaged with the application resources.
     */
    init {
        try {
            System.loadLibrary("mprive-desktop")
            // JNI_OnLoad is automatically called after this point
        } catch (e: UnsatisfiedLinkError) {
            // Log error and rethrow
            // Note: Can't use native logging since library failed to load
            System.err.println("Failed to load mprive-desktop native library: ${e.message}")
            System.err.println("Make sure libmprive-desktop.so is in java.library.path")
            System.err.println("java.library.path = ${System.getProperty("java.library.path")}")
            throw e
        }
    }
    
    /**
     * Initialize the native library.
     * 
     * Native signature (from bindings_init.cpp):
     *   JNIEXPORT void JNICALL
     *   Java_app_rive_mp_RiveNative_nativeInit(JNIEnv* env, jobject thiz, jobject context)
     * 
     * @param context Platform-specific context (can be any object for Desktop)
     */
    actual external fun nativeInit(context: Any)
    
    /**
     * Get the native library version string.
     * 
     * Native signature (from bindings_init.cpp):
     *   JNIEXPORT jstring JNICALL
     *   Java_app_rive_mp_RiveNative_nativeGetVersion(JNIEnv* env, jobject thiz)
     * 
     * @return Version string (e.g., "mprive-1.0.0-linux")
     */
    actual external fun nativeGetVersion(): String
    
    /**
     * Get detailed platform information.
     * 
     * Native signature (from bindings_init.cpp):
     *   JNIEXPORT jstring JNICALL
     *   Java_app_rive_mp_RiveNative_nativeGetPlatformInfo(JNIEnv* env, jobject thiz)
     * 
     * @return Platform information string (e.g., "Linux (JNI, Desktop)")
     */
    actual external fun nativeGetPlatformInfo(): String
}
