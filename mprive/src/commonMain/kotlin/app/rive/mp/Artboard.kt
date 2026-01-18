/**
 * Artboard - Multiplatform Rive artboard wrapper
 *
 * An instantiated artboard from a [RiveFile].
 *
 * Usage:
 * ```kotlin
 * // Using rememberArtboard composable (recommended):
 * val artboard = rememberArtboard(file)  // Default artboard
 * // or
 * val artboard = rememberArtboard(file, "MyArtboard")  // Named artboard
 *
 * // Using Artboard.fromFile directly (manual lifecycle management):
 * val artboard = Artboard.fromFile(file)
 * try {
 *     // Use the artboard...
 * } finally {
 *     artboard.close()
 * }
 * ```
 */
package app.rive.mp

private const val ARTBOARD_TAG = "Rive/Artboard"

/**
 * An instantiated artboard from a [RiveFile].
 *
 * Can be queried for state machine names and used to create a [StateMachine].
 *
 * Create an instance using [rememberArtboard] composable or [Artboard.fromFile].
 * When using the latter, ensure you call [close] when done to release resources.
 *
 * @param artboardHandle The handle to the artboard on the command server.
 * @param riveWorker The CommandQueue that owns the artboard.
 * @param fileHandle The file handle that owns this artboard.
 * @param name The name of the artboard, or null if default.
 */
class Artboard internal constructor(
    val artboardHandle: ArtboardHandle,
    internal val riveWorker: CommandQueue,
    internal val fileHandle: FileHandle,
    val name: String?,
) : AutoCloseable {

    private var closed = false

    companion object {
        /**
         * Creates a new [Artboard] from a file.
         *
         *   The lifetime of the returned artboard is managed by the caller. Make sure to call
         * [close] when you are done with it to release its resources.
         *
         * @param file The [RiveFile] to instantiate the artboard from.
         * @param artboardName The name of the artboard to load. If null, the default artboard will
         *    be loaded.
         * @return The created artboard.
         * @throws IllegalArgumentException If the artboard cannot be created.
         */
        fun fromFile(
            file: RiveFile,
            artboardName: String? = null
        ): Artboard {
            val handle = artboardName?.let { name ->
                file.riveWorker.createArtboardByName(file.fileHandle, name)
            } ?: file.riveWorker.createDefaultArtboard(file.fileHandle)
            
            val nameLog = artboardName?.let { "with name $it" } ?: "(default)"
            RiveLog.d(ARTBOARD_TAG) { "Created $handle $nameLog (${file.fileHandle})" }
            return Artboard(handle, file.riveWorker, file.fileHandle, artboardName)
        }
    }

    /**
     * Closes this artboard and releases its resources.
     *
     * After calling this method, the Artboard instance should not be used.
     * This method is idempotent - it's safe to call multiple times.
     */
    override fun close() {
        if (closed) return
        closed = true

        val nameLog = name?.let { "with name $it" } ?: "(default)"
        RiveLog.d(ARTBOARD_TAG) { "Deleting $artboardHandle $nameLog ($fileHandle)" }
        riveWorker.deleteArtboard(artboardHandle)
    }

    /**
     * Gets the names of all state machines on this artboard.
     *
     * @return A list of state machine names.
     * @throws IllegalStateException If the artboard has been closed.
     */
    suspend fun getStateMachineNames(): List<String> {
        check(!closed) { "Artboard has been closed" }
        return stateMachineNamesCache ?: riveWorker.getStateMachineNames(artboardHandle).also {
            stateMachineNamesCache = it
        }
    }

    private var stateMachineNamesCache: List<String>? = null

    /**
     * Resizes this artboard to match the given dimensions.
     *
     * 9 This is required when drawing with a fit type of [Fit.Layout], where the artboard is
     * expected to match the dimensions of the surface it is drawn to and layout its children within
     * those bounds.
     *
     *   In order for this to take effect, the state machine associated with this artboard must be
     * advanced, even if just by 0.
     *
     * @param width The new width in pixels.
     * @param height The new height in pixels.
     * @param scaleFactor The scale factor to apply when resizing. The artboard will be resized to
     *    dimensions divided by this factor. Defaults to 1f.
     * @throws IllegalStateException If the artboard has been closed.
     */
    @Throws(IllegalStateException::class)
    fun resizeArtboard(
        width: Int,
        height: Int,
        scaleFactor: Float = 1f
    ) {
        check(!closed) { "Artboard has been closed" }
        riveWorker.resizeArtboard(artboardHandle, width, height, scaleFactor)
    }

    /**
     * Resets this artboard to its original dimensions.
     *
     * 9 This should be called if the artboard was previously resized with [resizeArtboard] and
     * you now want to draw with a fit type other than [Fit.Layout], to restore the artboard to its
     * original size.
     *
     *   In order for this to take effect, the state machine associated with this artboard must be
     * advanced, even if just by 0.
     *
     * @throws IllegalStateException If the artboard has been closed.
     */
    @Throws(IllegalStateException::class)
    fun resetArtboardSize() {
        check(!closed) { "Artboard has been closed" }
        riveWorker.resetArtboardSize(artboardHandle)
    }

    override fun toString(): String = "Artboard($artboardHandle, name=$name)"
}