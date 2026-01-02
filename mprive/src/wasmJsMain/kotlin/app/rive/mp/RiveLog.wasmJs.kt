package app.rive.mp

/**
 * Wasm/JS implementation of platform logging using console.
 * Format: [LEVEL/TAG] Message
 */
internal actual fun platformLog(level: LogLevel, tag: String, message: String, throwable: Throwable?) {
    val formattedMessage = if (throwable != null) {
        "[${level.short}/$tag] $message: ${throwable.message}"
    } else {
        "[${level.short}/$tag] $message"
    }
    when (level) {
        LogLevel.Verbose, LogLevel.Debug -> console.log(formattedMessage)
        LogLevel.Info -> console.info(formattedMessage)
        LogLevel.Warning -> console.warn(formattedMessage)
        LogLevel.Error -> console.error(formattedMessage)
    }
}

// External declarations for JavaScript console
private external object console {
    fun log(message: String)
    fun info(message: String)
    fun warn(message: String)
    fun error(message: String)
}
