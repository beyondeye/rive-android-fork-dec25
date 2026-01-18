/**
 * iOS implementation of MpRive initialization.
 */
package app.rive.mp

/**
 * iOS implementation of [MpRive].
 *
 * On iOS, the Rive framework is linked at compile time via Kotlin/Native cinterop.
 * No runtime initialization is required, but this stub provides API consistency.
 *
 * @see MpRive
 */
actual object MpRive {
    @Volatile
    private var _isInitialized = false

    actual val isInitialized: Boolean
        get() = _isInitialized

    /**
     * Initialize the Rive runtime for iOS.
     *
     * On iOS, the framework is linked at compile time. This method exists for
     * API consistency across platforms. It marks the library as initialized
     * but performs no actual runtime loading.
     *
     * @param context Not required for iOS. Can be null or any value.
     */
    actual fun init(context: Any?) {
        if (_isInitialized) return

        // iOS framework is linked at compile time via cinterop
        // No runtime library loading needed
        _isInitialized = true

        // TODO: When iOS native implementation is ready, call actual initialization here
        // For now, this is a stub that just marks as initialized
    }
}