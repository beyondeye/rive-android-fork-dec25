package app.rive.mp.event

/**
 * Internal data class for transferring event data from native layer.
 * Used by the JNI bindings to create typed event objects.
 *
 * @property name The event name.
 * @property typeCode The event type key (128=General, 131=OpenURL, 407=Audio).
 * @property delay Delay in seconds since the advance that triggered the event.
 * @property url URL for OpenURLEvent (empty for other types).
 * @property targetValue Numeric target value for OpenURLEvent (0=_blank, 1=_parent, 2=_self, 3=_top).
 * @property assetId Asset ID for AudioEvent (0 for other types).
 * @property properties Custom properties attached to the event.
 */
internal data class RiveEventData(
    val name: String,
    val typeCode: Short,
    val delay: Float,
    val url: String,
    val targetValue: Int,
    val assetId: UInt,
    val properties: Map<String, Any>
) {
    /**
     * Converts this data object to the appropriate RiveEvent subclass.
     */
    fun toRiveEvent(): RiveEvent {
        val type = EventType.fromValue(typeCode)
        val target = OpenUrlTarget.fromValue(targetValue)

        val data = buildMap<String, Any> {
            put("name", name)
            put("type", type.value)
            put("delay", delay)
            putAll(properties)
            if (url.isNotEmpty()) {
                put("url", url)
                put("target", target.targetName)
            }
            if (assetId != 0u) {
                put("assetId", assetId)
            }
        }

        return when (type) {
            EventType.OpenURLEvent -> RiveOpenURLEvent(
                name = name,
                delay = delay,
                properties = properties,
                data = data,
                url = url,
                target = target
            )
            EventType.AudioEvent -> RiveAudioEvent(
                name = name,
                delay = delay,
                properties = properties,
                data = data,
                assetId = assetId
            )
            EventType.GeneralEvent -> RiveGeneralEvent(
                name = name,
                delay = delay,
                properties = properties,
                data = data
            )
        }
    }
}
