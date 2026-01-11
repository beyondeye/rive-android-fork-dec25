package app.rive.mp.core

/**
 * Holds the native pointers to the listeners used by the CommandQueue.
 * 
 * On Android, this uses a single listener of each type to simplify lifetime management.
 * This is as opposed to a listener for each handle.
 *
 * The Listeners object is created from the native layer via [CommandQueueBridge.cppCreateListeners].
 *
 * @property fileListener Native pointer to the file listener.
 * @property artboardListener Native pointer to the artboard listener.
 * @property stateMachineListener Native pointer to the state machine listener.
 * @property viewModelInstanceListener Native pointer to the view model instance listener.
 * @property imageListener Native pointer to the image listener.
 * @property audioListener Native pointer to the audio listener.
 * @property fontListener Native pointer to the font listener.
 */
data class Listeners(
    val fileListener: Long,
    val artboardListener: Long,
    val stateMachineListener: Long,
    val viewModelInstanceListener: Long,
    val imageListener: Long,
    val audioListener: Long,
    val fontListener: Long,
) : AutoCloseable {
    /**
     * Close the listeners and free their native resources.
     * This is implemented via expect/actual for platform-specific cleanup.
     */
    override fun close() {
        closeNative()
    }
}

/**
 * Platform-specific implementation for closing native listeners.
 */
expect fun Listeners.closeNative()
