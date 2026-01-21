/**
 * Animation - Multiplatform Rive linear animation wrapper
 *
 * A linear animation instance from an [Artboard] that plays timeline-based animations.
 *
 * Linear animations are simpler than state machines - they play a timeline without
 * interactivity. Use them when you need simple playback without inputs/triggers.
 *
 * Usage:
 * ```kotlin
 * // Using rememberAnimation composable (recommended):
 * val animation = rememberAnimation(artboard)  // Default animation
 * // or
 * val animation = rememberAnimation(artboard, "MyAnimation")  // Named
 *
 * // Using Animation.fromArtboard directly (manual lifecycle management):
 * val animation = Animation.fromArtboard(artboard)
 * try {
 *     // Use the animation...
 * } finally {
 *     animation.close()
 * }
 * ```
 */
package app.rive.mp

import kotlin.time.Duration

private const val ANIMATION_TAG = "Rive/Animation"

/**
 * Loop mode for linear animations.
 */
enum class Loop(val value: Int) {
    /** Plays once and stops. */
    ONE_SHOT(0),
    /** Loops continuously. */
    LOOP(1),
    /** Plays forward then backward, continuously. */
    PING_PONG(2)
}

/**
 * Playback direction for linear animations.
 */
enum class Direction(val value: Int) {
    /** Plays forward. */
    FORWARDS(1),
    /** Plays backward. */
    BACKWARDS(-1)
}

/**
 * A linear animation instance from an [Artboard] that plays timeline-based animations.
 *
 * Linear animations are timeline-based and play independently of state machines.
 * They support loop modes (oneShot, loop, pingPong) and direction control.
 *
 * Create an instance using [rememberAnimation] composable or [Animation.fromArtboard].
 * When using the latter, ensure you call [close] when done to release resources.
 *
 * @param animationHandle The handle to the animation on the command server.
 * @param riveWorker The CommandQueue that owns the animation.
 * @param artboardHandle The artboard handle that owns this animation.
 * @param name The name of the animation, or null if default.
 */
class Animation internal constructor(
    val animationHandle: AnimationHandle,
    internal val riveWorker: CommandQueue,
    internal val artboardHandle: ArtboardHandle,
    val name: String?,
) : AutoCloseable {

    private var closed = false

    companion object {
        /**
         * Creates a new [Animation] from an artboard.
         *
         * ⚠️ The lifetime of the returned animation is managed by the caller. Make sure to call
         * [close] when you are done with it to release its resources.
         *
         * @param artboard The [Artboard] to create the animation from.
         * @param animationName The name of the animation to load. If null, the default (first)
         *    animation will be loaded.
         * @return The created animation.
         * @throws IllegalArgumentException If the animation cannot be created.
         */
        fun fromArtboard(
            artboard: Artboard,
            animationName: String? = null
        ): Animation {
            val handle = animationName?.let { name ->
                artboard.riveWorker.createAnimationByName(artboard.artboardHandle, name)
            } ?: artboard.riveWorker.createDefaultAnimation(artboard.artboardHandle)

            val nameLog = animationName?.let { "with name $it" } ?: "(default)"
            RiveLog.d(ANIMATION_TAG) { "Created $handle $nameLog (${artboard.artboardHandle})" }
            return Animation(handle, artboard.riveWorker, artboard.artboardHandle, animationName)
        }

        /**
         * Creates a new [Animation] from an artboard, returning null if creation fails.
         *
         * Use this when you want to gracefully handle artboards that have no animations.
         *
         * @param artboard The [Artboard] to create the animation from.
         * @param animationName The name of the animation to load. If null, the default (first)
         *    animation will be loaded.
         * @return The created animation, or null if the artboard has no animations.
         */
        fun fromArtboardOrNull(
            artboard: Artboard,
            animationName: String? = null
        ): Animation? {
            return try {
                fromArtboard(artboard, animationName)
            } catch (e: IllegalArgumentException) {
                RiveLog.d(ANIMATION_TAG) { "No animation available: ${e.message}" }
                null
            }
        }
    }

    /**
     * Closes this animation and releases its resources.
     *
     * After calling this method, the Animation instance should not be used.
     * This method is idempotent - it's safe to call multiple times.
     */
    override fun close() {
        if (closed) return
        closed = true

        val nameLog = name?.let { "with name $it" } ?: "(default)"
        RiveLog.d(ANIMATION_TAG) { "Deleting $animationHandle $nameLog ($artboardHandle)" }
        riveWorker.deleteAnimation(animationHandle)
    }

    /**
     * Advances the animation by the given time delta and applies it to the artboard.
     *
     * This should be called every frame to update the animation. The time delta is
     * typically the time since the last frame.
     *
     * @param deltaTime The time to advance by.
     * @param advanceArtboard If true, also advances the artboard (use when no state machine is active).
     *                        When a state machine is present, it handles artboard advancement internally,
     *                        so pass false. Defaults to true for standalone animation playback.
     * @return true if the animation is still playing, false if it completed (oneShot mode).
     * @throws IllegalStateException If the animation has been closed.
     */
    @Throws(IllegalStateException::class)
    fun advanceAndApply(deltaTime: Duration, advanceArtboard: Boolean = true): Boolean {
        check(!closed) { "Animation has been closed" }
        return riveWorker.advanceAndApplyAnimation(
            animationHandle,
            artboardHandle,
            deltaTime.inWholeNanoseconds / 1_000_000_000f,
            advanceArtboard
        )
    }

    /**
     * Sets the animation's current time position.
     *
     * @param time The time position in seconds.
     * @throws IllegalStateException If the animation has been closed.
     */
    @Throws(IllegalStateException::class)
    fun setTime(time: Float) {
        check(!closed) { "Animation has been closed" }
        riveWorker.setAnimationTime(animationHandle, time)
    }

    /**
     * Sets the animation's loop mode.
     *
     * @param loop The loop mode.
     * @throws IllegalStateException If the animation has been closed.
     */
    @Throws(IllegalStateException::class)
    fun setLoop(loop: Loop) {
        check(!closed) { "Animation has been closed" }
        riveWorker.setAnimationLoop(animationHandle, loop.value)
    }

    /**
     * Sets the animation's playback direction.
     *
     * @param direction The playback direction.
     * @throws IllegalStateException If the animation has been closed.
     */
    @Throws(IllegalStateException::class)
    fun setDirection(direction: Direction) {
        check(!closed) { "Animation has been closed" }
        riveWorker.setAnimationDirection(animationHandle, direction.value)
    }

    override fun toString(): String = "Animation($animationHandle, name=$name)"
}