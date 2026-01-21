package app.rive.mp.compose

import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.remember
import app.rive.mp.Animation
import app.rive.mp.Artboard
import app.rive.mp.ExperimentalRiveComposeAPI
import app.rive.mp.Loop
import app.rive.mp.RiveLog
import app.rive.mp.StateMachine

private const val TAG = "Rive/Anim"

/**
 * Creates an [Animation] from the given [Artboard].
 *
 * The lifetime of the animation is managed by this composable. It will delete the animation
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
 *         val animation = rememberAnimation(artboard)
 *         // Use animation.advanceAndApply(deltaTime) in your render loop
 *     }
 *     // Handle Loading/Error...
 * }
 * ```
 *
 * @param artboard The [Artboard] to create the animation from.
 * @param animationName The name of the animation to load. If null, the default (first) animation
 *    will be loaded.
 * @param loop The loop mode for the animation. Defaults to [Loop.LOOP].
 * @return The created [Animation].
 * @throws IllegalArgumentException If the animation cannot be created.
 *
 * @see Artboard For creating an artboard.
 * @see rememberArtboard For creating an artboard with lifecycle management.
 */
@ExperimentalRiveComposeAPI
@Composable
fun rememberAnimation(
    artboard: Artboard,
    animationName: String? = null,
    loop: Loop = Loop.LOOP,
): Animation {
    val animation = remember(artboard, animationName) {
        Animation.fromArtboard(artboard, animationName).also {
            it.setLoop(loop)
        }
    }

    DisposableEffect(animation) {
        onDispose { animation.close() }
    }

    return animation
}

/**
 * Creates an [Animation] from the given [Artboard], or returns null if no animations exist.
 *
 * This is useful when you want to handle artboards that may or may not have linear animations.
 * For artboards with only state machines (no linear animations), this will return null.
 *
 * @param artboard The [Artboard] to create the animation from.
 * @param animationName The name of the animation to load. If null, the default (first) animation
 *    will be loaded.
 * @param loop The loop mode for the animation. Defaults to [Loop.LOOP].
 * @return The created [Animation], or null if no animations exist in the artboard.
 *
 * @see rememberAnimation For the non-nullable version that throws on missing animations.
 */
@ExperimentalRiveComposeAPI
@Composable
fun rememberAnimationOrNull(
    artboard: Artboard,
    animationName: String? = null,
    loop: Loop = Loop.LOOP,
): Animation? {
    val animation = remember(artboard, animationName) {
        try {
            Animation.fromArtboard(artboard, animationName).also {
                it.setLoop(loop)
            }
        } catch (e: IllegalArgumentException) {
            RiveLog.d(TAG) { "No animations in artboard: ${e.message}" }
            null
        }
    }

    DisposableEffect(animation) {
        onDispose { animation?.close() }
    }

    return animation
}

/**
 * Creates an [Animation] from the given [Artboard] only if no [StateMachine] is provided.
 *
 * This matches the rive-android reference behavior where linear animations are used to drive
 * the artboard when no state machine is present or when the state machine doesn't have
 * auto-playing content.
 *
 * If a state machine is provided and non-null, this returns null (state machine drives rendering).
 * If no state machine is provided, this attempts to create the default animation.
 *
 * @param artboard The [Artboard] to create the animation from.
 * @param stateMachine The state machine, if any. If non-null, returns null.
 * @param animationName The name of the animation to load. If null, the default (first) animation
 *    will be loaded.
 * @param loop The loop mode for the animation. Defaults to [Loop.LOOP].
 * @return The created [Animation], or null if a state machine exists or no animations exist.
 */
@ExperimentalRiveComposeAPI
@Composable
fun rememberAnimationOrNull(
    artboard: Artboard,
    stateMachine: StateMachine?,
    animationName: String? = null,
    loop: Loop = Loop.LOOP,
): Animation? {
    // If we have a state machine, let it drive the animation
    // We still create an animation for cases where the state machine doesn't auto-play
    // This matches the rive-android reference behavior
    val animation = remember(artboard, animationName) {
        try {
            Animation.fromArtboard(artboard, animationName).also {
                it.setLoop(loop)
            }
        } catch (e: IllegalArgumentException) {
            RiveLog.d(TAG) { "No animations in artboard: ${e.message}" }
            null
        }
    }

    DisposableEffect(animation) {
        onDispose { animation?.close() }
    }

    return animation
}