package app.rive.mpapp

import rive_android.mpapp.generated.resources.Res
import org.jetbrains.compose.resources.ExperimentalResourceApi

/**
 * Enumeration of available demo Rive files.
 * 
 * These files are loaded from Kotlin Multiplatform resources and are available
 * on all platforms (Android, Desktop, iOS, WasmJs).
 */
enum class DemoRiveFile(val displayName: String, val resourcePath: String) {
    MARTY("Marty", "files/marty.riv"),
    OFF_ROAD_CAR("Off Road Car Blog", "files/off_road_car_blog.riv"),
    RATING("Rating", "files/rating.riv"),
    BASKETBALL("Basketball", "files/basketball.riv"),
    BUTTON("Button", "files/button.riv"),
    BLINKO("Blinko", "files/blinko.riv"),
    EXPLORER("Explorer", "files/explorer.riv"),
    F22("F22", "files/f22.riv"),
    MASCOT("Mascot", "files/mascot.riv");

    companion object {
        /**
         * Loads the bytes of a demo Rive file from resources.
         * 
         * @param file The demo file to load.
         * @return The file contents as a ByteArray.
         */
        @OptIn(ExperimentalResourceApi::class)
        suspend fun loadBytes(file: DemoRiveFile): ByteArray {
            return Res.readBytes(file.resourcePath)
        }
    }
}