package app.rive.mprive

import platform.UIKit.UIDevice

/**
 * iOS implementation of the Rive platform interface.
 */
actual class RivePlatform actual constructor() {
    actual fun getPlatformName(): String {
        return "iOS ${UIDevice.currentDevice.systemVersion}"
    }
    
    actual fun initialize(): Boolean {
        // TODO: Initialize iOS-specific Rive runtime via C-interop
        // This will use Kotlin/Native's cinterop mechanism
        return true
    }
}
