package app.rive.mp.compose

import androidx.compose.runtime.Composable
import androidx.compose.runtime.produceState
import app.rive.mp.CommandQueue
import app.rive.mp.Result
import app.rive.mp.RiveFile
import app.rive.mp.RiveUI_ExperimentalAPI

/**
 * Loads a [RiveFile] from the given [source].
 *
 * The lifetime of the [RiveFile] is managed by this composable. It will release the resources
 * allocated to the file when it falls out of scope.
 *
 * This composable uses Compose's [produceState] to handle the asynchronous loading of the file.
 * While loading, the result will be [Result.Loading]. Once loaded, it will be [Result.Success]
 * with the [RiveFile]. If loading fails, it will be [Result.Error] with the exception.
 *
 * Example usage:
 * ```kotlin
 * val riveWorker = rememberRiveWorker()
 * val fileResult = rememberRiveFile(
 *     source = RiveFileSource.Bytes(fileBytes),
 *     riveWorker = riveWorker
 * )
 *
 * when (fileResult) {
 *     is Result.Loading -> CircularProgressIndicator()
 *     is Result.Error -> Text("Error: ${fileResult.throwable.message}")
 *     is Result.Success -> {
 *         val file = fileResult.value
 *         // Use the file with Rive composable
 *     }
 * }
 * ```
 *
 * @param source The source of the Rive file, which can be a byte array or platform-specific source.
 * @param riveWorker The CommandQueue that will own and manage the file.
 * @return The [Result] of loading the Rive file, which can be Loading, Error, or Success
 *         with the [RiveFile].
 *
 * @see RiveFileSource For available file source types.
 * @see RiveFile For operations available on the loaded file.
 * @see rememberRiveWorker For creating the CommandQueue.
 */
@RiveUI_ExperimentalAPI
@Composable
fun rememberRiveFile(
    source: RiveFileSource,
    riveWorker: CommandQueue,
): Result<RiveFile> = produceState<Result<RiveFile>>(Result.Loading, source, riveWorker) {
    val result = RiveFile.fromSource(source, riveWorker)
    value = result

    when (result) {
        is Result.Success -> awaitDispose {
            result.value.close()
        }

        else -> {
            // For Loading and Error states, no cleanup needed
        }
    }
}.value