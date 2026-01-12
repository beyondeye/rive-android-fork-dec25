package app.rive.mp.test.rendering

import app.rive.mp.DrawKey
import app.rive.mp.RiveSurface
import app.rive.mp.core.SpriteDrawCommand
import app.rive.mp.test.utils.MpCommandQueueTestUtil
import app.rive.mp.test.utils.MpTestContext
import app.rive.mp.test.utils.MpTestResources
import app.rive.mp.test.utils.loadRiveFile
import kotlinx.coroutines.test.runTest
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith

/**
 * Tests for batch rendering operations via CommandQueue.
 *
 * These tests verify that drawMultiple() and drawMultipleToBuffer() work correctly
 * for efficient batch rendering of multiple sprites.
 *
 * ## Test Categories
 * - Basic drawMultiple invocation (fire-and-forget)
 * - Multiple sprites in single batch
 * - Different transform configurations
 * - drawMultipleToBuffer for offscreen rendering
 * - Error handling (empty commands, etc.)
 */
class MpRiveBatchRenderingTest {

    init {
        MpTestContext.initPlatform()
    }

    /**
     * Mock RiveSurface for testing batch rendering.
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
    // Basic drawMultiple Tests
    // =========================================================================

    /**
     * Test that drawMultiple can be called with a single sprite without throwing.
     */
    @Test
    fun drawMultiple_singleSprite_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            val surface = MockRiveSurface()

            // Create a single sprite draw command with identity transform
            val command = SpriteDrawCommand(
                artboardHandle = artboardHandle,
                stateMachineHandle = smHandle,
                transform = floatArrayOf(1f, 0f, 0f, 1f, 0f, 0f), // Identity transform
                artboardWidth = 100f,
                artboardHeight = 100f
            )

            // Fire drawMultiple - should not throw
            testUtil.commandQueue.drawMultiple(
                surface = surface,
                commands = listOf(command),
                clearColor = 0xFF000000.toInt()
            )

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test that drawMultiple can handle multiple sprites in one batch.
     */
    @Test
    fun drawMultiple_multipleSprites_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            // Create multiple artboards and state machines
            val artboard1 = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val sm1 = testUtil.commandQueue.createDefaultStateMachine(artboard1)
            
            val artboard2 = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val sm2 = testUtil.commandQueue.createDefaultStateMachine(artboard2)
            
            val artboard3 = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val sm3 = testUtil.commandQueue.createDefaultStateMachine(artboard3)

            val surface = MockRiveSurface()

            // Create multiple sprite draw commands
            val commands = listOf(
                SpriteDrawCommand(
                    artboardHandle = artboard1,
                    stateMachineHandle = sm1,
                    transform = floatArrayOf(1f, 0f, 0f, 1f, 0f, 0f), // Position at (0, 0)
                    artboardWidth = 100f,
                    artboardHeight = 100f
                ),
                SpriteDrawCommand(
                    artboardHandle = artboard2,
                    stateMachineHandle = sm2,
                    transform = floatArrayOf(1f, 0f, 0f, 1f, 200f, 0f), // Position at (200, 0)
                    artboardWidth = 100f,
                    artboardHeight = 100f
                ),
                SpriteDrawCommand(
                    artboardHandle = artboard3,
                    stateMachineHandle = sm3,
                    transform = floatArrayOf(1f, 0f, 0f, 1f, 400f, 0f), // Position at (400, 0)
                    artboardWidth = 100f,
                    artboardHeight = 100f
                )
            )

            // Fire drawMultiple - should not throw
            testUtil.commandQueue.drawMultiple(
                surface = surface,
                commands = commands,
                clearColor = 0xFF000000.toInt()
            )

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(sm3)
            testUtil.commandQueue.deleteStateMachine(sm2)
            testUtil.commandQueue.deleteStateMachine(sm1)
            testUtil.commandQueue.deleteArtboard(artboard3)
            testUtil.commandQueue.deleteArtboard(artboard2)
            testUtil.commandQueue.deleteArtboard(artboard1)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    // =========================================================================
    // Transform Tests
    // =========================================================================

    /**
     * Test drawMultiple with scaled sprites.
     */
    @Test
    fun drawMultiple_withScaleTransform_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            val surface = MockRiveSurface()

            // Create a sprite with 2x scale
            val command = SpriteDrawCommand(
                artboardHandle = artboardHandle,
                stateMachineHandle = smHandle,
                transform = floatArrayOf(2f, 0f, 0f, 2f, 0f, 0f), // 2x scale
                artboardWidth = 100f,
                artboardHeight = 100f
            )

            // Fire drawMultiple - should not throw
            testUtil.commandQueue.drawMultiple(
                surface = surface,
                commands = listOf(command)
            )

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test drawMultiple with rotated sprites (via skew in transform).
     */
    @Test
    fun drawMultiple_withRotationTransform_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            val surface = MockRiveSurface()

            // Create a sprite with 45 degree rotation
            // cos(45) ≈ 0.707, sin(45) ≈ 0.707
            val cos45 = 0.707f
            val sin45 = 0.707f
            val command = SpriteDrawCommand(
                artboardHandle = artboardHandle,
                stateMachineHandle = smHandle,
                transform = floatArrayOf(cos45, sin45, -sin45, cos45, 100f, 100f),
                artboardWidth = 100f,
                artboardHeight = 100f
            )

            // Fire drawMultiple - should not throw
            testUtil.commandQueue.drawMultiple(
                surface = surface,
                commands = listOf(command)
            )

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test drawMultiple with translated sprites at various positions.
     */
    @Test
    fun drawMultiple_withTranslationTransform_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            val surface = MockRiveSurface(800, 600)

            // Create sprites at different positions
            val commands = listOf(
                // Top-left
                SpriteDrawCommand(
                    artboardHandle = artboardHandle,
                    stateMachineHandle = smHandle,
                    transform = floatArrayOf(1f, 0f, 0f, 1f, 0f, 0f),
                    artboardWidth = 100f,
                    artboardHeight = 100f
                ),
                // Center
                SpriteDrawCommand(
                    artboardHandle = artboardHandle,
                    stateMachineHandle = smHandle,
                    transform = floatArrayOf(1f, 0f, 0f, 1f, 350f, 250f),
                    artboardWidth = 100f,
                    artboardHeight = 100f
                ),
                // Bottom-right
                SpriteDrawCommand(
                    artboardHandle = artboardHandle,
                    stateMachineHandle = smHandle,
                    transform = floatArrayOf(1f, 0f, 0f, 1f, 700f, 500f),
                    artboardWidth = 100f,
                    artboardHeight = 100f
                )
            )

            // Fire drawMultiple - should not throw
            testUtil.commandQueue.drawMultiple(
                surface = surface,
                commands = commands
            )

            // Cleanup
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
     * Test drawMultiple with different clear colors.
     */
    @Test
    fun drawMultiple_withDifferentClearColors_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            val surface = MockRiveSurface()

            val command = SpriteDrawCommand(
                artboardHandle = artboardHandle,
                stateMachineHandle = smHandle,
                transform = floatArrayOf(1f, 0f, 0f, 1f, 0f, 0f),
                artboardWidth = 100f,
                artboardHeight = 100f
            )

            // Test with various clear colors
            val clearColors = listOf(
                0x00000000, // Transparent
                0xFF000000.toInt(), // Opaque black
                0xFFFFFFFF.toInt(), // Opaque white
                0xFFFF0000.toInt(), // Opaque red
                0x80FFFFFF.toInt()  // Semi-transparent white
            )

            for (color in clearColors) {
                testUtil.commandQueue.drawMultiple(
                    surface = surface,
                    commands = listOf(command),
                    clearColor = color
                )
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
    // Error Handling Tests
    // =========================================================================

    /**
     * Test that drawMultiple throws on empty commands list.
     */
    @Test
    fun drawMultiple_emptyCommands_throwsException() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val surface = MockRiveSurface()

            assertFailsWith<IllegalArgumentException> {
                testUtil.commandQueue.drawMultiple(
                    surface = surface,
                    commands = emptyList()
                )
            }
        } finally {
            testUtil.cleanup()
        }
    }

    // =========================================================================
    // drawMultipleToBuffer Tests
    // =========================================================================

    /**
     * Test that drawMultipleToBuffer can be called without throwing.
     */
    @Test
    fun drawMultipleToBuffer_singleSprite_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            val width = 100
            val height = 100
            val surface = MockRiveSurface(width, height)
            val buffer = ByteArray(width * height * 4) // RGBA

            val command = SpriteDrawCommand(
                artboardHandle = artboardHandle,
                stateMachineHandle = smHandle,
                transform = floatArrayOf(1f, 0f, 0f, 1f, 0f, 0f),
                artboardWidth = width.toFloat(),
                artboardHeight = height.toFloat()
            )

            // Fire drawMultipleToBuffer - should not throw
            testUtil.commandQueue.drawMultipleToBuffer(
                surface = surface,
                commands = listOf(command),
                clearColor = 0xFF000000.toInt(),
                buffer = buffer
            )

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test that drawMultipleToBuffer throws on buffer too small.
     */
    @Test
    fun drawMultipleToBuffer_bufferTooSmall_throwsException() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            val width = 100
            val height = 100
            val surface = MockRiveSurface(width, height)
            val buffer = ByteArray(100) // Too small - should be width * height * 4

            val command = SpriteDrawCommand(
                artboardHandle = artboardHandle,
                stateMachineHandle = smHandle,
                transform = floatArrayOf(1f, 0f, 0f, 1f, 0f, 0f),
                artboardWidth = width.toFloat(),
                artboardHeight = height.toFloat()
            )

            assertFailsWith<IllegalArgumentException> {
                testUtil.commandQueue.drawMultipleToBuffer(
                    surface = surface,
                    commands = listOf(command),
                    buffer = buffer
                )
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
     * Test that drawMultipleToBuffer throws on empty commands.
     */
    @Test
    fun drawMultipleToBuffer_emptyCommands_throwsException() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val surface = MockRiveSurface(100, 100)
            val buffer = ByteArray(100 * 100 * 4)

            assertFailsWith<IllegalArgumentException> {
                testUtil.commandQueue.drawMultipleToBuffer(
                    surface = surface,
                    commands = emptyList(),
                    buffer = buffer
                )
            }
        } finally {
            testUtil.cleanup()
        }
    }

    // =========================================================================
    // Large Batch Tests
    // =========================================================================

    /**
     * Test drawMultiple with a larger number of sprites (stress test).
     */
    @Test
    fun drawMultiple_manySprites_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            val surface = MockRiveSurface(1920, 1080)

            // Create many sprites in a grid
            val cols = 10
            val rows = 10
            val spriteWidth = 50f
            val spriteHeight = 50f

            val commands = mutableListOf<SpriteDrawCommand>()
            for (row in 0 until rows) {
                for (col in 0 until cols) {
                    commands.add(
                        SpriteDrawCommand(
                            artboardHandle = artboardHandle,
                            stateMachineHandle = smHandle,
                            transform = floatArrayOf(
                                1f, 0f, 0f, 1f,
                                col * spriteWidth * 1.5f,
                                row * spriteHeight * 1.5f
                            ),
                            artboardWidth = spriteWidth,
                            artboardHeight = spriteHeight
                        )
                    )
                }
            }

            assertEquals(100, commands.size, "Should have 100 sprites")

            // Fire drawMultiple - should not throw
            testUtil.commandQueue.drawMultiple(
                surface = surface,
                commands = commands,
                clearColor = 0xFF000000.toInt()
            )

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
}