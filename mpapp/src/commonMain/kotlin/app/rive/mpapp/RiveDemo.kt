package app.rive.mpapp

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.material3.Button
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExposedDropdownMenuBox
import androidx.compose.material3.ExposedDropdownMenuDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.ExposedDropdownMenuAnchorType
import androidx.compose.material3.Text
import androidx.compose.material3.TextField
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
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
import app.rive.mp.RiveFile
import app.rive.mp.compose.Fit
import app.rive.mp.compose.Rive
import app.rive.mp.compose.RiveFileSource
import app.rive.mp.compose.rememberArtboard
import app.rive.mp.compose.rememberRiveFile
import app.rive.mp.compose.rememberRiveWorker

/**
 * A demo composable that displays multiple Rive files with selection menus.
 * 
 * Features:
 * - Dropdown menu to select from multiple demo Rive files
 * - Dropdown menu to select artboard (for files with multiple artboards)
 * - Fit mode selection buttons
 * - Play/pause control
 * 
 * Files are loaded on-demand from Kotlin Multiplatform resources.
 */
@OptIn(ExperimentalRiveComposeAPI::class, ExperimentalMaterial3Api::class)
@Composable
fun RiveDemo(
    onBack: () -> Unit = {}
) {
    // State for selected file and loading
    var selectedDemoFile by remember { mutableStateOf<DemoRiveFile?>(null) }
    var loadedFileBytes by remember { mutableStateOf<ByteArray?>(null) }
    var isLoadingFile by remember { mutableStateOf(false) }
    var loadError by remember { mutableStateOf<String?>(null) }
    
    // State for artboard selection
    var selectedArtboardName by remember { mutableStateOf<String?>(null) }
    var artboardNames by remember { mutableStateOf<List<String>>(emptyList()) }
    
    // State for playback controls
    var currentFit by remember { mutableStateOf<Fit>(Fit.Contain()) }
    var isPlaying by remember { mutableStateOf(true) }
    
    // Load file bytes when selection changes
    LaunchedEffect(selectedDemoFile) {
        selectedDemoFile?.let { demoFile ->
            isLoadingFile = true
            loadError = null
            loadedFileBytes = null
            selectedArtboardName = null
            artboardNames = emptyList()
            
            try {
                loadedFileBytes = DemoRiveFile.loadBytes(demoFile)
            } catch (e: Exception) {
                loadError = "Failed to load ${demoFile.displayName}: ${e.message}"
            } finally {
                isLoadingFile = false
            }
        }
    }
    
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
    ) {
        // Header
        Text(
            text = "Rive Demo",
            style = MaterialTheme.typography.headlineSmall
        )
        
        Spacer(modifier = Modifier.height(8.dp))
        
        // File selection dropdown
        FileSelectionDropdown(
            selectedFile = selectedDemoFile,
            onFileSelected = { selectedDemoFile = it }
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
            when {
                selectedDemoFile == null -> {
                    Text("Select a Rive file from the dropdown above")
                }
                isLoadingFile -> {
                    Column(horizontalAlignment = Alignment.CenterHorizontally) {
                        CircularProgressIndicator()
                        Spacer(modifier = Modifier.height(8.dp))
                        Text("Loading ${selectedDemoFile?.displayName}...")
                    }
                }
                loadError != null -> {
                    Text(
                        text = loadError!!,
                        color = MaterialTheme.colorScheme.error
                    )
                }
                loadedFileBytes != null -> {
                    RiveContent(
                        fileBytes = loadedFileBytes!!,
                        fit = currentFit,
                        playing = isPlaying,
                        selectedArtboardName = selectedArtboardName,
                        onArtboardNamesLoaded = { names ->
                            artboardNames = names
                        }
                    )
                }
            }
        }
        
        Spacer(modifier = Modifier.height(8.dp))
        
        // Artboard selection dropdown (only show if file is loaded and has multiple artboards)
        if (artboardNames.size > 1) {
            ArtboardSelectionDropdown(
                artboardNames = artboardNames,
                selectedArtboard = selectedArtboardName,
                onArtboardSelected = { selectedArtboardName = it }
            )
            Spacer(modifier = Modifier.height(8.dp))
        }
        
        // Controls
        Column {
            // Fit mode selection
            Text("Fit Mode: ${currentFit.fitName}", style = MaterialTheme.typography.bodyMedium)
            Spacer(modifier = Modifier.height(4.dp))
            
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                Button(onClick = { currentFit = Fit.Contain() }) { Text("Contain") }
                Button(onClick = { currentFit = Fit.Cover() }) { Text("Cover") }
                Button(onClick = { currentFit = Fit.Fill }) { Text("Fill") }
            }
            
            Spacer(modifier = Modifier.height(8.dp))
            
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp)
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

/**
 * Dropdown menu for selecting a demo Rive file.
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun FileSelectionDropdown(
    selectedFile: DemoRiveFile?,
    onFileSelected: (DemoRiveFile) -> Unit
) {
    var expanded by remember { mutableStateOf(false) }
    
    ExposedDropdownMenuBox(
        expanded = expanded,
        onExpandedChange = { expanded = it }
    ) {
        TextField(
            value = selectedFile?.displayName ?: "Select a Rive file...",
            onValueChange = {},
            readOnly = true,
            label = { Text("Rive File") },
            trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = expanded) },
            modifier = Modifier
                .menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
                .fillMaxWidth()
        )
        
        ExposedDropdownMenu(
            expanded = expanded,
            onDismissRequest = { expanded = false }
        ) {
            DemoRiveFile.entries.forEach { demoFile ->
                DropdownMenuItem(
                    text = { Text(demoFile.displayName) },
                    onClick = {
                        onFileSelected(demoFile)
                        expanded = false
                    }
                )
            }
        }
    }
}

/**
 * Dropdown menu for selecting an artboard from the loaded Rive file.
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun ArtboardSelectionDropdown(
    artboardNames: List<String>,
    selectedArtboard: String?,
    onArtboardSelected: (String?) -> Unit
) {
    var expanded by remember { mutableStateOf(false) }
    
    ExposedDropdownMenuBox(
        expanded = expanded,
        onExpandedChange = { expanded = it }
    ) {
        TextField(
            value = selectedArtboard ?: "Default Artboard",
            onValueChange = {},
            readOnly = true,
            label = { Text("Artboard") },
            trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = expanded) },
            modifier = Modifier
                .menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
                .fillMaxWidth()
        )
        
        ExposedDropdownMenu(
            expanded = expanded,
            onDismissRequest = { expanded = false }
        ) {
            // Default artboard option
            DropdownMenuItem(
                text = { Text("Default Artboard") },
                onClick = {
                    onArtboardSelected(null)
                    expanded = false
                }
            )
            
            // All artboard options
            artboardNames.forEach { artboardName ->
                DropdownMenuItem(
                    text = { Text(artboardName) },
                    onClick = {
                        onArtboardSelected(artboardName)
                        expanded = false
                    }
                )
            }
        }
    }
}

/**
 * The actual Rive content that loads and displays the animation.
 */
@OptIn(ExperimentalRiveComposeAPI::class)
@Composable
private fun RiveContent(
    fileBytes: ByteArray,
    fit: Fit,
    playing: Boolean,
    selectedArtboardName: String?,
    onArtboardNamesLoaded: (List<String>) -> Unit
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
            val file = fileResult.value
            
            // Load artboard names when file is loaded
            LaunchedEffect(file) {
                val names = file.getArtboardNames()
                onArtboardNamesLoaded(names)
            }
            
            // Create artboard with optional name
            val artboard = rememberArtboard(file, selectedArtboardName)
            
            Rive(
                file = file,
                modifier = Modifier.fillMaxSize(),
                playing = playing,
                fit = fit,
                artboard = artboard,
                backgroundColor = 0xFFEEEEEE.toInt()
            )
        }
    }
}