package app.rive.mp

import kotlin.concurrent.Volatile


/**
 * RiveLog allows configuring logging within Rive.
 *
 * By default, it uses `NoOpLogger`, which does nothing. You can set your own logger by assigning an
 * instance of `Logger` to `RiveLog.logger`. For basic logging, you can use `RiveLog.MultiplatformLogger`
 * to log using the multiplatform logger.
 */
object RiveLog {
    /**
     * The current logging implementation. Apps can override this to provide their own logging. For
     * example, in your application initialization:
     * ```kotlin
     * if (DEBUG) { RiveLog.logger = RiveLog.MultiplatformLogger() }
     * ```
     *
     * Marked as `@Volatile` to ensure immediate visibility across threads.
     */
    @Volatile
    var logger: Logger = NoOpLogger

    /**
     * Interface for logging. Implementations should provide methods for different log levels.
     *
     * Users can implement this interface to use a logging library.
     *
     * The `msg` parameter is a lambda to allow lazy evaluation of the log message, which can
     * improve performance by avoiding unnecessary string concatenation when the log level is not
     * enabled.
     */
    interface Logger {
        /** Log verbose. */
        fun v(tag: String, msg: () -> String) = Unit

        /** Log debug. */
        fun d(tag: String, msg: () -> String) = Unit

        /** Log info. */
        fun i(tag: String, msg: () -> String) = Unit

        /** Log warning. */
        fun w(tag: String, msg: () -> String) = Unit

        /** Log error. */
        fun e(tag: String, t: Throwable? = null, msg: () -> String) = Unit
    }

    @Suppress("NOTHING_TO_INLINE")
    inline fun v(tag: String, noinline msg: () -> String) = logger.v(tag, msg)

    @Suppress("NOTHING_TO_INLINE")
    inline fun d(tag: String, noinline msg: () -> String) = logger.d(tag, msg)

    @Suppress("NOTHING_TO_INLINE")
    inline fun i(tag: String, noinline msg: () -> String) = logger.i(tag, msg)

    @Suppress("NOTHING_TO_INLINE")
    inline fun w(tag: String, noinline msg: () -> String) = logger.w(tag, msg)

    @Suppress("NOTHING_TO_INLINE")
    inline fun e(tag: String, t: Throwable? = null, noinline msg: () -> String) =
        logger.e(tag, t, msg)

    /**
     * JNI bridge methods that take a String directly instead of a lambda. These are used by the C++
     * helper for efficient logging from native code.
     */
    @Suppress("UNUSED") // Used by native code
    fun logV(tag: String, msg: String) = logger.v(tag) { msg }

    @Suppress("UNUSED") // Used by native code
    fun logD(tag: String, msg: String) = logger.d(tag) { msg }

    @Suppress("UNUSED") // Used by native code
    fun logI(tag: String, msg: String) = logger.i(tag) { msg }

    @Suppress("UNUSED") // Used by native code
    fun logW(tag: String, msg: String) = logger.w(tag) { msg }

    @Suppress("UNUSED") // Used by native code
    fun logE(tag: String, msg: String) = logger.e(tag, null) { msg }

    /**
     * Implementation that uses the multiplatform logger library with lazy `msg()` evaluation.
     * This adapter wraps the de.halfbit:logger library to work with RiveLog's Logger interface.
     */
    class MultiplatformLogger : Logger {
        override fun v(tag: String, msg: () -> String) {
            //TODO v log level not supported by halfbit logger: add it to halfbit logger and fix this
            de.halfbit.logger.i(tag) { msg() }
        }

        override fun d(tag: String, msg: () -> String) {
            de.halfbit.logger.d(tag){ msg() }
        }

        override fun i(tag: String, msg: () -> String) {
            de.halfbit.logger.i(tag) { msg() }
        }

        override fun w(tag: String, msg: () -> String) {
            de.halfbit.logger.w(tag) {  msg() }
        }

        override fun e(tag: String, t: Throwable?, msg: () -> String) {
            if(t!=null) {
                de.halfbit.logger.e(tag,t){msg()}
            } else {
                de.halfbit.logger.e(tag){msg()}
            }
        }
    }

    /** Implementation that logs nothing. Used by default. */
    object NoOpLogger : Logger
}