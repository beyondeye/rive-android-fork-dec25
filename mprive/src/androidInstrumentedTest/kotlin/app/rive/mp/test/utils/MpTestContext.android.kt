package app.rive.mp.test.utils

import androidx.test.platform.app.InstrumentationRegistry
import app.rive.mp.RiveNative

/**
 * Android-specific actual implementation of MpTestContext.
 * Phase A: Minimal implementation for basic tests.
 */
actual class MpTestContext {
    private val context by lazy {
        InstrumentationRegistry.getInstrumentation().targetContext
    }
    
    actual fun initRive() {
        // Initialize Rive native library
        RiveNative.nativeInit(context)
    }
    
    actual fun getPlatformName(): String = "Android"
}
