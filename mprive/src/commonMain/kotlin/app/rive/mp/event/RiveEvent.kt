package app.rive.mp.event

/**
 * A Rive event emitted by a state machine during animation playback.
 *
 * Events can carry custom properties and be used to trigger application logic
 * in response to animation state changes.
 *
 * This is a pure Kotlin data class (no native pointers) suitable for
 * Kotlin Multiplatform.
 *
 * Use `when` to handle different event types:
 * ```kotlin
 * when (event) {
 *     is RiveGeneralEvent -> handleGeneral(event)
 *     is RiveOpenURLEvent -> openUrl(event.url)
 *     is RiveAudioEvent -> playAudio(event.assetId)
 * }
 * ```
 *
 * @property name Name of the event as defined in Rive.
 * @property type Type of event (see [EventType]).
 * @property delay Delay in seconds since the advance that triggered the event.
 * @property properties Custom properties attached to the event.
 * @property data All event data as a flat map.
 *
 * @see RiveGeneralEvent
 * @see RiveOpenURLEvent
 * @see RiveAudioEvent
 */
open class RiveEvent(
    val name: String,
    val type: EventType,
    val delay: Float,
    val properties: Map<String, Any>,
    val data: Map<String, Any>
) {
    override fun toString(): String = "RiveEvent(name=$name, type=$type, delay=$delay)"
}
