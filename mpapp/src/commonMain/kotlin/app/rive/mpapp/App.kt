package app.rive.mpapp

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import app.rive.mprive.getRivePlatform

/**
 * Represents the different screens in the app.
 */
enum class Screen {
    Main,
    RiveDemo
}

/**
 * Main application composable.
 * This is shared across all platforms.
 */
@Composable
fun App(
    riveFileBytes: ByteArray? = null
) {
    var currentScreen by remember { mutableStateOf(Screen.Main) }
    
    MaterialTheme {
        Surface(
            modifier = Modifier.fillMaxSize(),
            color = MaterialTheme.colorScheme.background
        ) {
            when (currentScreen) {
                Screen.Main -> MainScreen(
                    onNavigateToDemo = { currentScreen = Screen.RiveDemo },
                    hasRiveFile = riveFileBytes != null
                )
                Screen.RiveDemo -> RiveDemo(
                    riveFileBytes = riveFileBytes,
                    onBack = { currentScreen = Screen.Main }
                )
            }
        }
    }
}

@Composable
fun MainScreen(
    onNavigateToDemo: () -> Unit = {},
    hasRiveFile: Boolean = false
) {
    val platform = remember { getRivePlatform() }
    var initialized by remember { mutableStateOf(false) }
    
    LaunchedEffect(Unit) {
        initialized = platform.initialize()
    }
    
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center
    ) {
        Text(
            text = "Rive Multiplatform App",
            style = MaterialTheme.typography.headlineMedium
        )
        
        Spacer(modifier = Modifier.height(16.dp))
        
        Text(
            text = "Platform: ${platform.getPlatformName()}",
            style = MaterialTheme.typography.bodyLarge
        )
        
        Spacer(modifier = Modifier.height(8.dp))
        
        Text(
            text = "Initialized: $initialized",
            style = MaterialTheme.typography.bodyMedium,
            color = if (initialized) {
                MaterialTheme.colorScheme.primary
            } else {
                MaterialTheme.colorScheme.error
            }
        )
        
        Spacer(modifier = Modifier.height(24.dp))
        
        // Rive Demo Button
        Button(
            onClick = onNavigateToDemo,
            enabled = initialized && hasRiveFile,
            modifier = Modifier.fillMaxWidth(0.8f)
        ) {
            Text("Open Rive Demo")
        }
        
        if (!hasRiveFile) {
            Spacer(modifier = Modifier.height(8.dp))
            Text(
                text = "(No Rive file loaded)",
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.error
            )
        }
        
        Spacer(modifier = Modifier.height(24.dp))
        
        Card(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 16.dp)
        ) {
            Column(
                modifier = Modifier.padding(16.dp)
            ) {
                Text(
                    text = "About",
                    style = MaterialTheme.typography.titleMedium
                )
                Spacer(modifier = Modifier.height(8.dp))
                Text(
                    text = "This is a Compose Multiplatform demo app for testing the mprive library's Rive composable.",
                    style = MaterialTheme.typography.bodySmall
                )
            }
        }
    }
}