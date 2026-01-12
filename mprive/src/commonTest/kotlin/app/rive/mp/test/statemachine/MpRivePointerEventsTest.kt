package app.rive.mp.test.statemachine

import app.rive.mp.DrawKey
import app.rive.mp.RiveSurface
import app.rive.mp.core.Alignment
import app.rive.mp.core.Fit
import app.rive.mp.core.UniquePointer
import app.rive.mp.test.utils.MpCommandQueueTestUtil
import app.rive.mp.test.utils.MpTestContext
import app.rive.mp.test.utils.MpTestResources
import app.rive.mp.test.utils.loadRiveFile
import kotlinx.coroutines.test.runTest
import kotlin.test.Test

/**
 * Tests for pointer event operations via CommandQueue.
 *
 * These tests verify that pointer events (move, down, up, exit) can be sent to
 * state machines for user interaction handling.
 *
 * ## Test Categories
 * - Basic pointer event invocation (fire-and-forget)
 * - Pointer events with different coordinates
 * - Pointer events with different fit/alignment modes
 * - Multi-touch support via pointerID
 */
class MpRivePointerEventsTest {

    init {
        MpTestContext.initPlatform()
    }

    /**
     * Mock RiveSurface for testing pointer events.
     * 
     * This provides a minimal surface implementation that can be used in tests
     * without requiring actual GPU resources.
     */
    private class MockRiveSurface(
        width: Int = 800,
        height: Int = 600
    ) : RiveSurface(
        renderTargetPointer = 1L, // Placeholder, won't be dereferenced in tests
        drawKey = DrawKey(1L),
        width = width,
        height = height
    ) {
        override val surfaceNativePointer: Long = 0L

        // Override dispose to avoid calling native code
        override fun dispose(renderTargetPointer: Long) {
            // No-op for mock
        }

        // Override close to avoid issues with UniquePointer
        override fun close() {
            // No-op for mock
        }

        override val closed: Boolean = false
    }

    // =========================================================================
    // Basic Pointer Event Tests
    // =========================================================================

    /**
     * Test that pointerMove can be called without throwing.
     */
    @Test
    fun pointerMove_firesWithoutThrowing() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            val surface = MockRiveSurface()

            // Fire pointer move event - should not throw
            testUtil.commandQueue.pointerMove(
                smHandle = smHandle,
                surface = surface,
                x = 100f,
                y = 100f
            )

            // Advance the state machine to process the event
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
     * Test that pointerDown can be called without throwing.
     */
    @Test
    fun pointerDown_firesWithoutThrowing() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            val surface = MockRiveSurface()

            // Fire pointer down event - should not throw
            testUtil.commandQueue.pointerDown(
                smHandle = smHandle,
                surface = surface,
                x = 100f,
                y = 100f
            )

            // Advance the state machine to process the event
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
     * Test that pointerUp can be called without throwing.
     */
    @Test
    fun pointerUp_firesWithoutThrowing() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            val surface = MockRiveSurface()

            // Fire pointer up event - should not throw
            testUtil.commandQueue.pointerUp(
                smHandle = smHandle,
                surface = surface,
                x = 100f,
                y = 100f
            )

            // Advance the state machine to process the event
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
     * Test that pointerExit can be called without throwing.
     */
    @Test
    fun pointerExit_firesWithoutThrowing() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            val surface = MockRiveSurface()

            // Fire pointer exit event - should not throw
            testUtil.commandQueue.pointerExit(
                smHandle = smHandle,
                surface = surface
            )

            // Advance the state machine to process the event
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    // =========================================================================
    // Click/Touch Sequence Tests
    // =========================================================================

    /**
     * Test a complete click sequence (down -> up).
     */
    @Test
    fun pointerClick_completeSequence() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            val surface = MockRiveSurface()

            // Click at center of surface
            val x = surface.width / 2f
            val y = surface.height / 2f

            // Pointer down
            testUtil.commandQueue.pointerDown(smHandle, surface, x, y)
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)

            // Pointer up (completing the click)
            testUtil.commandQueue.pointerUp(smHandle, surface, x, y)
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
     * Test a drag sequence (down -> move -> move -> up).
     */
    @Test
    fun pointerDrag_completeSequence() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            val surface = MockRiveSurface()

            // Start drag at top-left
            val startX = 100f
            val startY = 100f
            val endX = 300f
            val endY = 300f

            // Pointer down (start drag)
            testUtil.commandQueue.pointerDown(smHandle, surface, startX, startY)
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)

            // Drag moves
            val steps = 5
            for (i in 1..steps) {
                val progress = i.toFloat() / steps
                val x = startX + (endX - startX) * progress
                val y = startY + (endY - startY) * progress
                testUtil.commandQueue.pointerMove(smHandle, surface, x, y)
                testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)
            }

            // Pointer up (end drag)
            testUtil.commandQueue.pointerUp(smHandle, surface, endX, endY)
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
     * Test hover sequence (move -> move -> exit).
     */
    @Test
    fun pointerHover_completeSequence() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            val surface = MockRiveSurface()

            // Hover enters
            testUtil.commandQueue.pointerMove(smHandle, surface, 100f, 100f)
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)

            // Hover moves around
            testUtil.commandQueue.pointerMove(smHandle, surface, 150f, 120f)
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)

            testUtil.commandQueue.pointerMove(smHandle, surface, 200f, 150f)
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)

            // Hover exits
            testUtil.commandQueue.pointerExit(smHandle, surface)
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    // =========================================================================
    // Fit and Alignment Tests
    // =========================================================================

    /**
     * Test pointer events with different fit modes.
     */
    @Test
    fun pointerMove_withDifferentFitModes() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            val surface = MockRiveSurface()

            // Test all fit modes
            val fitModes = listOf(
                Fit.FILL,
                Fit.CONTAIN,
                Fit.COVER,
                Fit.FIT_WIDTH,
                Fit.FIT_HEIGHT,
                Fit.NONE,
                Fit.SCALE_DOWN
            )

            for (fit in fitModes) {
                testUtil.commandQueue.pointerMove(
                    smHandle = smHandle,
                    surface = surface,
                    x = 100f,
                    y = 100f,
                    fit = fit
                )
                testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)
            }

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test pointer events with different alignment modes.
     */
    @Test
    fun pointerMove_withDifferentAlignments() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            val surface = MockRiveSurface()

            // Test all alignment modes
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
                testUtil.commandQueue.pointerMove(
                    smHandle = smHandle,
                    surface = surface,
                    x = 100f,
                    y = 100f,
                    alignment = alignment
                )
                testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)
            }

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    // =========================================================================
    // Multi-Touch Tests
    // =========================================================================

    /**
     * Test pointer events with multiple pointer IDs (multi-touch).
     */
    @Test
    fun pointerEvents_multiTouch() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            val surface = MockRiveSurface()

            // First finger down
            testUtil.commandQueue.pointerDown(
                smHandle = smHandle,
                surface = surface,
                x = 100f,
                y = 100f,
                pointerID = 0
            )
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)

            // Second finger down
            testUtil.commandQueue.pointerDown(
                smHandle = smHandle,
                surface = surface,
                x = 200f,
                y = 200f,
                pointerID = 1
            )
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)

            // Both fingers move (pinch gesture)
            testUtil.commandQueue.pointerMove(smHandle, surface, 120f, 120f, pointerID = 0)
            testUtil.commandQueue.pointerMove(smHandle, surface, 180f, 180f, pointerID = 1)
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)

            // First finger up
            testUtil.commandQueue.pointerUp(smHandle, surface, 120f, 120f, pointerID = 0)
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)

            // Second finger up
            testUtil.commandQueue.pointerUp(smHandle, surface, 180f, 180f, pointerID = 1)
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    // =========================================================================
    // Scale Factor Tests
    // =========================================================================

    /**
     * Test pointer events with different scale factors (HiDPI support).
     */
    @Test
    fun pointerMove_withScaleFactor() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            val surface = MockRiveSurface()

            // Test different scale factors
            val scaleFactors = listOf(1.0f, 1.5f, 2.0f, 3.0f)

            for (scale in scaleFactors) {
                testUtil.commandQueue.pointerMove(
                    smHandle = smHandle,
                    surface = surface,
                    x = 100f,
                    y = 100f,
                    scaleFactor = scale
                )
                testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)
            }

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    // =========================================================================
    // Edge Cases
    // =========================================================================

    /**
     * Test pointer events at surface boundaries.
     */
    @Test
    fun pointerMove_atSurfaceBoundaries() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            val surface = MockRiveSurface(800, 600)

            // Test corner coordinates
            val boundaryCoords = listOf(
                0f to 0f,                           // Top-left
                surface.width.toFloat() to 0f,      // Top-right
                0f to surface.height.toFloat(),     // Bottom-left
                surface.width.toFloat() to surface.height.toFloat() // Bottom-right
            )

            for ((x, y) in boundaryCoords) {
                testUtil.commandQueue.pointerMove(smHandle, surface, x, y)
                testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)
            }

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test pointer events with negative coordinates (outside surface).
     */
    @Test
    fun pointerMove_withNegativeCoordinates() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            val surface = MockRiveSurface()

            // Negative coordinates (pointer outside surface) - should not throw
            testUtil.commandQueue.pointerMove(smHandle, surface, -10f, -10f)
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
     * Test pointer events with coordinates beyond surface bounds.
     */
    @Test
    fun pointerMove_beyondSurfaceBounds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            val surface = MockRiveSurface(800, 600)

            // Coordinates beyond surface bounds - should not throw
            testUtil.commandQueue.pointerMove(smHandle, surface, 1000f, 800f)
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
     * Test rapid pointer events (stress test).
     */
    @Test
    fun pointerMove_rapidFire() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            val surface = MockRiveSurface()

            // Fire many pointer events rapidly
            repeat(100) { i ->
                val x = (i % surface.width).toFloat()
                val y = (i % surface.height).toFloat()
                testUtil.commandQueue.pointerMove(smHandle, surface, x, y)
            }

            // Single advance to process all queued events
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
}
