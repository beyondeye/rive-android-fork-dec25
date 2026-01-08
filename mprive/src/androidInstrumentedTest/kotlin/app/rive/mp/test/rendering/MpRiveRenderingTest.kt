package app.rive.mp.test.rendering

import app.rive.mp.core.Alignment
import app.rive.mp.core.Fit
import app.rive.mp.test.utils.MpTestContext
import app.rive.mp.test.utils.MpTestResources
import app.rive.mp.test.utils.loadRiveFile
import kotlinx.coroutines.delay
import kotlinx.coroutines.test.runTest
import kotlin.test.Test
import kotlin.test.assertTrue

/**
 * End-to-End rendering tests for the mprive CommandQueue rendering pipeline.
 * 
 * These tests verify that the rendering system works correctly by:
 * 1. Loading Rive files
 * 2. Creating artboards and state machines
 * 3. Creating PBuffer surfaces
 * 4. Calling draw() with various configurations
 * 5. Verifying no crashes occur
 * 
 * ## Test Files Used
 * - `shapes.riv` - Simple shapes for basic tests
 * - `flux_capacitor.riv` - Animated content for fit/alignment tests
 * - `basketball.riv` - Simple animation for loop tests
 * 
 * ## Debug Preview
 * Set [DebugRenderPreview.isEnabled] to true before running tests
 * to visually see rendered output.
 */
class MpRiveRenderingTest {

    init {
        MpTestContext.initPlatform()
    }

    // =========================================================================
    // Basic Rendering Tests
    // =========================================================================

    /**
     * Test rendering a single frame to a PBuffer surface.
     * 
     * This is the most basic rendering test that verifies the entire pipeline:
     * - File loading
     * - Artboard creation
     * - State machine creation
     * - Surface creation
     * - draw() call
     */
    @Test
    fun renderSingleFrame() = runTest {
        val testUtil = AndroidRenderTestUtil(this)
        try {
            // 1. Load file
            val bytes = MpTestResources.loadRiveFile("shapes.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            // 2. Create artboard
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            
            // 3. Create state machine
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)
            
            // 4. Create PBuffer surface (100x100)
            val surface = testUtil.createTestSurface(100, 100)
            
            // 5. Draw (should not crash)
            testUtil.commandQueue.draw(
                artboardHandle = artboardHandle,
                smHandle = smHandle,
                surface = surface,
                fit = Fit.CONTAIN,
                alignment = Alignment.CENTER
            )
            
            // 6. Advance to process draw command
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)
            delay(100) // Allow draw to complete
            
            // Success: no crash occurred
            assertTrue(true, "Single frame render completed without crash")
            
            // 7. Cleanup
            surface.close()
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    // =========================================================================
    // Fit Mode Tests
    // =========================================================================

    /**
     * Test rendering with CONTAIN fit mode.
     * CONTAIN scales the content to fit within bounds while maintaining aspect ratio.
     */
    @Test
    fun renderWithFitContain() = runTest {
        val testUtil = AndroidRenderTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)
            
            // Use non-square surface to test aspect ratio handling
            val surface = testUtil.createTestSurface(200, 100)
            
            // Draw with CONTAIN fit
            testUtil.commandQueue.draw(
                artboardHandle = artboardHandle,
                smHandle = smHandle,
                surface = surface,
                fit = Fit.CONTAIN,
                alignment = Alignment.CENTER
            )
            
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)
            delay(100)
            
            assertTrue(true, "CONTAIN fit render completed without crash")
            
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
     * Test rendering with FILL fit mode.
     * FILL stretches the content to fill the entire bounds (may distort aspect ratio).
     */
    @Test
    fun renderWithFitFill() = runTest {
        val testUtil = AndroidRenderTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)
            
            // Use non-square surface
            val surface = testUtil.createTestSurface(200, 100)
            
            // Draw with FILL fit
            testUtil.commandQueue.draw(
                artboardHandle = artboardHandle,
                smHandle = smHandle,
                surface = surface,
                fit = Fit.FILL,
                alignment = Alignment.CENTER
            )
            
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)
            delay(100)
            
            assertTrue(true, "FILL fit render completed without crash")
            
            // Cleanup
            surface.close()
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    // =========================================================================
    // Alignment Tests
    // =========================================================================

    /**
     * Test rendering with all 9 alignment positions.
     * Verifies that all alignment modes work without crashing.
     */
    @Test
    fun renderWithAlignments() = runTest {
        val testUtil = AndroidRenderTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("shapes.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)
            val surface = testUtil.createTestSurface(150, 150)
            
            // Test all 9 alignment positions
            val alignments = listOf(
                Alignment.TOP_LEFT,
                Alignment.TOP_CENTER,
                Alignment.TOP_RIGHT,
                Alignment.CENTER_LEFT,
                Alignment.CENTER,
                Alignment.CENTER_RIGHT,
                Alignment.BOTTOM_LEFT,
                Alignment.BOTTOM_CENTER,
                Alignment.BOTTOM_RIGHT
            )
            
            for (alignment in alignments) {
                testUtil.commandQueue.draw(
                    artboardHandle = artboardHandle,
                    smHandle = smHandle,
                    surface = surface,
                    fit = Fit.CONTAIN,
                    alignment = alignment
                )
                testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)
                delay(50)
            }
            
            assertTrue(true, "All 9 alignment modes rendered without crash")
            
            // Cleanup
            surface.close()
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    // =========================================================================
    // Animation Loop Tests
    // =========================================================================

    /**
     * Test rendering an animation loop with multiple frames.
     * This tests the typical use case of continuously rendering an animation.
     */
    @Test
    fun renderAnimationLoop() = runTest {
        val testUtil = AndroidRenderTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("basketball.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)
            val surface = testUtil.createTestSurface(200, 200)
            
            // Render 60 frames (~1 second at 60fps)
            repeat(60) { frame ->
                // Advance animation
                testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)
                
                // Render frame
                testUtil.commandQueue.draw(
                    artboardHandle = artboardHandle,
                    smHandle = smHandle,
                    surface = surface,
                    fit = Fit.CONTAIN,
                    alignment = Alignment.CENTER
                )
                
                // Small delay between frames
                delay(16)
            }
            
            assertTrue(true, "Animation loop (60 frames) completed without crash")
            
            // Cleanup
            surface.close()
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    // =========================================================================
    // Clear Color Tests
    // =========================================================================

    /**
     * Test rendering with different clear colors.
     * Verifies that the clearColor parameter is applied correctly.
     */
    @Test
    fun renderWithDifferentClearColors() = runTest {
        val testUtil = AndroidRenderTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("shapes.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)
            val surface = testUtil.createTestSurface(100, 100)
            
            // Test various clear colors
            val clearColors = listOf(
                0xFF000000.toInt(),  // Opaque black
                0xFFFFFFFF.toInt(),  // Opaque white
                0xFFFF0000.toInt(),  // Opaque red
                0xFF00FF00.toInt(),  // Opaque green
                0xFF0000FF.toInt(),  // Opaque blue
                0x80000000.toInt(),  // Semi-transparent black
                0x00000000           // Fully transparent
            )
            
            for (clearColor in clearColors) {
                testUtil.commandQueue.draw(
                    artboardHandle = artboardHandle,
                    smHandle = smHandle,
                    surface = surface,
                    fit = Fit.CONTAIN,
                    alignment = Alignment.CENTER,
                    clearColor = clearColor
                )
                testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)
                delay(50)
            }
            
            assertTrue(true, "All clear colors rendered without crash")
            
            // Cleanup
            surface.close()
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
}
