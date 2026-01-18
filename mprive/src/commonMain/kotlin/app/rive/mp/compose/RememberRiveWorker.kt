package app.rive.mp.compose

import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.MutableState
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.withFrameNanos
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.compose.LocalLifecycleOwner
import androidx.lifecycle.repeatOnLifecycle
import app.rive.mp.CommandQueue
import app.rive.mp.RiveInitializationException
import app.rive.mp.RiveLog
import kotlinx.coroutines.isActive

private const val RIVE_WORKER_TAG = "Rive/Worker"

/**
 * A [CommandQueue] (also known as RiveWorker) is the worker that runs Rive in a thread.
 * It holds all of the state, including assets, Rive files, artboards, state machines,
 * and view model instances.
 *
 * The lifetime of the Rive worker is managed by this composable. It will release the resources
 * allocated to the Rive worker when it falls out of scope.
 *
 * A Rive worker needs to be polled to receive messages from the command server. This composable
 * creates a poll loop that runs while the [Lifecycle] is in the [Lifecycle.State.RESUMED] state.
 * The poll rate is once per frame, which is typically 60 FPS.
 *
 * This function throws a [RiveInitializationException] if the Rive worker cannot be created.
 * If you want to handle failure gracefully, use [rememberRiveWorkerOrNull] instead.
 *
 * @param autoPoll Whether to automatically poll the CommandQueue every frame while resumed.
 *                 Defaults to true. Set to false if you want to manually control polling.
 * @return The created [CommandQueue].
 * @throws RiveInitializationException If the Rive worker cannot be created for any reason.
 * @see CommandQueue
 * @see rememberRiveWorkerOrNull
 */
@Composable
@Throws(RiveInitializationException::class)
fun rememberRiveWorker(autoPoll: Boolean = true): CommandQueue {
    val errorState = remember { mutableStateOf<Throwable?>(null) }
    val riveWorker = rememberRiveWorkerOrNull(errorState, autoPoll)
    return riveWorker ?: throw RiveInitializationException(
        "Failed to create Rive worker",
        errorState.value
    )
}

/**
 * A nullable variant of [rememberRiveWorker] that returns null if the Rive worker cannot be
 * created.
 *
 * Use this variant if you want to handle the failure of Rive worker creation gracefully, which may
 * be desirable in production.
 *
 * @param errorState A mutable state that holds the error if the Rive worker creation fails. Useful
 *    if you want to display or pass the error.
 * @param autoPoll Whether to automatically poll the CommandQueue every frame while resumed.
 *                 Defaults to true. Set to false if you want to manually control polling.
 * @return The created [CommandQueue], or null if creation failed.
 * @see rememberRiveWorker
 */
@Composable
fun rememberRiveWorkerOrNull(
    errorState: MutableState<Throwable?> = mutableStateOf(null),
    autoPoll: Boolean = true,
): CommandQueue? {
    val lifecycleOwner = LocalLifecycleOwner.current
    
    val worker = remember {
        runCatching { CommandQueue() }
            .onFailure {
                if (errorState.value == null) {
                    errorState.value = it
                }
                RiveLog.e(RIVE_WORKER_TAG) { "Failed to create Rive worker: ${it.message}" }
            }.getOrNull()
    }

    /**
     * Start polling the Rive worker for messages. This runs in a loop while the [Lifecycle] is in
     * the [Lifecycle.State.RESUMED] state.
     *
     * Uses Compose's [withFrameNanos] for frame timing, which works across all platforms.
     */
    LaunchedEffect(lifecycleOwner, worker, autoPoll) {
        if (worker == null || !autoPoll) return@LaunchedEffect

        lifecycleOwner.lifecycle.repeatOnLifecycle(Lifecycle.State.RESUMED) {
            RiveLog.d(RIVE_WORKER_TAG) { "Starting command queue polling" }
            while (isActive) {
                withFrameNanos { _ ->
                    worker.pollMessages()
                }
            }
            RiveLog.d(RIVE_WORKER_TAG) { "Stopping command queue polling" }
        }
    }

    // Note: Audio engine management is platform-specific and handled separately
    // On Android, this would be done via AudioEngine.acquire()/release()
    // For now, we focus on the core lifecycle management

    /** Disposes the Rive worker when it falls out of scope. */
    DisposableEffect(worker) {
        if (worker == null) return@DisposableEffect onDispose {}

        onDispose {
            worker.release(RIVE_WORKER_TAG, "Compose dispose")
        }
    }

    return worker
}