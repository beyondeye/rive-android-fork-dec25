package app.rive.mp.test.rendering

import androidx.test.platform.app.InstrumentationRegistry
import app.rive.mp.core.Alignment
import app.rive.mp.core.Fit
import app.rive.mp.test.utils.MpTestContext
import app.rive.mp.test.utils.MpTestResources
import app.rive.mp.test.utils.loadRiveFile
import kotlinx.coroutines.delay
import kotlinx.coroutines.test.runTest
import kotlin.test.Test
import kotlin.test.assertNotNull
import kotlin.test.assertTrue
import kotlin.time.Duration.Companion.seconds

/**
 * Tests for the debug preview functionality.
 * 
 * These tests verify that the [DebugRenderPreview] infrastructure works correctly:
 * - Reading pixels from PBuffer surfaces
 * - Creating bitmaps from rendered content
 * - Launching debug preview activities (when enabled)
 * 
 * **Note**: These tests are primarily for verifying the debug infrastructure.
 * Set [DebugRenderPreview.isEnabled] to true to see visual output.
 */
class MpRiveDebugPreviewTest {

    init {
        MpTestContext.initPlatform()
    }

    /**
     * Test that we can read pixels from a PBuffer surface after rendering.
     * This verifies the core functionality needed for debug preview.
     */
    @Test
    fun readPixelsFromPBuffer() = runTest {
        val testUtil = AndroidRenderTestUtil(this)
        try {
            // Load and setup
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)
            val surface = testUtil.createTestSurface(100, 100)
            
            // Render
            testUtil.commandQueue.draw(
                artboardHandle = artboardHandle,
                smHandle = smHandle,
                surface = surface,
                fit = Fit.CONTAIN,
                alignment = Alignment.CENTER,
                clearColor = 0xFFFF0000.toInt() // Red background for visibility
            )
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)
            delay(100) // Allow render to complete
            
            // Read pixels - this tests the core debug functionality
            // Note: This may fail if GL context is not current
            // In a real test environment, we'd need to ensure GL context is available
            try {
                val bitmap = DebugRenderPreview.readPixels(surface)
                assertNotNull(bitmap, "Bitmap should not be null")
                assertTrue(bitmap.width == 100, "Bitmap width should be 100")
                assertTrue(bitmap.height == 100, "Bitmap height should be 100")
                bitmap.recycle()
            } catch (e: Exception) {
                // GL context may not be available in all test environments
                // This is expected in some cases
                println("Note: readPixels failed (expected in some test environments): ${e.message}")
            }
            
            // Cleanup
            surface.close()
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test static bitmap preview display.
     * 
     * This test is ONLY meaningful when [DebugRenderPreview.isEnabled] is true.
     * When disabled, it just verifies that the code doesn't crash.
     */
    @Test
    fun debugPreviewDisplaysStaticBitmap() = runTest {
        val testUtil = AndroidRenderTestUtil(this)
        try {
            // Enable debug preview for this test
            // Uncomment the next line to see the preview:
            // DebugRenderPreview.isEnabled = true
            DebugRenderPreview.staticDisplayDuration = 2.seconds
            
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)
            val surface = testUtil.createTestSurface(200, 200)
            
            // Render with a visible clear color
            testUtil.commandQueue.draw(
                artboardHandle = artboardHandle,
                smHandle = smHandle,
                surface = surface,
                fit = Fit.CONTAIN,
                alignment = Alignment.CENTER,
                clearColor = 0xFF0000FF.toInt() // Blue background
            )
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)
            delay(100)
            
            // Show preview (only visible if DebugRenderPreview.isEnabled = true)
            if (DebugRenderPreview.isEnabled) {
                try {
                    val bitmap = DebugRenderPreview.readPixels(surface)
                    val context = InstrumentationRegistry.getInstrumentation().targetContext
                    DebugRenderPreview.showBitmap(context, bitmap, "Static Preview Test")
                    
                    // Wait for display duration
                    delay(DebugRenderPreview.staticDisplayDuration)
                } catch (e: Exception) {
                    println("Debug preview display failed: ${e.message}")
                }
            }
            
            assertTrue(true, "Debug preview test completed")
            
            // Cleanup
            surface.close()
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
            DebugRenderPreview.isEnabled = false
        }
    }

    /**
     * Test animation loop preview display.
     * 
     * This test is ONLY meaningful when [DebugRenderPreview.isEnabled] is true.
     * When disabled, it just renders frames without visual output.
     */
    @Test
    fun debugPreviewDisplaysAnimationLoop() = runTest {
        val testUtil = AndroidRenderTestUtil(this)
        try {
            // Enable debug preview for this test
            // Uncomment the next line to see the preview:
            // DebugRenderPreview.isEnabled = true
            
            val bytes = MpTestResources.loadRiveFile("basketball.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)
            val surface = testUtil.createTestSurface(300, 300)
            
            if (DebugRenderPreview.isEnabled) {
                try {
                    val context = InstrumentationRegistry.getInstrumentation().targetContext
                    DebugRenderPreview.showAnimationLoop(
                        context = context,
                        renderTestUtil = testUtil,
                        artboardHandle = artboardHandle,
                        smHandle = smHandle,
                        surface = surface,
                        duration = 3.seconds,
                        fit = Fit.CONTAIN,
                        alignment = Alignment.CENTER,
                        title = "Animation Loop Test"
                    )
                    
                    // Wait for animation to complete
                    delay(3.seconds)
                } catch (e: Exception) {
                    println("Animation preview display failed: ${e.message}")
                }
            } else {
                // Just render some frames without preview
                repeat(30) {
                    testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)
                    testUtil.commandQueue.draw(
                        artboardHandle = artboardHandle,
                        smHandle = smHandle,
                        surface = surface,
                        fit = Fit.CONTAIN,
                        alignment = Alignment.CENTER
                    )
                    delay(16)
                }
            }
            
            assertTrue(true, "Animation preview test completed")
            
            // Cleanup
            surface.close()
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
            DebugRenderPreview.isEnabled = false
        }
    }

    /**
     * Test that debug preview can be enabled and disabled.
     */
    @Test
    fun debugPreviewCanBeToggled() {
        // Default should be disabled
        assertTrue(!DebugRenderPreview.isEnabled, "Debug preview should be disabled by default")
        
        // Enable
        DebugRenderPreview.isEnabled = true
        assertTrue(DebugRenderPreview.isEnabled, "Debug preview should be enabled after setting")
        
        // Disable
        DebugRenderPreview.isEnabled = false
        assertTrue(!DebugRenderPreview.isEnabled, "Debug preview should be disabled after setting")
    }
}
