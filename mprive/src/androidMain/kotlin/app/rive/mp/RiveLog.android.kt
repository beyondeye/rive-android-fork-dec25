package app.rive.mp

import android.util.Log

/**
 * Android implementation of platform logging using android.util.Log.
 */
internal actual fun platformLog(level: LogLevel, tag: String, message: String, throwable: Throwable?) {
    when (level) {
        LogLevel.Verbose -> Log.v(tag, message)
        LogLevel.Debug -> Log.d(tag, message)
        LogLevel.Info -> Log.i(tag, message)
        LogLevel.Warning -> {
            if (throwable != null) {
                Log.w(tag, "$message: ${throwable.message}")
            } else {
                Log.w(tag, message)
            }
        }
        LogLevel.Error -> {
            if (throwable != null) {
                Log.e(tag, "$message: ${throwable.message}")
            } else {
                Log.e(tag, message)
            }
        }
    }
}
