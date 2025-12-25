package app.rive.mpapp

import androidx.compose.ui.window.Window
import androidx.compose.ui.window.application

/**
 * Desktop entry point.
 * This is the main function for running the app on desktop platforms.
 */
fun main() = application {
    Window(
        onCloseRequest = ::exitApplication,
        title = "Rive Multiplatform App"
    ) {
        App()
    }
}
