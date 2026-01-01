/**
 * RiveNative - Android implementation
 * 
 * This file provides the Android-specific implementation of the RiveNative interface,
 * including native library loading and JNI method declarations.
 */
package app.rive.mp

/**
 * Android implementation of RiveNative.
 * 
 * The native library (libmprive-android.so) is automatically loaded when this
 * object is first accessed. The library loading triggers JNI_OnLoad in the
 * native code, which performs initial setup.
 */
actual object RiveNative {
    
    /**
     * Load the native library.
     * 
     * This is called automatically when the object is first accessed.
     * The library name "mprive-android" corresponds to "libmprive-android.so"
     * which is built by CMake and packaged with the APK.
     */
    init {
        try {
            System.loadLibrary("mprive-android")
            // JNI_OnLoad is automatically called after this point
        } catch (e: UnsatisfiedLinkError) {
            // Log error and rethrow
            // Note: Can't use native logging since library failed to load
            System.err.println("Failed to load mprive-android native library: ${e.message}")
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
     * @param context Android Context (typically Application context)
     */
    actual external fun nativeInit(context: Any)
    
    /**
     * Get the native library version string.
     * 
     * Native signature (from bindings_init.cpp):
     *   JNIEXPORT jstring JNICALL
     *   Java_app_rive_mp_RiveNative_nativeGetVersion(JNIEnv* env, jobject thiz)
     * 
     * @return Version string (e.g., "mprive-1.0.0-android")
     */
    actual external fun nativeGetVersion(): String
    
    /**
     * Get detailed platform information.
     * 
     * Native signature (from bindings_init.cpp):
     *   JNIEXPORT jstring JNICALL
     *   Java_app_rive_mp_RiveNative_nativeGetPlatformInfo(JNIEnv* env, jobject thiz)
     * 
     * @return Platform information string (e.g., "Android (JNI, Mobile)")
     */
    actual external fun nativeGetPlatformInfo(): String
}
