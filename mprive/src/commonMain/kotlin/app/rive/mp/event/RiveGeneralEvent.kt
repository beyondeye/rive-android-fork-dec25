package app.rive.mp.event

/**
 * A general-purpose Rive event.
 *
 * General events can carry arbitrary properties defined in the Rive editor.
 * Access properties via the [properties] map.
 *
 * Example:
 * ```kotlin
 * if (event is RiveGeneralEvent) {
 *     val rating = event.properties["rating"] as? Number
 *     println("Rating: $rating")
 * }
 * ```
 *
 * @see RiveOpenURLEvent
 */
class RiveGeneralEvent(
    name: String,
    delay: Float,
    properties: Map<String, Any>,
    data: Map<String, Any>
) : RiveEvent(name, EventType.GeneralEvent, delay, properties, data) {
    override fun toString(): String =
        "RiveGeneralEvent(name=$name, delay=$delay, properties=$properties)"
}
