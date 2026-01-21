package app.rive.mp.event

/**
 * Internal data class for transferring event data from native layer.
 * Used by the JNI bindings to create typed event objects.
 */
internal data class RiveEventData(
    val name: String,
    val typeCode: Short,
    val delay: Float,
    val url: String,
    val target: String,
    val properties: Map<String, Any>
) {
    /**
     * Converts this data object to the appropriate RiveEvent subclass.
     */
    fun toRiveEvent(): RiveEvent {
        val type = EventType.fromValue(typeCode)
        val data = buildMap<String, Any> {
            put("name", name)
            put("type", type.value)
            put("delay", delay)
            putAll(properties)
            if (url.isNotEmpty()) put("url", url)
            if (target.isNotEmpty()) put("target", target)
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
            EventType.GeneralEvent -> RiveGeneralEvent(
                name = name,
                delay = delay,
                properties = properties,
                data = data
            )
        }
    }
}
