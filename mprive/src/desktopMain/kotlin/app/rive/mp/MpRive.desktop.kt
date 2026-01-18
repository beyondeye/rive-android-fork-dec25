/**
 * Desktop implementation of MpRive initialization.
 */
package app.rive.mp

/**
 * Desktop implementation of [MpRive].
 *
 * On Desktop (JVM), initialization:
 * 1. Loads the native library (libmprive-desktop.so)
 * 2. Initializes the JNI environment
 *
 * @see MpRive
 */
actual object MpRive {
    @Volatile
    private var _isInitialized = false

    actual val isInitialized: Boolean
        get() = _isInitialized

    /**
     * Initialize the Rive runtime for Desktop.
     *
     * @param context Not required for Desktop. Can be null or any value.
     * @throws IllegalStateException if native library loading fails
     */
    actual fun init(context: Any?) {
        if (_isInitialized) return

        synchronized(this) {
            if (_isInitialized) return

            // Access RiveNative to trigger native library loading
            // This causes System.loadLibrary("mprive-desktop") to be called in RiveNative.init
            RiveNative

            // Desktop doesn't need context for class loader, but call nativeInit for consistency
            // Pass Unit as a placeholder context
            RiveNative.nativeInit(Unit)

            _isInitialized = true

            RiveLog.i("MpRive") { "Rive initialized successfully for Desktop" }
        }
    }
}