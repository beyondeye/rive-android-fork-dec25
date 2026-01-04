package app.rive.mp.test.utils

import androidx.test.platform.app.InstrumentationRegistry
import app.rive.mp.RiveNative

/**
 * Android implementation of MpTestContext.
 * Uses Android's instrumentation context to initialize Rive for testing.
 */
actual class MpTestContext {
    private val context = InstrumentationRegistry.getInstrumentation().targetContext
    
    /**
     * Initialize Rive runtime for Android tests.
     * Calls RiveNative.nativeInit() with the instrumentation target context.
     */
    actual fun initRive() {
        try {
            RiveNative.nativeInit(context)
        } catch (e: Exception) {
            throw RuntimeException("Failed to initialize Rive for Android tests", e)
        }
    }
    
    /**
     * Get platform name for debugging.
     */
    actual fun getPlatformName(): String = "Android"
    
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
