/**
 * Multiplatform Rive initialization.
 *
 * This object provides explicit initialization for the Rive runtime across all platforms.
 * Initialization must be called before using any Rive functionality.
 *
 * ## Usage
 *
 * ### Android
 * Call in your Application.onCreate() or Activity.onCreate() before using Rive:
 * ```kotlin
 * class MainActivity : ComponentActivity() {
 *     override fun onCreate(savedInstanceState: Bundle?) {
 *         super.onCreate(savedInstanceState)
 *         MpRive.init(applicationContext)
 *         // Now you can use Rive
 *     }
 * }
 * ```
 *
 * ### Desktop
 * Call at application startup:
 * ```kotlin
 * fun main() {
 *     MpRive.init()
 *     // Now you can use Rive
 * }
 * ```
 *
 * ## What initialization does
 *
 * ### Android
 * 1. **Loads native library** - Triggers System.loadLibrary("mprive-android")
 * 2. **Stores JVM pointer** - Required for getting JNIEnv from any thread
 * 3. **Initializes logging** - Sets up native logging system
 * 4. **Initializes JNI ClassLoader** - Critical for finding Java classes from
 *    non-main threads (like render threads). Without this, callbacks from
 *    the render thread to Kotlin would fail.
 *
 * ### Desktop
 * 1. **Loads native library** - Triggers System.loadLibrary("mprive-desktop")
 * 2. **Stores JVM pointer** - Required for getting JNIEnv from any thread
 * 3. **Initializes logging** - Sets up native logging system
 *
 * ### iOS (future)
 * Framework is linked at compile time, no runtime initialization needed.
 *
 * ### wasm/js (future)
 * WebAssembly module is loaded by the JS runtime.
 */
package app.rive.mp

/**
 * Multiplatform Rive initialization object.
 *
 * @see init
 * @see isInitialized
 */
expect object MpRive {
    /**
     * Initialize the Rive runtime.
     *
     * Must be called before using any Rive functionality. It is safe to call
     * this multiple times - subsequent calls are no-ops.
     *
     * @param context Platform-specific context:
     *   - **Android**: `android.content.Context` (Application or Activity context).
     *     Required for initializing the JNI ClassLoader which enables callbacks
     *     from native threads to Kotlin.
     *   - **Desktop**: `null` (no context needed). You can pass any value.
     *   - **iOS**: `null` (no context needed). Framework is linked at compile time.
     *   - **wasm/js**: `null` (no context needed). Module loaded by JS runtime.
     *
     * @throws IllegalStateException if initialization fails
     * @throws IllegalArgumentException if Android is passed a non-Context object
     */
    fun init(context: Any? = null)

    /**
     * Returns true if Rive has been initialized via [init].
     *
     * Use this to check if initialization is needed, or to verify that
     * initialization completed successfully.
     */
    val isInitialized: Boolean
}