package app.rive.mp.event

/**
 * The type of Rive event.
 *
 * Matches the reference implementation in kotlin/src/main.
 */
enum class EventType(val value: Short) {
    GeneralEvent(128),
    OpenURLEvent(131);

    companion object {
        private val map = entries.associateBy(EventType::value)
        fun fromValue(value: Short) = map[value] ?: GeneralEvent
    }
}
