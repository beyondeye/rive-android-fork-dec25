package app.rive.runtime.example.sprites

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxScope
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.KeyboardArrowRight
import androidx.compose.material.icons.filled.KeyboardArrowLeft
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.MutableState
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
import app.rive.RiveFileSource
import app.rive.RiveLog
import app.rive.rememberCommandQueueOrNull
import app.rive.rememberRiveFile
import app.rive.runtime.example.R
import app.rive.sprites.RiveSprite
import app.rive.sprites.SpriteRenderMode
import app.rive.sprites.SpriteScale
import app.rive.sprites.drawRiveSprites
import app.rive.sprites.rememberRiveSpriteScene
import kotlinx.coroutines.isActive
import androidx.compose.runtime.withFrameNanos
import kotlin.math.roundToInt
import kotlin.time.Duration.Companion.nanoseconds
import java.util.ArrayDeque

private const val TAG = "Rive/SpriteDemo"

class FramePerSecondCalculator(val averageFrameCount: Int = DEFAULT_FPS_AVERAGE_FRAME_COUNT,
    val updateEveryNFrames:Int=60) {
    private val frameTimeHistory = ArrayDeque<Long>(averageFrameCount)
    var updateCounter: Long=0
    fun updateFramePerSeconds(
        deltaTime: Long,
        fps: MutableState<Float>,
        frameTimeMs: MutableState<Float>)
    {
        if (deltaTime > 0) {
            if (frameTimeHistory.size >= averageFrameCount) {
                frameTimeHistory.removeFirst()
            }
            frameTimeHistory.addLast(deltaTime)
            if((updateCounter++ % updateEveryNFrames)==0L) {
                val avgDeltaNs = frameTimeHistory.average()
                val avgDeltaSeconds = avgDeltaNs / 1_000_000_000.0
                fps.value = (1.0 / avgDeltaSeconds).toFloat()
                frameTimeMs.value = (avgDeltaSeconds * 1000.0).toFloat()
            }
        }
    }

    companion object {
        // Number of frames to average for FPS calculation
        const val DEFAULT_FPS_AVERAGE_FRAME_COUNT = 60

    }
}
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
    fun update(deltaSeconds: Float, enableRotation: Boolean = true, enableScaling: Boolean = true, enableMovement: Boolean = true) {
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

        // Interpolate position if movement is enabled, otherwise use startPos
        sprite.position = if (enableMovement) {
            Offset(
                x = lerp(startPos.x, endPos.x, progress),
                y = lerp(startPos.y, endPos.y, progress)
            )
        } else {
            startPos
        }

        // Rotation: 0° to 180° as progress goes 0 to 1, or fixed at 0 if disabled
        sprite.rotation = if (enableRotation) progress * 180f else 0f

        // Scale: 1.0 to 0.5 as progress goes 0 to 1, or fixed at 1.0 if disabled
        val scale = if (enableScaling) lerp(1f, 0.5f, progress) else 1f
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

    // FPS tracking
    val fps = remember { mutableStateOf(0f) }
    val frameTimeMs = remember { mutableStateOf(0f) }
    val fpsCalculator = remember { FramePerSecondCalculator() }

    // Animation control switches
    var enableRotation by remember { mutableStateOf(true) }
    var enableScaling by remember { mutableStateOf(true) }
    var enableMovement by remember { mutableStateOf(true) }
    var frameCounter by remember { mutableStateOf(0L) }
    
    // Render mode switch
    var useBatchRendering by remember { mutableStateOf(false) }
    
    // Sprite configuration controls
    var spritesPerDirection by remember { mutableStateOf(2) }
    var spriteSize by remember { mutableStateOf(50) }

    // Create sprites when all files are loaded and canvas size is known
    LaunchedEffect(horizontalFileResult, verticalFileResult, diagonalFileResult, canvasSize, spritesPerDirection, spriteSize) {
        if (canvasSize == Size.Zero) return@LaunchedEffect

        val hFile = (horizontalFileResult as? Result.Success)?.value ?: return@LaunchedEffect
        val vFile = (verticalFileResult as? Result.Success)?.value ?: return@LaunchedEffect
        val dFile = (diagonalFileResult as? Result.Success)?.value ?: return@LaunchedEffect

        // Clear existing sprites
        scene.clearSprites()

        val sprites = mutableListOf<MovingSprite>()
        val spriteSizeValue = Size(spriteSize.toFloat(), spriteSize.toFloat())
        val margin = 100f

        // Create horizontal sprites
        repeat(spritesPerDirection) { i ->
            val sprite = scene.createSprite(hFile)
            sprite.size = spriteSizeValue
            sprite.zIndex = i

            val yPos = margin + (i + 1) * (canvasSize.height - 2 * margin) / (spritesPerDirection + 1)
            val newMovingSprite = MovingSprite(
                sprite = sprite,
                startPos = Offset(margin, yPos),
                endPos = Offset(canvasSize.width - margin - spriteSizeValue.width, yPos),
                progress = (i.toFloat() / spritesPerDirection.coerceAtLeast(1)) * 0.5f,  // Stagger start positions
                speed = 0.25f + i * 0.05f  // Slightly different speeds
            )
            sprites.add(newMovingSprite)
            RiveLog.d(TAG) { "*NEW SPRITE* Horiz. at starting pos: ${newMovingSprite.startPos.x}, ${newMovingSprite.startPos.y}, progress: ${newMovingSprite.progress}" }
        }

        // Create vertical sprites
        repeat(spritesPerDirection) { i ->
            val sprite = scene.createSprite(vFile)
            sprite.size = spriteSizeValue
            sprite.zIndex = spritesPerDirection + i

            val xPos = margin + (i + 1) * (canvasSize.width - 2 * margin) / (spritesPerDirection + 1)

            val newMovingSprite = MovingSprite(
                sprite = sprite,
                startPos = Offset(xPos, margin),
                endPos = Offset(xPos, canvasSize.height - margin - spriteSizeValue.height),
                progress = (i.toFloat() / spritesPerDirection.coerceAtLeast(1)) * 0.5f,
                speed = 0.3f + i * 0.05f
            )
            sprites.add(newMovingSprite)
            RiveLog.d(TAG) { "*NEW SPRITE* Vert. at starting pos: ${newMovingSprite.startPos.x}, ${newMovingSprite.startPos.y}, progress: ${newMovingSprite.progress}" }
        }

        // Create diagonal sprites
        repeat(spritesPerDirection) { i ->
            val sprite = scene.createSprite(dFile)
            sprite.size = spriteSizeValue
            sprite.zIndex = 2 * spritesPerDirection + i

            val startX = if (i % 2 == 0) margin else canvasSize.width - margin - spriteSizeValue.width
            val startY = margin + (i / 2) * (canvasSize.height - 2 * margin) / ((spritesPerDirection + 1) / 2).coerceAtLeast(1)
            val endX = if (i % 2 == 0) canvasSize.width - margin - spriteSizeValue.width else margin
            val endY = canvasSize.height - margin - spriteSizeValue.height - (i / 2) * (canvasSize.height - 2 * margin) / ((spritesPerDirection + 1) / 2).coerceAtLeast(1)

            val newMovingSprite = MovingSprite(
                sprite = sprite,
                startPos = Offset(startX, startY),
                endPos = Offset(endX, endY),
                progress = (i.toFloat() / spritesPerDirection.coerceAtLeast(1)) * 0.3f,
                speed = 0.2f + i * 0.03f
            )
            sprites.add(newMovingSprite)
            RiveLog.d(TAG) { "*NEW SPRITE* Diag. at starting pos: ${newMovingSprite.startPos.x}, ${newMovingSprite.startPos.y}, progress: ${newMovingSprite.progress}" }
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
                frameCounter++ // Force recomposition
                val deltaTime = if (lastFrameTime == 0L) 0L else frameTimeNs - lastFrameTime
                lastFrameTime = frameTimeNs
                val deltaSeconds = deltaTime / 1_000_000_000f

                // Update FPS rolling average
                fpsCalculator.updateFramePerSeconds(deltaTime,fps, frameTimeMs)

                // Update sprite positions, rotations, and scales
                movingSprites.forEach { it.update(deltaSeconds, enableRotation, enableScaling, enableMovement) }

                // Advance Rive animations
                scene.advance(deltaTime.nanoseconds)
            }
        }
    }

    // Canvas with grid and sprites
    Box(modifier = modifier) {
        Canvas(modifier = Modifier.fillMaxSize()) {
            @Suppress("UNUSED_EXPRESSION")
            frameCounter // Just reading this forces the Canvas to redraw
            // Update canvas size
            if (canvasSize != size) {
                canvasSize = size
            }

            // Draw reference grid
            drawGrid(textMeasurer)

            // Draw all Rive sprites with selected render mode
            drawRiveSprites(
                scene,
                renderMode = if (useBatchRendering) SpriteRenderMode.BATCH else SpriteRenderMode.PER_SPRITE
            )
        }

        ControlPanel(
            fps = fps,
            frameTimeMs = frameTimeMs,
            spritesPerDirection = spritesPerDirection,
            onSpritesPerDirectionChange = { spritesPerDirection = it },
            spriteSize = spriteSize,
            onSpriteSizeChange = { spriteSize = it },
            enableRotation = enableRotation,
            onEnableRotationChange = { enableRotation = it },
            enableScaling = enableScaling,
            onEnableScalingChange = { enableScaling = it },
            enableMovement = enableMovement,
            onEnableMovementChange = { enableMovement = it },
            useBatchRendering = useBatchRendering,
            onUseBatchRenderingChange = { useBatchRendering = it }
        )
    }
}

/**
 * A reusable numeric control with plus/minus buttons.
 */
@Composable
private fun NumericControl(
    label: String,
    value: Int,
    onValueChange: (Int) -> Unit,
    min: Int,
    max: Int,
    step: Int = 1
) {
    Row(
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(
            text = label,
            color = Color.Black,
            fontSize = 14.sp
        )
        IconButton(
            onClick = { if (value > min) onValueChange(value - step) },
            modifier = Modifier.size(32.dp),
            enabled = value > min
        ) {
            Icon(
                imageVector = Icons.Default.KeyboardArrowLeft,
                contentDescription = "Decrease $label",
                tint = if (value > min) Color.Black else Color.Gray
            )
        }
        Text(
            text = value.toString(),
            color = Color.Black,
            fontSize = 14.sp,
            modifier = Modifier.width(40.dp),
            textAlign = androidx.compose.ui.text.style.TextAlign.Center
        )
        IconButton(
            onClick = { if (value < max) onValueChange(value + step) },
            modifier = Modifier.size(32.dp),
            enabled = value < max
        ) {
            Icon(
                imageVector = Icons.Default.KeyboardArrowRight,
                contentDescription = "Increase $label",
                tint = if (value < max) Color.Black else Color.Gray
            )
        }
    }
}

@Composable
private fun BoxScope.ControlPanel(
    fps: MutableState<Float>,
    frameTimeMs: MutableState<Float>,
    spritesPerDirection: Int,
    onSpritesPerDirectionChange: (Int) -> Unit,
    spriteSize: Int,
    onSpriteSizeChange: (Int) -> Unit,
    enableRotation: Boolean,
    onEnableRotationChange: (Boolean) -> Unit,
    enableScaling: Boolean,
    onEnableScalingChange: (Boolean) -> Unit,
    enableMovement: Boolean,
    onEnableMovementChange: (Boolean) -> Unit,
    useBatchRendering: Boolean,
    onUseBatchRenderingChange: (Boolean) -> Unit
) {
    Column(
        modifier = Modifier
            .align(Alignment.BottomEnd)
            .padding(16.dp),
        horizontalAlignment = Alignment.End
    ) {
        // Numeric controls row (sprites per direction and size)
        Row(
            modifier = Modifier
                .background(Color.White.copy(alpha = 0.7f), RoundedCornerShape(4.dp))
                .padding(horizontal = 8.dp, vertical = 4.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            NumericControl(
                label = "Sprts",
                value = spritesPerDirection,
                onValueChange = onSpritesPerDirectionChange,
                min = 1,
                max = 20,
                step = 1
            )
            NumericControl(
                label = "sz",
                value = spriteSize,
                onValueChange = onSpriteSizeChange,
                min = 20,
                max = 300,
                step = 10
            )
        }
        
        Spacer(modifier = Modifier.padding(top = 8.dp))
        
        // Switches row
        Row(
            modifier = Modifier
                .background(Color.White.copy(alpha = 0.7f), RoundedCornerShape(4.dp))
                .padding(horizontal = 8.dp, vertical = 4.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(
                    text = "Rot",
                    color = Color.Black,
                    fontSize = 14.sp
                )
                Spacer(modifier = Modifier.width(4.dp))
                Switch(
                    checked = enableRotation,
                    onCheckedChange = onEnableRotationChange
                )
            }
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(
                    text = "Scale",
                    color = Color.Black,
                    fontSize = 14.sp
                )
                Spacer(modifier = Modifier.width(4.dp))
                Switch(
                    checked = enableScaling,
                    onCheckedChange = onEnableScalingChange
                )
            }
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(
                    text = "Move",
                    color = Color.Black,
                    fontSize = 14.sp
                )
                Spacer(modifier = Modifier.width(4.dp))
                Switch(
                    checked = enableMovement,
                    onCheckedChange = onEnableMovementChange
                )
            }
        }
        
        // FPS indicator with Batch switch
        FPSIndicator(
            fps = fps,
            frameTimeMs = frameTimeMs,
            useBatchRendering = useBatchRendering,
            onUseBatchRenderingChange = onUseBatchRenderingChange
        )
    }
}

@Composable
private fun FPSIndicator(
    fps: MutableState<Float>,
    frameTimeMs: MutableState<Float>,
    useBatchRendering: Boolean,
    onUseBatchRenderingChange: (Boolean) -> Unit
) {
    Row(
        modifier = Modifier
            .padding(top = 8.dp)
            .background(Color.White.copy(alpha = 0.7f), RoundedCornerShape(4.dp))
            .padding(horizontal = 8.dp, vertical = 4.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        Text(
            text = "FPS: ${fps.value.roundToInt()} (${frameTimeMs.value.roundToInt()} ms)",
            color = Color.Black,
            fontSize = 14.sp
        )
        Text(
            text = "Batch",
            color = Color.Black,
            fontSize = 14.sp
        )
        Switch(
            checked = useBatchRendering,
            onCheckedChange = onUseBatchRenderingChange
        )
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
