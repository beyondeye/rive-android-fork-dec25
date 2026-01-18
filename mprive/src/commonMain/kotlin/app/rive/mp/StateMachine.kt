/**
 * StateMachine - Multiplatform Rive state machine wrapper
 *
 * A state machine instance from an [Artboard] that drives animations and handles input.
 *
 * Usage:
 * ```kotlin
 * // Using rememberStateMachine composable (recommended):
 * val stateMachine = rememberStateMachine(artboard)  // Default state machine
 * // or
 * val stateMachine = rememberStateMachine(artboard, "MyStateMachine")  // Named
 *
 * // Using StateMachine.fromArtboard directly (manual lifecycle management):
 * val stateMachine = StateMachine.fromArtboard(artboard)
 * try {
 *     // Use the state machine...
 * } finally {
 *     stateMachine.close()
 * }
 * ```
 */
package app.rive.mp

import kotlin.time.Duration

private const val STATE_MACHINE_TAG = "Rive/SM"

/**
 * A state machine instance from an [Artboard] that drives animations and handles input.
 *
 * State machines control animation playback and respond to inputs (numbers, booleans, triggers).
 * They can also be bound to a [ViewModelInstance] for data binding.
 *
 * Create an instance using [rememberStateMachine] composable or [StateMachine.fromArtboard].
 * When using the latter, ensure you call [close] when done to release resources.
 *
 * @param stateMachineHandle The handle to the state machine on the command server.
 * @param riveWorker The CommandQueue that owns the state machine.
 * @param artboardHandle The artboard handle that owns this state machine.
 * @param name The name of the state machine, or null if default.
 */
class StateMachine internal constructor(
    val stateMachineHandle: StateMachineHandle,
    internal val riveWorker: CommandQueue,
    internal val artboardHandle: ArtboardHandle,
    val name: String?,
) : AutoCloseable {

    private var closed = false

    companion object {
        /**
         * Creates a new [StateMachine] from an artboard.
         *
         * ⚠️ The lifetime of the returned state machine is managed by the caller. Make sure to call
         * [close] when you are done with it to release its resources.
         *
         * @param artboard The [Artboard] to create the state machine from.
         * @param stateMachineName The name of the state machine to load. If null, the default 
         *    state machine will be loaded.
         * @return The created state machine.
         * @throws IllegalArgumentException If the state machine cannot be created.
         */
        fun fromArtboard(
            artboard: Artboard,
            stateMachineName: String? = null
        ): StateMachine {
            val handle = stateMachineName?.let { name ->
                artboard.riveWorker.createStateMachineByName(artboard.artboardHandle, name)
            } ?: artboard.riveWorker.createDefaultStateMachine(artboard.artboardHandle)
            
            val nameLog = stateMachineName?.let { "with name $it" } ?: "(default)"
            RiveLog.d(STATE_MACHINE_TAG) { "Created $handle $nameLog (${artboard.artboardHandle})" }
            return StateMachine(handle, artboard.riveWorker, artboard.artboardHandle, stateMachineName)
        }
    }

    /**
     * Closes this state machine and releases its resources.
     *
     * After calling this method, the StateMachine instance should not be used.
     * This method is idempotent - it's safe to call multiple times.
     */
    override fun close() {
        if (closed) return
        closed = true

        val nameLog = name?.let { "with name $it" } ?: "(default)"
        RiveLog.d(STATE_MACHINE_TAG) { "Deleting $stateMachineHandle $nameLog ($artboardHandle)" }
        riveWorker.deleteStateMachine(stateMachineHandle)
    }

    /**
     * Advances the state machine by the given time delta.
     *
     * This should be called every frame to update animations. The time delta is
     * typically the time since the last frame.
     *
     * @param deltaTime The time to advance by.
     * @throws IllegalStateException If the state machine has been closed.
     */
    @Throws(IllegalStateException::class)
    fun advance(deltaTime: Duration) {
        check(!closed) { "StateMachine has been closed" }
        riveWorker.advanceStateMachine(stateMachineHandle, deltaTime.inWholeNanoseconds / 1_000_000_000f)
    }

    // =============================================================================
    // Input Operations
    // =============================================================================

    /**
     * Gets the number of inputs on this state machine.
     *
     * @return The number of inputs.
     * @throws IllegalStateException If the state machine has been closed.
     */
    suspend fun getInputCount(): Int {
        check(!closed) { "StateMachine has been closed" }
        return riveWorker.getInputCount(stateMachineHandle)
    }

    /**
     * Gets the names of all inputs on this state machine.
     *
     * @return A list of input names.
     * @throws IllegalStateException If the state machine has been closed.
     */
    suspend fun getInputNames(): List<String> {
        check(!closed) { "StateMachine has been closed" }
        return riveWorker.getInputNames(stateMachineHandle)
    }

    /**
     * Gets information about an input by index.
     *
     * @param index The index of the input (0-based).
     * @return The input information (name and type).
     * @throws IllegalStateException If the state machine has been closed.
     */
    suspend fun getInputInfo(index: Int): InputInfo {
        check(!closed) { "StateMachine has been closed" }
        return riveWorker.getInputInfo(stateMachineHandle, index)
    }

    /**
     * Gets the value of a number input.
     *
     * @param inputName The name of the input.
     * @return The current value of the input.
     * @throws IllegalStateException If the state machine has been closed.
     */
    suspend fun getNumberInput(inputName: String): Float {
        check(!closed) { "StateMachine has been closed" }
        return riveWorker.getNumberInput(stateMachineHandle, inputName)
    }

    /**
     * Sets the value of a number input.
     *
     * @param inputName The name of the input.
     * @param value The value to set.
     * @throws IllegalStateException If the state machine has been closed.
     */
    @Throws(IllegalStateException::class)
    fun setNumberInput(inputName: String, value: Float) {
        check(!closed) { "StateMachine has been closed" }
        riveWorker.setNumberInput(stateMachineHandle, inputName, value)
    }

    /**
     * Gets the value of a boolean input.
     *
     * @param inputName The name of the input.
     * @return The current value of the input.
     * @throws IllegalStateException If the state machine has been closed.
     */
    suspend fun getBooleanInput(inputName: String): Boolean {
        check(!closed) { "StateMachine has been closed" }
        return riveWorker.getBooleanInput(stateMachineHandle, inputName)
    }

    /**
     * Sets the value of a boolean input.
     *
     * @param inputName The name of the input.
     * @param value The value to set.
     * @throws IllegalStateException If the state machine has been closed.
     */
    @Throws(IllegalStateException::class)
    fun setBooleanInput(inputName: String, value: Boolean) {
        check(!closed) { "StateMachine has been closed" }
        riveWorker.setBooleanInput(stateMachineHandle, inputName, value)
    }

    /**
     * Fires a trigger input.
     *
     * @param inputName The name of the trigger input.
     * @throws IllegalStateException If the state machine has been closed.
     */
    @Throws(IllegalStateException::class)
    fun fireTrigger(inputName: String) {
        check(!closed) { "StateMachine has been closed" }
        riveWorker.fireTrigger(stateMachineHandle, inputName)
    }

    override fun toString(): String = "StateMachine($stateMachineHandle, name=$name)"
}