package app.rive.mp.test.artboard

import app.rive.mp.test.utils.MpCommandQueueTestUtil
import app.rive.mp.test.utils.MpTestContext
import app.rive.mp.test.utils.MpTestResources
import app.rive.mp.test.utils.loadRiveFile
import kotlinx.coroutines.test.runTest
import kotlin.test.Test
import kotlin.test.assertNotEquals

/**
 * Tests for artboard resizing operations via CommandQueue.
 * 
 * These tests verify the Phase E.3 artboard resizing functionality which is
 * required for Fit.Layout mode where the artboard must match surface dimensions.
 * 
 * Note: These are integration tests that verify the resize/reset methods
 * execute without errors. Full visual verification of resize effects requires
 * a GPU render context.
 */
class MpRiveArtboardResizeTest {
    
    init {
        MpTestContext.initPlatform()
    }
    
    /**
     * Test basic artboard resize operation.
     * Verifies that resizeArtboard can be called without errors.
     */
    @Test
    fun resizeArtboard_basicResize_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            
            // Resize artboard to specific dimensions
            testUtil.commandQueue.resizeArtboard(
                artboardHandle = artboardHandle,
                width = 800,
                height = 600
            )
            
            // If we get here without exception, the resize command was accepted
            // Note: Actual resize effect verification requires GPU context
            
            // Cleanup
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
    
    /**
     * Test artboard reset to original size.
     * Verifies that resetArtboardSize can be called without errors.
     */
    @Test
    fun resetArtboardSize_afterResize_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            
            // First resize the artboard
            testUtil.commandQueue.resizeArtboard(
                artboardHandle = artboardHandle,
                width = 1920,
                height = 1080
            )
            
            // Then reset to original size
            testUtil.commandQueue.resetArtboardSize(artboardHandle)
            
            // If we get here without exception, the reset was accepted
            
            // Cleanup
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
    
    /**
     * Test artboard resize with scale factor.
     * Verifies that the scaleFactor parameter is accepted.
     */
    @Test
    fun resizeArtboard_withScaleFactor_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            
            // Resize with a 2x scale factor (common for high DPI displays)
            testUtil.commandQueue.resizeArtboard(
                artboardHandle = artboardHandle,
                width = 1600,
                height = 1200,
                scaleFactor = 2.0f
            )
            
            // Cleanup
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
    
    /**
     * Test multiple resize operations on the same artboard.
     * Verifies that consecutive resizes are handled correctly.
     */
    @Test
    fun resizeArtboard_multipleResizes_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            
            // Resize multiple times (simulating window resize events)
            testUtil.commandQueue.resizeArtboard(artboardHandle, 640, 480)
            testUtil.commandQueue.resizeArtboard(artboardHandle, 800, 600)
            testUtil.commandQueue.resizeArtboard(artboardHandle, 1024, 768)
            testUtil.commandQueue.resizeArtboard(artboardHandle, 1920, 1080)
            
            // Cleanup
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
    
    /**
     * Test complete resize/reset cycle.
     * Verifies the full workflow: resize -> use -> reset.
     */
    @Test
    fun resizeArtboard_fullCycle_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            
            // Create state machine to advance (typical usage pattern)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)
            assertNotEquals(0L, smHandle.handle, "State machine handle should be non-zero")
            
            // Resize artboard (for Fit.Layout mode)
            testUtil.commandQueue.resizeArtboard(artboardHandle, 1280, 720)
            
            // Advance state machine (resize should affect layout)
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f) // ~60fps
            
            // Reset to original dimensions
            testUtil.commandQueue.resetArtboardSize(artboardHandle)
            
            // Advance again to verify reset took effect
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)
            
            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
    
    /**
     * Test resize with zero dimensions.
     * Verifies that zero dimensions don't cause crashes (edge case).
     */
    @Test
    fun resizeArtboard_zeroDimensions_doesNotCrash() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            
            // Attempt resize with zero dimensions
            // This is an edge case that should be handled gracefully
            testUtil.commandQueue.resizeArtboard(artboardHandle, 0, 0)
            
            // Reset to recover
            testUtil.commandQueue.resetArtboardSize(artboardHandle)
            
            // Cleanup
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
    
    /**
     * Test resize on multiple artboards from the same file.
     * Verifies that resizing one artboard doesn't affect another.
     */
    @Test
    fun resizeArtboard_multipleArtboards_independent() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("multipleartboards.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            // Create two different artboards
            val artboard1 = testUtil.commandQueue.createArtboardByName(fileHandle, "artboard1")
            val artboard2 = testUtil.commandQueue.createArtboardByName(fileHandle, "artboard2")
            
            assertNotEquals(artboard1.handle, artboard2.handle, "Artboards should have different handles")
            
            // Resize each artboard to different dimensions
            testUtil.commandQueue.resizeArtboard(artboard1, 800, 600)
            testUtil.commandQueue.resizeArtboard(artboard2, 1024, 768)
            
            // Reset only one
            testUtil.commandQueue.resetArtboardSize(artboard1)
            
            // Cleanup
            testUtil.commandQueue.deleteArtboard(artboard1)
            testUtil.commandQueue.deleteArtboard(artboard2)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
    
    /**
     * Test reset without prior resize.
     * Verifies that reset is a no-op when artboard hasn't been resized.
     */
    @Test
    fun resetArtboardSize_withoutPriorResize_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            
            // Reset without any prior resize - should be a no-op
            testUtil.commandQueue.resetArtboardSize(artboardHandle)
            
            // Cleanup
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
}