package app.rive.mp.event

/**
 * The type of Rive event.
 *
 * These are the user-facing event types that can be reported by a state machine.
 * Internal event types (ListenerFireEvent, StateMachineFireEvent) are not exposed
 * as they are trigger mechanisms rather than reportable events.
 *
 * @property value The type key value as defined in the Rive runtime.
 */
enum class EventType(val value: Short) {
    /**
     * A general-purpose event that can carry custom properties.
     * This is the base event type (typeKey: 128).
     */
    GeneralEvent(128),

    /**
     * An event that requests opening a URL.
     * Contains [RiveOpenURLEvent.url] and [RiveOpenURLEvent.target] properties.
     * (typeKey: 131)
     */
    OpenURLEvent(131),

    /**
     * An event associated with audio playback.
     * Contains [RiveAudioEvent.assetId] referencing the audio asset.
     * (typeKey: 407)
     */
    AudioEvent(407);

    companion object {
        private val map = entries.associateBy(EventType::value)

        /**
         * Returns the [EventType] for the given type key value.
         * Returns [GeneralEvent] if the value is not recognized.
         */
        fun fromValue(value: Short) = map[value] ?: GeneralEvent
    }
}
