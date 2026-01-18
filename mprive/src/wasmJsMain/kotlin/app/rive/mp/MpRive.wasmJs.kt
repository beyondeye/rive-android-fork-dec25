/**
 * wasm/js implementation of MpRive initialization.
 */
package app.rive.mp

/**
 * wasm/js implementation of [MpRive].
 *
 * On wasm/js, the Rive WebAssembly module is loaded by the JavaScript runtime.
 * No explicit runtime initialization is required from Kotlin, but this stub
 * provides API consistency.
 *
 * @see MpRive
 */
actual object MpRive {
    private var _isInitialized = false

    actual val isInitialized: Boolean
        get() = _isInitialized

    /**
     * Initialize the Rive runtime for wasm/js.
     *
     * On wasm/js, the WebAssembly module is loaded by the JavaScript runtime.
     * This method exists for API consistency across platforms. It marks the
     * library as initialized but performs no actual runtime loading.
     *
     * @param context Not required for wasm/js. Can be null or any value.
     */
    actual fun init(context: Any?) {
        if (_isInitialized) return

        // WebAssembly module is loaded by the JS runtime
        // No explicit initialization needed from Kotlin
        _isInitialized = true

        // TODO: When wasm/js native implementation is ready, call actual initialization here
        // For now, this is a stub that just marks as initialized
    }
}