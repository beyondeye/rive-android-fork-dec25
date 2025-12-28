package app.rive.mp

@RequiresOptIn(
    level = RequiresOptIn.Level.WARNING,
    message = "The Rive Compose API is experimental and may change in the future. Opt-in is required."
)
@Retention(AnnotationRetention.BINARY)
@Target(AnnotationTarget.CLASS, AnnotationTarget.FUNCTION, AnnotationTarget.TYPEALIAS)
annotation class ExperimentalRiveComposeAPI
