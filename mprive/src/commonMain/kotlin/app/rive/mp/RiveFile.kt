/**
 * RiveFile - Multiplatform Rive file wrapper
 *
 * A Rive file containing artboards, state machines, and view model instances.
 *
 * Usage:
 * ```kotlin
 * // Using rememberRiveFile composable (recommended):
 * val fileResult = rememberRiveFile(
 *     source = RiveFileSource.Bytes(bytes),
 *     riveWorker = riveWorker
 * )
 *
 * when (fileResult) {
 *     is Result.Loading -> { /* Show loading indicator */ }
 *     is Result.Error -> { /* Handle error */ }
 *     is Result.Success -> {
 *         val file = fileResult.value
 *         // Use the file...
 *     }
 * }
 *
 * // Using fromSource directly (manual lifecycle management):
 * val file = RiveFile.fromSource(source, riveWorker)
 * try {
 *     // Use the file...
 * } finally {
 *     file.close()
 * }
 * ```
 */
package app.rive.mp

import androidx.compose.runtime.Stable
import app.rive.mp.compose.RiveFileSource
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlin.coroutines.cancellation.CancellationException

private const val FILE_TAG = "Rive/File"

/**
 * Represents a loaded Rive file containing artboards, state machines, and view model instances.
 *
 * A Rive file is created from the Rive editor and exported as a `.riv` file.
 *
 * Create an instance using [rememberRiveFile] composable or [RiveFile.fromSource].
 * When using the latter, ensure you call [close] when done to release resources.
 *
 * This object can query file contents (artboard names, view model names, etc.) and
 * be passed to [rememberArtboard] to create an [Artboard].
 *
 * @param fileHandle The handle to the file on the command server.
 * @param riveWorker The CommandQueue that owns and performs operations on this file.
 */
@Stable
class RiveFile internal constructor(
    val fileHandle: FileHandle,
    val riveWorker: CommandQueue
) : AutoCloseable {

    private var closed = false

    companion object {
        /**
         * Loads a [RiveFile] from the given [source].
         *
         *   The lifetime of the [RiveFile] is managed by the caller. Make sure to call [close]
         * when you are done with the file to release its resources.
         *
         * @param source The source of the Rive file.
         * @param riveWorker The CommandQueue that will own the file.
         * @return The [Result] of loading the file - either Success with the file, or Error.
         */
        suspend fun fromSource(
            source: RiveFileSource,
            riveWorker: CommandQueue
        ): Result<RiveFile> {
            RiveLog.d(FILE_TAG) { "Loading Rive file from source: $source" }
            return try {
                riveWorker.acquire(FILE_TAG)

                val fileBytes = when (source) {
                    is RiveFileSource.Bytes -> source.data
                }

                RiveLog.v(FILE_TAG) { "Loaded Rive file bytes from source: $source; sending to Rive worker" }
                val fileHandle = riveWorker.loadFile(fileBytes)

                RiveLog.d(FILE_TAG) { "Loaded Rive file from source: $source; $fileHandle" }
                Result.Success(RiveFile(fileHandle, riveWorker))
            } catch (ce: CancellationException) {
                // Thrown by suspend function if the coroutine is cancelled
                RiveLog.d(FILE_TAG) { "Rive file loading was cancelled: $source" }
                riveWorker.release(FILE_TAG, "Cancellation")
                // Propagate the cancellation exception
                throw ce
            } catch (e: Exception) {
                RiveLog.e(FILE_TAG, e) { "Error loading Rive file with source: $source" }
                riveWorker.release(FILE_TAG, "Load error")
                Result.Error(e)
            }
        }
    }

    /**
     * Closes this file and releases its resources.
     *
     * After calling this method, the RiveFile instance should not be used.
     * This method is idempotent - it's safe to call multiple times.
     */
    override fun close() {
        if (closed) return
        closed = true

        RiveLog.d(FILE_TAG) { "Deleting $fileHandle" }
        riveWorker.deleteFile(fileHandle)
        riveWorker.release(FILE_TAG, "RiveFile closed")
    }

    /**
     * Gets the names of all artboards in this file.
     *
     * @return A list of artboard names.
     * @throws IllegalStateException If the file has been closed.
     */
    suspend fun getArtboardNames(): List<String> {
        check(!closed) { "RiveFile has been closed" }
        return artboardNamesCache ?: riveWorker.getArtboardNames(fileHandle).also {
            artboardNamesCache = it
        }
    }

    private var artboardNamesCache: List<String>? = null

    /**
     * Gets the names of all view models in this file.
     *
     * @return A list of view model names.
     * @throws IllegalStateException If the file has been closed.
     */
    suspend fun getViewModelNames(): List<String> {
        check(!closed) { "RiveFile has been closed" }
        return viewModelNamesCache ?: riveWorker.getViewModelNames(fileHandle).also {
            viewModelNamesCache = it
        }
    }

    private var viewModelNamesCache: List<String>? = null

    /**
     * Gets the names of all instances of a ViewModel in this file.
     *
     * @param viewModel The name of the view model to query instances for.
     * @return A list of instance names for the specified ViewModel.
     * @throws IllegalStateException If the file has been closed.
     */
    suspend fun getViewModelInstanceNames(viewModel: String): List<String> {
        check(!closed) { "RiveFile has been closed" }
        return synchronized(instanceNamesCache) {
            instanceNamesCache.getOrPut(viewModel) {
                null // Placeholder, will be filled below
            }
        } ?: riveWorker.getViewModelInstanceNames(fileHandle, viewModel).also { names ->
            synchronized(instanceNamesCache) {
                instanceNamesCache[viewModel] = names
            }
        }
    }

    private val instanceNamesCache = mutableMapOf<String, List<String>?>()

    /**
     * Gets the properties defined on a ViewModel in this file.
     *
     * @param viewModel The name of the view model to query properties for.
     * @return A list of property definitions for the specified ViewModel.
     * @throws IllegalStateException If the file has been closed.
     */
    suspend fun getViewModelProperties(viewModel: String): List<ViewModelProperty> {
        check(!closed) { "RiveFile has been closed" }
        return synchronized(propertiesCache) {
            propertiesCache.getOrPut(viewModel) {
                null // Placeholder, will be filled below
            }
        } ?: riveWorker.getViewModelProperties(fileHandle, viewModel).also { props ->
            synchronized(propertiesCache) {
                propertiesCache[viewModel] = props
            }
        }
    }

    private val propertiesCache = mutableMapOf<String, List<ViewModelProperty>?>()

    /**
     * Gets all enum definitions in this file.
     *
     * @return A list of enum definitions in the file.
     * @throws IllegalStateException If the file has been closed.
     */
    suspend fun getEnums(): List<RiveEnum> {
        check(!closed) { "RiveFile has been closed" }
        return enumsCache ?: riveWorker.getEnums(fileHandle).also {
            enumsCache = it
        }
    }

    private var enumsCache: List<RiveEnum>? = null

    override fun toString(): String = "RiveFile($fileHandle)"
}