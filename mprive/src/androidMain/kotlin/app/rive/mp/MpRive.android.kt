/**
 * Android implementation of MpRive initialization.
 */
package app.rive.mp

import android.content.Context

/**
 * Android implementation of [MpRive].
 *
 * On Android, initialization:
 * 1. Loads the native library (libmprive-android.so)
 * 2. Initializes the JNI ClassLoader for cross-thread class loading
 *
 * @see MpRive
 */
actual object MpRive {
    @Volatile
    private var _isInitialized = false

    actual val isInitialized: Boolean
        get() = _isInitialized

    /**
     * Initialize the Rive runtime for Android.
     *
     * @param context Android Context (Application or Activity). Required for
     *        initializing the JNI ClassLoader which enables callbacks from
     *        native threads to Kotlin.
     * @throws IllegalArgumentException if context is null or not an Android Context
     * @throws IllegalStateException if native library loading fails
     */
    actual fun init(context: Any?) {
        if (_isInitialized) return

        synchronized(this) {
            if (_isInitialized) return

            // Android requires Context
            requireNotNull(context) {
                "Android requires a Context for initialization. " +
                    "Call MpRive.init(applicationContext) or MpRive.init(this) from an Activity."
            }
            require(context is Context) {
                "Android requires android.content.Context, got ${context::class.simpleName}. " +
                    "Call MpRive.init(applicationContext) or MpRive.init(this) from an Activity."
            }

            // Access RiveNative to trigger native library loading
            // This causes System.loadLibrary("mprive-android") to be called in RiveNative.init
            RiveNative

            // Initialize the JNI ClassLoader for cross-thread class loading
            // This is critical for callbacks from render threads to Kotlin
            RiveNative.nativeInit(context)

            _isInitialized = true

            RiveLog.i("MpRive") { "Rive initialized successfully for Android" }
        }
    }
}