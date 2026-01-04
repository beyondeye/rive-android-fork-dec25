package app.rive.mp.test.utils

/**
 * Multiplatform test resource loader.
 * Provides expect/actual implementations for loading test resources (.riv files, images, etc.)
 * from the test resources directory.
 */
expect object MpTestResources {
    /**
     * Load a test resource file as a ByteArray.
     * 
     * @param resourcePath The path to the resource file relative to resources directory.
     *                     Example: "rive/flux_capacitor.riv"
     * @return The file contents as a ByteArray.
     * @throws IllegalArgumentException If the resource cannot be found or loaded.
     */
    fun loadResource(resourcePath: String): ByteArray
    
    /**
     * Check if a test resource file exists.
     * 
     * @param resourcePath The path to the resource file relative to resources directory.
     * @return True if the resource exists, false otherwise.
     */
    fun resourceExists(resourcePath: String): Boolean
}

/**
 * Helper extension to load Rive files from the test resources.
 * 
 * @param filename The Rive file name (without path).
 * @return The file contents as a ByteArray.
 */
fun MpTestResources.loadRiveFile(filename: String): ByteArray {
    return loadResource("rive/$filename")
}
