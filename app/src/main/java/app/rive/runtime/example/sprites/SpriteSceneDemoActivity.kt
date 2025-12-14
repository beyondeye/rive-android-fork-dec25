package app.rive.runtime.example.sprites

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.TextMeasurer
import androidx.compose.ui.text.drawText
import androidx.compose.ui.text.rememberTextMeasurer
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import app.rive.ExperimentalRiveComposeAPI
import app.rive.Result
import app.rive.RiveFile
import app.rive.RiveFileSource
import app.rive.RiveLog
import app.rive.rememberCommandQueueOrNull
import app.rive.rememberRiveFile
import app.rive.runtime.example.R
import app.rive.sprites.RiveSprite
import app.rive.sprites.RiveSpriteScene
import app.rive.sprites.SpriteScale
import app.rive.sprites.drawRiveSprites
import app.rive.sprites.rememberRiveSpriteScene
import kotlinx.coroutines.isActive
import androidx.compose.ui.graphics.drawscope.withTransform
import androidx.compose.runtime.withFrameNanos
import kotlin.time.Duration.Companion.nanoseconds

private const val TAG = "Rive/SpriteDemo"

/**
 * Demo activity showcasing the RiveSpriteScene API with multiple animated sprites.
 *
 * Features demonstrated:
 * - Multiple sprites from different Rive files
 * - Position animation (horizontal, vertical, diagonal movement)
 * - Rotation animation (0° to 180° and back)
 * - Scale animation (1.0 to 0.5 and back)
 * - Grid reference lines for visual debugging
 */
class SpriteSceneDemoActivity : ComponentActivity() {

    @OptIn(ExperimentalRiveComposeAPI::class)
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        RiveLog.logger = RiveLog.LogcatLogger()

        setContent {
            MaterialTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    SpriteSceneDemo(
                        modifier = Modifier
                            .padding(innerPadding)
                            .fillMaxSize()
                    )
                }
            }
        }
    }

    companion object {
        // Configurable Rive files per movement group
        // Change these to use different animations for each group
        val HORIZONTAL_RIV = R.raw.basketball
        val VERTICAL_RIV = R.raw.basketball
        val DIAGONAL_RIV = R.raw.basketball
    }
}

/**
 * Data class representing a sprite with movement animation.
 */
private data class MovingSprite(
    val sprite: RiveSprite,
    val startPos: Offset,
    val endPos: Offset,
    var progress: Float = 0f,
    var movingForward: Boolean = true,
    val speed: Float = 0.3f  // Progress per second (0.3 = ~3.3 seconds per trip)
) {
    /**
     * Update the sprite's position, rotation, and scale based on elapsed time.
     */
    fun update(deltaSeconds: Float) {
        val delta = speed * deltaSeconds
        if (movingForward) {
            progress += delta
            if (progress >= 1f) {
                progress = 1f
                movingForward = false
            }
        } else {
            progress -= delta
            if (progress <= 0f) {
                progress = 0f
                movingForward = true
            }
        }

        // Interpolate position
        sprite.position = Offset(
            x = lerp(startPos.x, endPos.x, progress),
            y = lerp(startPos.y, endPos.y, progress)
        )

        // Rotation: 0° to 180° as progress goes 0 to 1
        sprite.rotation = progress * 180f

        // Scale: 1.0 to 0.5 as progress goes 0 to 1
        val scale = lerp(1f, 0.5f, progress)
        sprite.scale = SpriteScale(scale)
    }
}

/**
 * Linear interpolation between two values.
 */
private fun lerp(start: Float, end: Float, fraction: Float): Float {
    return start + (end - start) * fraction
}

@OptIn(ExperimentalRiveComposeAPI::class)
@Composable
private fun SpriteSceneDemo(modifier: Modifier = Modifier) {
    val context = LocalContext.current
    val resources = context.resources
    val textMeasurer = rememberTextMeasurer()

    // Create command queue
    val errorState = remember { mutableStateOf<Throwable?>(null) }
    val commandQueue = rememberCommandQueueOrNull(errorState)

    if (commandQueue == null) {
        Box(modifier = modifier, contentAlignment = Alignment.Center) {
            Text("Error: ${errorState.value?.message ?: "Failed to create command queue"}")
        }
        return
    }

    // Create sprite scene
    val scene = rememberRiveSpriteScene(commandQueue)

    // Load Rive files for each movement group
    val horizontalFileResult = rememberRiveFile(
        RiveFileSource.RawRes(SpriteSceneDemoActivity.HORIZONTAL_RIV, resources),
        commandQueue
    )
    val verticalFileResult = rememberRiveFile(
        RiveFileSource.RawRes(SpriteSceneDemoActivity.VERTICAL_RIV, resources),
        commandQueue
    )
    val diagonalFileResult = rememberRiveFile(
        RiveFileSource.RawRes(SpriteSceneDemoActivity.DIAGONAL_RIV, resources),
        commandQueue
    )

    // Track canvas size for sprite positioning
    var canvasSize by remember { mutableStateOf(Size.Zero) }

    // Track moving sprites
    var movingSprites by remember { mutableStateOf<List<MovingSprite>>(emptyList()) }

    // Create sprites when all files are loaded and canvas size is known
    LaunchedEffect(horizontalFileResult, verticalFileResult, diagonalFileResult, canvasSize) {
        if (canvasSize == Size.Zero) return@LaunchedEffect

        val hFile = (horizontalFileResult as? Result.Success)?.value ?: return@LaunchedEffect
        val vFile = (verticalFileResult as? Result.Success)?.value ?: return@LaunchedEffect
        val dFile = (diagonalFileResult as? Result.Success)?.value ?: return@LaunchedEffect

        // Clear existing sprites
        scene.clearSprites()

        val sprites = mutableListOf<MovingSprite>()
        val spriteSize = Size(80f, 80f)
        val margin = 100f

        // Create 2 horizontal sprites
        repeat(2) { i ->
            val sprite = scene.createSprite(hFile)
            sprite.size = spriteSize
            sprite.zIndex = i

            val yPos = margin + (i + 1) * (canvasSize.height - 2 * margin) / 3
            sprites.add(
                MovingSprite(
                    sprite = sprite,
                    startPos = Offset(margin, yPos),
                    endPos = Offset(canvasSize.width - margin - spriteSize.width, yPos),
                    progress = i * 0.5f,  // Stagger start positions
                    speed = 0.25f + i * 0.1f  // Slightly different speeds
                )
            )
        }

        // Create 2 vertical sprites
        repeat(2) { i ->
            val sprite = scene.createSprite(vFile)
            sprite.size = spriteSize
            sprite.zIndex = 2 + i

            val xPos = margin + (i + 1) * (canvasSize.width - 2 * margin) / 3
            sprites.add(
                MovingSprite(
                    sprite = sprite,
                    startPos = Offset(xPos, margin),
                    endPos = Offset(xPos, canvasSize.height - margin - spriteSize.height),
                    progress = i * 0.5f,
                    speed = 0.3f + i * 0.1f
                )
            )
        }

        // Create 2 diagonal sprites
        repeat(2) { i ->
            val sprite = scene.createSprite(dFile)
            sprite.size = spriteSize
            sprite.zIndex = 4 + i

            val startX = if (i == 0) margin else canvasSize.width - margin - spriteSize.width
            val startY = margin
            val endX = if (i == 0) canvasSize.width - margin - spriteSize.width else margin
            val endY = canvasSize.height - margin - spriteSize.height

            sprites.add(
                MovingSprite(
                    sprite = sprite,
                    startPos = Offset(startX, startY),
                    endPos = Offset(endX, endY),
                    progress = i * 0.3f,
                    speed = 0.2f + i * 0.05f
                )
            )
        }

        movingSprites = sprites
        RiveLog.d(TAG) { "Created ${sprites.size} moving sprites" }
    }

    // Animation loop
    LaunchedEffect(movingSprites) {
        if (movingSprites.isEmpty()) return@LaunchedEffect

        var lastFrameTime = 0L
        while (isActive) {
            withFrameNanos { frameTimeNs ->
                val deltaTime = if (lastFrameTime == 0L) 0L else frameTimeNs - lastFrameTime
                lastFrameTime = frameTimeNs
                val deltaSeconds = deltaTime / 1_000_000_000f

                // Update sprite positions, rotations, and scales
                movingSprites.forEach { it.update(deltaSeconds) }

                // Advance Rive animations
                scene.advance(deltaTime.nanoseconds)
            }
        }
    }

    // Canvas with grid and sprites
    Canvas(modifier = modifier) {
        // Update canvas size
        if (canvasSize != size) {
            canvasSize = size
        }

        // Draw reference grid
        drawGrid(textMeasurer)

        // Draw all Rive sprites
        drawRiveSprites(scene)
    }
}

/**
 * Draw a reference grid with labeled coordinates.
 */
private fun DrawScope.drawGrid(textMeasurer: TextMeasurer) {
    val gridSpacing = 50f
    val labelSpacing = 100f
    val gridColor = Color.LightGray.copy(alpha = 0.5f)
    val labelColor = Color.Gray

    // Vertical lines
    var x = 0f
    while (x <= size.width) {
        drawLine(
            color = gridColor,
            start = Offset(x, 0f),
            end = Offset(x, size.height),
            strokeWidth = if (x % labelSpacing == 0f) 2f else 1f
        )
        x += gridSpacing
    }

    // Horizontal lines
    var y = 0f
    while (y <= size.height) {
        drawLine(
            color = gridColor,
            start = Offset(0f, y),
            end = Offset(size.width, y),
            strokeWidth = if (y % labelSpacing == 0f) 2f else 1f
        )
        y += gridSpacing
    }

    // X-axis labels (at top)
    x = 0f
    while (x <= size.width) {
        if (x % labelSpacing == 0f) {
            val text = x.toInt().toString()
            val textLayoutResult = textMeasurer.measure(
                text = text,
                style = androidx.compose.ui.text.TextStyle(
                    fontSize = 10.sp,
                    color = labelColor
                )
            )
            drawText(
                textLayoutResult = textLayoutResult,
                topLeft = Offset(x + 2f, 2f)
            )
        }
        x += gridSpacing
    }

    // Y-axis labels (at left)
    y = labelSpacing  // Skip 0 to avoid overlap with X label
    while (y <= size.height) {
        if (y % labelSpacing == 0f) {
            val text = y.toInt().toString()
            val textLayoutResult = textMeasurer.measure(
                text = text,
                style = androidx.compose.ui.text.TextStyle(
                    fontSize = 10.sp,
                    color = labelColor
                )
            )
            drawText(
                textLayoutResult = textLayoutResult,
                topLeft = Offset(2f, y + 2f)
            )
        }
        y += gridSpacing
    }
}
