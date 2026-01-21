package app.rive.mp.compose

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.style.TextAlign
import app.rive.mp.Artboard
import app.rive.mp.RiveFile
import app.rive.mp.RiveLog
import app.rive.mp.StateMachine
import app.rive.mp.ViewModelInstance

private const val TAG = "Rive/Desktop"

/**
 * Desktop implementation of the Rive composable.
 * 
 * **Note**: This is a stub implementation. Full desktop rendering support
 * with Skia will be implemented in Phase 4.
 */
@Composable
actual fun Rive(
    file: RiveFile,
    modifier: Modifier,
    playing: Boolean,
    artboard: Artboard?,
    stateMachine: StateMachine?,
    viewModelInstance: ViewModelInstance?,
    fit: Fit,
    backgroundColor: Int,
    pointerInputMode: RivePointerInputMode
) {
    // Log a warning that desktop is not yet implemented
    RiveLog.w(TAG) { "Desktop Rive rendering is not yet implemented. Coming in Phase 4." }
    
    // Show a placeholder
    Box(
        modifier = modifier
            .fillMaxSize()
            .background(Color(backgroundColor)),
        contentAlignment = Alignment.Center
    ) {
        Text(
            text = "Rive Desktop\nComing in Phase 4",
            textAlign = TextAlign.Center,
            color = Color.Gray
        )
    }
}