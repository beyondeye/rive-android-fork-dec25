package app.rive.mp

import platform.Foundation.NSLog

/**
 * iOS implementation of platform logging using NSLog.
 * Format: [LEVEL/TAG] Message
 */
internal actual fun platformLog(level: LogLevel, tag: String, message: String, throwable: Throwable?) {
    val formattedMessage = if (throwable != null) {
        "[${level.short}/$tag] $message: ${throwable.message}"
    } else {
        "[${level.short}/$tag] $message"
    }
    NSLog("%@", formattedMessage)
}
