package app.rive.mp.test.utils

import app.rive.mp.RiveNative

/**
 * Desktop implementation of MpTestContext.
 * Initializes Rive runtime for Desktop testing.
 */
actual class MpTestContext {
    /**
     * Initialize Rive runtime for Desktop tests.
     * Calls RiveNative.nativeInit() with a dummy context (not needed on Desktop).
     */
    actual fun initRive() {
        try {
            // Desktop doesn't need a specific context, so we pass an empty object
            RiveNative.nativeInit(Any())
        } catch (e: Exception) {
            throw RuntimeException("Failed to initialize Rive for Desktop tests", e)
        }
    }
    
    /**
     * Get platform name for debugging.
     */
    actual fun getPlatformName(): String = "Desktop"
    
    actual companion object {
        private var initialized = false
        
        /**
         * Initialize the platform once for all tests.
         * This ensures Rive is initialized before any tests run.
         */
        actual fun initPlatform() {
            if (!initialized) {
                val testContext = MpTestContext()
                testContext.initRive()
                initialized = true
            }
        }
    }
}
