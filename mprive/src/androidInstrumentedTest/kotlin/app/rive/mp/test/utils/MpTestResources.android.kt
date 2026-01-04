package app.rive.mp.test.utils

import androidx.test.platform.app.InstrumentationRegistry

/**
 * Android implementation of test resource loader.
 * Uses Android's instrumentation context to load resources from commonTest/resources.
 */
actual object MpTestResources {
    private val context = InstrumentationRegistry.getInstrumentation().context
    
    actual fun loadResource(resourcePath: String): ByteArray {
        try {
            // Android test resources are accessed via assets
            return context.assets.open(resourcePath).use { it.readBytes() }
        } catch (e: Exception) {
            throw IllegalArgumentException("Failed to load test resource: $resourcePath", e)
        }
    }
    
    actual fun resourceExists(resourcePath: String): Boolean {
        return try {
            context.assets.open(resourcePath).close()
            true
        } catch (e: Exception) {
            false
        }
    }
}
