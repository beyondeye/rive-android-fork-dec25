package app.rive.mp

/**
 * Desktop (JVM) implementation of platform logging using println.
 * Format: [LEVEL/TAG] Message
 */
internal actual fun platformLog(level: LogLevel, tag: String, message: String, throwable: Throwable?) {
    val formattedMessage = if (throwable != null) {
        "[${level.short}/$tag] $message: ${throwable.message}"
    } else {
        "[${level.short}/$tag] $message"
    }
    println(formattedMessage)
}
