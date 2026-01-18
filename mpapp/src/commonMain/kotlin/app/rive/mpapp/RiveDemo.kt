package app.rive.mpapp

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import app.rive.mp.ExperimentalRiveComposeAPI
import app.rive.mp.Result
import app.rive.mp.compose.Fit
import app.rive.mp.compose.Rive
import app.rive.mp.compose.RiveFileSource
import app.rive.mp.compose.rememberRiveFile
import app.rive.mp.compose.rememberRiveWorker

/**
 * A simple demo composable that tests the Rive composable with different configurations.
 * 
 * This is used for integration testing of the Rive.kt implementation.
 */
@OptIn(ExperimentalRiveComposeAPI::class)
@Composable
fun RiveDemo(
    riveFileBytes: ByteArray?,
    onBack: () -> Unit = {}
) {
    var currentFit by remember { mutableStateOf<Fit>(Fit.Contain()) }
    var isPlaying by remember { mutableStateOf(true) }
    
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
    ) {
        // Header
        Text(
            text = "Rive Demo - Integration Test",
            style = MaterialTheme.typography.headlineSmall
        )
        
        Spacer(modifier = Modifier.height(8.dp))
        
        // Rive animation area
        Box(
            modifier = Modifier
                .weight(1f)
                .fillMaxWidth()
                .background(Color.LightGray),
            contentAlignment = Alignment.Center
        ) {
            if (riveFileBytes == null) {
                Text("No file loaded")
            } else {
                RiveContent(
                    fileBytes = riveFileBytes,
                    fit = currentFit,
                    playing = isPlaying
                )
            }
        }
        
        Spacer(modifier = Modifier.height(16.dp))
        
        // Controls
        Column {
            // Fit mode selection
            Text("Fit Mode: ${currentFit.fitName}", style = MaterialTheme.typography.bodyMedium)
            Spacer(modifier = Modifier.height(4.dp))
            
            androidx.compose.foundation.layout.Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = androidx.compose.foundation.layout.Arrangement.spacedBy(8.dp)
            ) {
                Button(onClick = { currentFit = Fit.Contain() }) { Text("Contain") }
                Button(onClick = { currentFit = Fit.Cover() }) { Text("Cover") }
                Button(onClick = { currentFit = Fit.Fill }) { Text("Fill") }
            }
            
            Spacer(modifier = Modifier.height(8.dp))
            
            androidx.compose.foundation.layout.Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = androidx.compose.foundation.layout.Arrangement.spacedBy(8.dp)
            ) {
                Button(onClick = { currentFit = Fit.Layout() }) { Text("Layout") }
                Button(onClick = { currentFit = Fit.None() }) { Text("None") }
            }
            
            Spacer(modifier = Modifier.height(16.dp))
            
            // Play/Pause control
            Button(
                onClick = { isPlaying = !isPlaying },
                modifier = Modifier.fillMaxWidth()
            ) {
                Text(if (isPlaying) "Pause" else "Play")
            }
            
            Spacer(modifier = Modifier.height(8.dp))
            
            // Back button
            Button(
                onClick = onBack,
                modifier = Modifier.fillMaxWidth()
            ) {
                Text("Back")
            }
        }
    }
}

@OptIn(ExperimentalRiveComposeAPI::class)
@Composable
private fun RiveContent(
    fileBytes: ByteArray,
    fit: Fit,
    playing: Boolean
) {
    val riveWorker = rememberRiveWorker()
    val fileResult = rememberRiveFile(
        source = RiveFileSource.Bytes(fileBytes),
        riveWorker = riveWorker
    )
    
    when (fileResult) {
        is Result.Loading -> {
            CircularProgressIndicator()
        }
        is Result.Error -> {
            Text(
                text = "Error: ${fileResult.throwable.message}",
                color = MaterialTheme.colorScheme.error
            )
        }
        is Result.Success -> {
            Rive(
                file = fileResult.value,
                modifier = Modifier.fillMaxSize(),
                playing = playing,
                fit = fit,
                backgroundColor = 0xFFEEEEEE.toInt()
            )
        }
    }
}