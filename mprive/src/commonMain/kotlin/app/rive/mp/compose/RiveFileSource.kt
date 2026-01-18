package app.rive.mp.compose

/**
 * The source for loading a Rive file.
 *
 * This is a platform-agnostic sealed interface that defines the different ways
 * a Rive file can be loaded. Platform-specific implementations can add their
 * own variants.
 *
 * **Common sources:**
 * - [Bytes] - Load from a raw byte array (works on all platforms)
 *
 * **Platform-specific sources (defined via expect/actual):**
 * - Android: `RawRes` for loading from Android raw resources
 * - Desktop: `ClasspathResource` for loading from JVM classpath
 * - iOS: `BundleResource` for loading from iOS bundles
 *
 * @see app.rive.mp.RiveFile
 */
sealed interface RiveFileSource {
    /**
     * Load a Rive file from a raw byte array.
     *
     * This is the most portable option and works on all platforms.
     * Use this when you have already loaded the file bytes yourself.
     *
     * Example:
     * ```kotlin
     * val bytes = loadFileFromNetwork("https://example.com/animation.riv")
     * val source = RiveFileSource.Bytes(bytes)
     * ```
     *
     * @property data The raw bytes of the .riv file.
     */
    @JvmInline
    value class Bytes(val data: ByteArray) : RiveFileSource
}