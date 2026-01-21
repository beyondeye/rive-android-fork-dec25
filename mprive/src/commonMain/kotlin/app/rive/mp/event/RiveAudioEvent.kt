package app.rive.mp.event

/**
 * A Rive event associated with audio playback.
 *
 * This event type is triggered when an audio event occurs in the Rive animation.
 * The [assetId] property references the audio asset that should be played.
 *
 * Example:
 * ```kotlin
 * if (event is RiveAudioEvent) {
 *     println("Audio event triggered for asset: ${event.assetId}")
 *     // Handle audio playback based on assetId
 *     audioPlayer.play(event.assetId)
 * }
 * ```
 *
 * @property assetId The ID of the audio asset associated with this event.
 *
 * @see RiveGeneralEvent
 * @see RiveOpenURLEvent
 */
class RiveAudioEvent(
    name: String,
    delay: Float,
    properties: Map<String, Any>,
    data: Map<String, Any>,
    val assetId: UInt
) : RiveEvent(name, EventType.AudioEvent, delay, properties, data) {
    override fun toString(): String =
        "RiveAudioEvent(name=$name, assetId=$assetId, delay=$delay)"
}
