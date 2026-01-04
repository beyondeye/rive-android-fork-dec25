package app.rive.mp.test.utils

/**
 * iOS stub implementation of MpTestContext.
 * iOS tests are not currently supported - this exists only to satisfy the compiler.
 * 
 * Note: If you need to run tests on iOS in the future, implement proper initialization here.
 */
actual class MpTestContext {
    actual fun initRive() {
        // iOS tests not implemented
        // Throw an exception if someone tries to use this
        throw UnsupportedOperationException(
            "iOS tests are not implemented. " +
            "This is a stub to satisfy the compiler for Android/Desktop-only testing."
        )
    }
    
    actual fun getPlatformName(): String = "iOS (stub)"
}
