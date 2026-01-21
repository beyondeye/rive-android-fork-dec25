package app.rive.mp.event

/**
 * A Rive event that requests opening a URL.
 *
 * This event type is specifically designed for "Open URL" actions in Rive.
 * The application should handle this event by opening the URL in an appropriate way.
 *
 * Example:
 * ```kotlin
 * if (event is RiveOpenURLEvent) {
 *     uriHandler.openUri(event.url)
 * }
 * ```
 *
 * @property url The URL to open.
 * @property target The target window/context for the URL (e.g., "_blank", "_self").
 *
 * @see RiveGeneralEvent
 */
class RiveOpenURLEvent(
    name: String,
    delay: Float,
    properties: Map<String, Any>,
    data: Map<String, Any>,
    val url: String,
    val target: String
) : RiveEvent(name, EventType.OpenURLEvent, delay, properties, data) {
    override fun toString(): String =
        "RiveOpenURLEvent(name=$name, url=$url, target=$target)"
}
