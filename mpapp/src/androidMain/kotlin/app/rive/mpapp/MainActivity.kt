package app.rive.mpapp

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import app.rive.mp.MpRive

/**
 * Main Activity for Android.
 * This is the entry point for the Android application.
 */
class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        // Initialize Rive before using any Rive functionality
        // This loads the native library and sets up JNI class loader for cross-thread callbacks
        MpRive.init(applicationContext)
        
        setContent {
            App()
        }
    }
}