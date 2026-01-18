package app.rive.mpapp

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.runtime.remember
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
            // Load a simple Rive file from the app module's raw resources
            // For testing, we'll try to load from the shared raw resources
            val riveFileBytes = remember {
                loadRiveFileFromResources()
            }
            App(riveFileBytes = riveFileBytes)
        }
    }
    
    /**
     * Loads a Rive file from raw resources.
     * Returns null if the file cannot be loaded.
     */
    private fun loadRiveFileFromResources(): ByteArray? {
        return try {
            // Try to load rating.riv - a simple interactive animation
            // This file should be copied to mpapp/src/androidMain/res/raw/
            val resourceId = resources.getIdentifier("rating", "raw", packageName)
            if (resourceId != 0) {
                resources.openRawResource(resourceId).use { it.readBytes() }
            } else {
                // Fallback: try to load from app module resources if available
                android.util.Log.w("MainActivity", "No Rive file found in resources. Add rating.riv to res/raw/")
                null
            }
        } catch (e: Exception) {
            android.util.Log.e("MainActivity", "Failed to load Rive file: ${e.message}")
            null
        }
    }
}