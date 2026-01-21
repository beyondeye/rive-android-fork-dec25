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
 *     // Use target to determine how to open
 *     when (event.target) {
 *         OpenUrlTarget.Blank -> openInNewTab(event.url)
 *         OpenUrlTarget.Self -> navigateTo(event.url)
 *         else -> uriHandler.openUri(event.url)
 *     }
 * }
 * ```
 *
 * @property url The URL to open.
 * @property target The target window/context for the URL.
 *
 * @see OpenUrlTarget
 * @see RiveGeneralEvent
 * @see RiveAudioEvent
 */
class RiveOpenURLEvent(
    name: String,
    delay: Float,
    properties: Map<String, Any>,
    data: Map<String, Any>,
    val url: String,
    val target: OpenUrlTarget
) : RiveEvent(name, EventType.OpenURLEvent, delay, properties, data) {

    /**
     * The target as a string (e.g., "_blank", "_self").
     * Useful for passing to web APIs or platform-specific URL handlers.
     */
    val targetName: String get() = target.targetName

    override fun toString(): String =
        "RiveOpenURLEvent(name=$name, url=$url, target=${target.targetName})"
}
