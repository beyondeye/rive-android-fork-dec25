package app.rive.mp.compose

import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.remember
import app.rive.mp.Artboard
import app.rive.mp.RiveLog
import app.rive.mp.StateMachine

/**
 * Creates a [StateMachine] from the given [Artboard].
 *
 * The lifetime of the state machine is managed by this composable. It will delete the state machine
 * when it falls out of scope.
 *
 * Example usage:
 * ```kotlin
 * val riveWorker = rememberRiveWorker()
 * val fileResult = rememberRiveFile(source, riveWorker)
 *
 * when (fileResult) {
 *     is Result.Success -> {
 *         val artboard = rememberArtboard(fileResult.value)
 *         val stateMachine = rememberStateMachine(artboard)
 *         // Use with Rive composable
 *     }
 *     // Handle Loading/Error...
 * }
 * ```
 *
 * @param artboard The [Artboard] to create the state machine from.
 * @param stateMachineName The name of the state machine to load. If null, the default state machine
 *    will be loaded.
 * @return The created [StateMachine].
 * @throws IllegalArgumentException If the state machine cannot be created.
 *
 * @see Artboard For creating an artboard.
 * @see rememberArtboard For creating an artboard with lifecycle management.
 */
@Composable
fun rememberStateMachine(
    artboard: Artboard,
    stateMachineName: String? = null,
): StateMachine {
    val stateMachine = remember(artboard, stateMachineName) {
        StateMachine.fromArtboard(artboard, stateMachineName)
    }

    DisposableEffect(stateMachine) {
        onDispose { stateMachine.close() }
    }

    return stateMachine
}

private const val TAG = "Rive/SM"

/**
 * Creates a [StateMachine] from the given [Artboard], or returns null if no state machines exist.
 *
 * This is useful when you want to handle artboards that may or may not have state machines.
 * For artboards with only simple animations (no state machines), this will return null
 * and you can still render the artboard without interactivity.
 *
 * @param artboard The [Artboard] to create the state machine from.
 * @param stateMachineName The name of the state machine to load. If null, the default state machine
 *    will be loaded.
 * @return The created [StateMachine], or null if no state machines exist in the artboard.
 *
 * @see rememberStateMachine For the non-nullable version that throws on missing state machines.
 */
@Composable
fun rememberStateMachineOrNull(
    artboard: Artboard,
    stateMachineName: String? = null,
): StateMachine? {
    val stateMachine = remember(artboard, stateMachineName) {
        try {
            StateMachine.fromArtboard(artboard, stateMachineName)
        } catch (e: IllegalArgumentException) {
            RiveLog.w(TAG) { "No state machines in artboard: ${e.message}" }
            null
        }
    }

    DisposableEffect(stateMachine) {
        onDispose { stateMachine?.close() }
    }

    return stateMachine
}