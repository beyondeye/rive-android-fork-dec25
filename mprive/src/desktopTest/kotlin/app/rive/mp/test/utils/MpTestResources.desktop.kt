package app.rive.mp.test.utils

/**
 * Desktop implementation of test resource loader.
 * Uses classloader to load resources from commonTest/resources.
 */
actual object MpTestResources {
    actual fun loadResource(resourcePath: String): ByteArray {
        try {
            val resourceStream = this::class.java.classLoader?.getResourceAsStream(resourcePath)
                ?: throw IllegalArgumentException("Resource not found: $resourcePath")
            
            return resourceStream.use { it.readBytes() }
        } catch (e: Exception) {
            throw IllegalArgumentException("Failed to load test resource: $resourcePath", e)
        }
    }
    
    actual fun resourceExists(resourcePath: String): Boolean {
        return try {
            val resourceStream = this::class.java.classLoader?.getResourceAsStream(resourcePath)
            resourceStream?.close()
            resourceStream != null
        } catch (e: Exception) {
            false
        }
    }
}
