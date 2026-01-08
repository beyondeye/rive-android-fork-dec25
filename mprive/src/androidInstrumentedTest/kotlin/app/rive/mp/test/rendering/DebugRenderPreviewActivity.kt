package app.rive.mp.test.rendering

import android.graphics.Bitmap
import android.graphics.Color
import android.os.Bundle
import android.view.Gravity
import android.widget.FrameLayout
import android.widget.ImageView
import android.widget.TextView
import androidx.activity.ComponentActivity
import androidx.lifecycle.lifecycleScope
import app.rive.mp.RiveLog
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import kotlin.time.Duration
import kotlin.time.Duration.Companion.milliseconds
import kotlin.time.Duration.Companion.seconds

private const val TAG = "Rive/PreviewActivity"

/**
 * Activity that displays rendered content for debugging purposes.
 * 
 * This activity is used by [DebugRenderPreview] to show either:
 * - A static bitmap preview
 * - A real-time animation loop
 * 
 * The activity auto-closes after the specified duration.
 */
class DebugRenderPreviewActivity : ComponentActivity() {
    
    companion object {
        /** Pending bitmap to display (for static preview) */
        @Volatile
        var pendingBitmap: Bitmap? = null
        
        /** Title to display in the preview */
        @Volatile
        var pendingTitle: String = "Preview"
        
        /** Duration to display the preview */
        @Volatile
        var pendingDuration: Duration = 3.seconds
        
        /** Animation loop configuration (null for static preview) */
        @Volatile
        var animationLoop: AnimationLoopConfig? = null
    }
    
    private lateinit var imageView: ImageView
    private lateinit var titleView: TextView
    private lateinit var fpsView: TextView
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        RiveLog.d(TAG) { "Creating preview activity" }
        
        // Create layout
        val rootLayout = FrameLayout(this).apply {
            setBackgroundColor(Color.DKGRAY)
        }
        
        // Title text
        titleView = TextView(this).apply {
            text = pendingTitle
            setTextColor(Color.WHITE)
            textSize = 18f
            setPadding(16, 16, 16, 16)
        }
        rootLayout.addView(titleView, FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.WRAP_CONTENT,
            FrameLayout.LayoutParams.WRAP_CONTENT
        ).apply {
            gravity = Gravity.TOP or Gravity.START
        })
        
        // Image view for rendered content
        imageView = ImageView(this).apply {
            scaleType = ImageView.ScaleType.FIT_CENTER
            setBackgroundColor(Color.BLACK)
        }
        rootLayout.addView(imageView, FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT,
            FrameLayout.LayoutParams.MATCH_PARENT
        ).apply {
            setMargins(0, 80, 0, 80)
        })
        
        // FPS counter for animations
        fpsView = TextView(this).apply {
            text = ""
            setTextColor(Color.YELLOW)
            textSize = 14f
            setPadding(16, 16, 16, 16)
        }
        rootLayout.addView(fpsView, FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.WRAP_CONTENT,
            FrameLayout.LayoutParams.WRAP_CONTENT
        ).apply {
            gravity = Gravity.BOTTOM or Gravity.END
        })
        
        setContentView(rootLayout)
        
        // Start preview
        if (animationLoop != null) {
            startAnimationLoop()
        } else {
            showStaticBitmap()
        }
    }
    
    /**
     * Display a static bitmap and auto-close after duration.
     */
    private fun showStaticBitmap() {
        val bitmap = pendingBitmap
        if (bitmap == null) {
            RiveLog.w(TAG) { "No pending bitmap to display" }
            finish()
            return
        }
        
        RiveLog.d(TAG) { "Showing static bitmap: ${bitmap.width}x${bitmap.height}" }
        imageView.setImageBitmap(bitmap)
        
        // Auto-close after duration
        lifecycleScope.launch {
            delay(pendingDuration)
            RiveLog.d(TAG) { "Auto-closing static preview" }
            finish()
        }
    }
    
    /**
     * Run an animation loop, continuously rendering and displaying frames.
     */
    private fun startAnimationLoop() {
        val config = animationLoop
        if (config == null) {
            RiveLog.w(TAG) { "No animation loop config" }
            finish()
            return
        }
        
        RiveLog.d(TAG) { "Starting animation loop for ${pendingDuration}" }
        
        val startTime = System.currentTimeMillis()
        var frameCount = 0
        var lastFpsUpdate = startTime
        
        lifecycleScope.launch {
            val targetFrameTime = 16L // ~60 FPS
            var lastFrameTime = System.currentTimeMillis()
            
            while (isActive) {
                val currentTime = System.currentTimeMillis()
                val elapsedMs = currentTime - startTime
                
                // Check if duration exceeded
                if (elapsedMs > pendingDuration.inWholeMilliseconds) {
                    RiveLog.d(TAG) { "Animation loop complete" }
                    break
                }
                
                // Calculate delta time
                val deltaTime = (currentTime - lastFrameTime) / 1000f
                lastFrameTime = currentTime
                
                try {
                    // Advance state machine
                    config.renderTestUtil.commandQueue.advanceStateMachine(
                        config.smHandle,
                        deltaTime
                    )
                    
                    // Render frame
                    config.renderTestUtil.commandQueue.draw(
                        artboardHandle = config.artboardHandle,
                        smHandle = config.smHandle,
                        surface = config.surface,
                        fit = config.fit,
                        alignment = config.alignment
                    )
                    
                    // Read pixels and update display
                    // Note: This needs to happen on GL thread context
                    val bitmap = withContext(Dispatchers.Default) {
                        DebugRenderPreview.readPixels(config.surface)
                    }
                    
                    imageView.setImageBitmap(bitmap)
                    frameCount++
                    
                    // Update FPS counter every second
                    if (currentTime - lastFpsUpdate >= 1000) {
                        val fps = frameCount * 1000f / (currentTime - lastFpsUpdate)
                        fpsView.text = "%.1f FPS".format(fps)
                        frameCount = 0
                        lastFpsUpdate = currentTime
                    }
                    
                } catch (e: Exception) {
                    RiveLog.e(TAG) { "Animation loop error: ${e.message}" }
                    break
                }
                
                // Sleep to maintain frame rate
                val frameTime = System.currentTimeMillis() - currentTime
                val sleepTime = targetFrameTime - frameTime
                if (sleepTime > 0) {
                    delay(sleepTime.milliseconds)
                }
            }
            
            RiveLog.d(TAG) { "Auto-closing animation preview" }
            finish()
        }
    }
    
    override fun onDestroy() {
        super.onDestroy()
        RiveLog.d(TAG) { "Destroying preview activity" }
        
        // Clear static references
        pendingBitmap = null
        animationLoop = null
    }
}
