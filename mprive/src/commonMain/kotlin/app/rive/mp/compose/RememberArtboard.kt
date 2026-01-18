package app.rive.mp.compose

import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.remember
import app.rive.mp.Artboard
import app.rive.mp.ExperimentalRiveComposeAPI
import app.rive.mp.RiveFile

/**
 * Creates an [Artboard] from the given [RiveFile].
 *
 * The lifetime of the artboard is managed by this composable. It will delete the artboard when it
 * falls out of scope.
 *
 * Example usage:
 * ```kotlin
 * val riveWorker = rememberRiveWorker()
 * val fileResult = rememberRiveFile(source, riveWorker)
 *
 * when (fileResult) {
 *     is Result.Success -> {
 *         val artboard = rememberArtboard(fileResult.value)
 *         // Use artboard with Rive composable or create StateMachine
 *     }
 *     // Handle Loading/Error...
 * }
 * ```
 *
 * @param file The [RiveFile] to instantiate the artboard from.
 * @param artboardName The name of the artboard to load. If null, the default artboard will be
 *    loaded.
 * @return The created [Artboard].
 * @throws IllegalArgumentException If the artboard cannot be created.
 *
 * @see RiveFile For loading a Rive file.
 * @see rememberStateMachine For creating a state machine from the artboard.
 */
@ExperimentalRiveComposeAPI
@Composable
fun rememberArtboard(
    file: RiveFile,
    artboardName: String? = null,
): Artboard {
    val artboard = remember(file, artboardName) {
        Artboard.fromFile(file, artboardName)
    }

    DisposableEffect(artboard) {
        onDispose { artboard.close() }
    }

    return artboard
}