package app.rive.mp.test.utils

import app.rive.mp.RiveNative

/**
 * Desktop-specific actual implementation of MpTestContext.
 * Phase A: Minimal implementation for basic tests.
 */
actual class MpTestContext {
    actual fun initRive() {
        // Desktop doesn't need context-based initialization
        // Initialize with Unit (no context needed)
        RiveNative.nativeInit(Unit)
    }
    
    actual fun getPlatformName(): String = "Desktop"
}
