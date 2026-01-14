package app.rive.mp.test.rendering

import app.rive.mp.ArtboardHandle
import app.rive.mp.StateMachineHandle
import app.rive.mp.core.Alignment
import app.rive.mp.core.Fit
import app.rive.mp.test.utils.MpTestContext
import kotlinx.coroutines.runBlocking
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue

/**
 * Tests for the drawToBuffer API (Phase E.4).
 *
 * These tests verify the single artboard offscreen rendering capability.
 */
class MpRiveDrawToBufferTest {

    @Test
    fun drawToBuffer_fillsBufferWithPixelData() = runBlocking {
        MpTestContext.withCommandQueue { cq ->
            // Load a test file and create artboard
            val fileBytes = MpTestContext.loadTestResource("multipleartboards.riv")
            val fileHandle = cq.loadFile(fileBytes)
            val artboardHandle = cq.createDefaultArtboard(fileHandle)
            val smHandle = StateMachineHandle(0)  // No state machine for static render

            // Create a small test surface
            val drawKey = cq.createDrawKey()
            val surface = cq.createImageSurface(64, 64, drawKey)

            // Create buffer for pixel data
            val buffer = ByteArray(64 * 64 * 4)  // RGBA

            // Draw to buffer
            cq.drawToBuffer(
                artboardHandle = artboardHandle,
                smHandle = smHandle,
                surface = surface,
                buffer = buffer
            )

            // Verify buffer was written to (placeholder fills with magenta 0xFFFF00FF)
            // In RGBA order: R=0xFF, G=0x00, B=0xFF, A=0xFF
            assertTrue(buffer.isNotEmpty())
            
            // Check first pixel is magenta (placeholder color)
            assertEquals(0xFF.toByte(), buffer[0], "Red channel should be 0xFF")
            assertEquals(0x00.toByte(), buffer[1], "Green channel should be 0x00")
            assertEquals(0xFF.toByte(), buffer[2], "Blue channel should be 0xFF")
            assertEquals(0xFF.toByte(), buffer[3], "Alpha channel should be 0xFF")

            // Cleanup
            cq.destroyRiveSurface(surface)
            cq.deleteArtboard(artboardHandle)
            cq.deleteFile(fileHandle)
        }
    }

    @Test
    fun drawToBuffer_withFitAndAlignment() = runBlocking {
        MpTestContext.withCommandQueue { cq ->
            val fileBytes = MpTestContext.loadTestResource("multipleartboards.riv")
            val fileHandle = cq.loadFile(fileBytes)
            val artboardHandle = cq.createDefaultArtboard(fileHandle)

            val drawKey = cq.createDrawKey()
            val surface = cq.createImageSurface(100, 100, drawKey)
            val buffer = ByteArray(100 * 100 * 4)

            // Test with different fit/alignment options
            cq.drawToBuffer(
                artboardHandle = artboardHandle,
                smHandle = StateMachineHandle(0),
                surface = surface,
                buffer = buffer,
                fit = Fit.COVER,
                alignment = Alignment.TOP_LEFT,
                scaleFactor = 2.0f,
                clearColor = 0xFF000000.toInt()
            )

            // Verify buffer was written
            assertTrue(buffer.any { it != 0.toByte() }, "Buffer should contain non-zero data")

            // Cleanup
            cq.destroyRiveSurface(surface)
            cq.deleteArtboard(artboardHandle)
            cq.deleteFile(fileHandle)
        }
    }

    @Test
    fun drawToBuffer_bufferSizeValidation() = runBlocking {
        MpTestContext.withCommandQueue { cq ->
            val fileBytes = MpTestContext.loadTestResource("multipleartboards.riv")
            val fileHandle = cq.loadFile(fileBytes)
            val artboardHandle = cq.createDefaultArtboard(fileHandle)

            val drawKey = cq.createDrawKey()
            val surface = cq.createImageSurface(50, 50, drawKey)

            // Try with undersized buffer - should throw
            val smallBuffer = ByteArray(50 * 50 * 2)  // Too small (need 4 bytes per pixel)

            var exceptionThrown = false
            try {
                cq.drawToBuffer(
                    artboardHandle = artboardHandle,
                    smHandle = StateMachineHandle(0),
                    surface = surface,
                    buffer = smallBuffer
                )
            } catch (e: IllegalArgumentException) {
                exceptionThrown = true
                assertTrue(e.message?.contains("too small") == true)
            }

            assertTrue(exceptionThrown, "Should throw IllegalArgumentException for undersized buffer")

            // Cleanup
            cq.destroyRiveSurface(surface)
            cq.deleteArtboard(artboardHandle)
            cq.deleteFile(fileHandle)
        }
    }

    @Test
    fun drawToBuffer_withStateMachine() = runBlocking {
        MpTestContext.withCommandQueue { cq ->
            val fileBytes = MpTestContext.loadTestResource("what_a_state.riv")
            val fileHandle = cq.loadFile(fileBytes)
            val artboardHandle = cq.createDefaultArtboard(fileHandle)
            val smHandle = cq.createDefaultStateMachine(artboardHandle)

            // Advance the state machine
            cq.advanceStateMachine(smHandle, 0.016f)

            val drawKey = cq.createDrawKey()
            val surface = cq.createImageSurface(128, 128, drawKey)
            val buffer = ByteArray(128 * 128 * 4)

            // Draw with state machine
            cq.drawToBuffer(
                artboardHandle = artboardHandle,
                smHandle = smHandle,
                surface = surface,
                buffer = buffer
            )

            // Verify buffer was written
            assertTrue(buffer.any { it != 0.toByte() }, "Buffer should contain rendered pixels")

            // Cleanup
            cq.destroyRiveSurface(surface)
            cq.deleteStateMachine(smHandle)
            cq.deleteArtboard(artboardHandle)
            cq.deleteFile(fileHandle)
        }
    }
}