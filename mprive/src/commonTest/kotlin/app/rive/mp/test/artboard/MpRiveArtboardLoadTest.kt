package app.rive.mp.test.artboard

import app.rive.mp.test.utils.MpCommandQueueTestUtil
import app.rive.mp.test.utils.MpTestContext
import app.rive.mp.test.utils.MpTestResources
import app.rive.mp.test.utils.loadRiveFile
import kotlinx.coroutines.test.runTest
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertNotEquals
import kotlin.test.assertTrue

/**
 * Tests for artboard loading and querying operations via CommandQueue.
 * Ported from kotlin/src/androidTest/kotlin/app/rive/runtime/kotlin/core/RiveArtboardLoadTest.kt
 */
class MpRiveArtboardLoadTest {
    
    init {
        MpTestContext.initPlatform()
    }
    
    /**
     * Test querying artboard count from a file with multiple artboards.
     */
    @Test
    fun queryArtboardCount() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("multipleartboards.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            val artboardNames = testUtil.commandQueue.getArtboardNames(fileHandle)
            assertEquals(2, artboardNames.size, "Expected 2 artboards in multipleartboards.riv")
            
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
    
    /**
     * Test querying artboard names from a file.
     */
    @Test
    fun queryArtboardNames() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("multipleartboards.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            val artboardNames = testUtil.commandQueue.getArtboardNames(fileHandle)
            
            // Check specific artboard names
            assertTrue(artboardNames.contains("artboard1"), "Expected artboard1")
            assertTrue(artboardNames.contains("artboard2"), "Expected artboard2")
            
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
    
    /**
     * Test creating default artboard from a file.
     */
    @Test
    fun createDefaultArtboard() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            
            // Artboard handle should be valid (non-zero)
            assertNotEquals(0L, artboardHandle.handle, "Artboard handle should be non-zero")
            
            // Cleanup
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
    
    /**
     * Test creating artboard by name.
     */
    @Test
    fun createArtboardByName() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("multipleartboards.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            val artboardHandle = testUtil.commandQueue.createArtboardByName(fileHandle, "artboard2")
            
            // Artboard handle should be valid (non-zero)
            assertNotEquals(0L, artboardHandle.handle, "Artboard handle should be non-zero")
            
            // Cleanup
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
    
    /**
     * Test that creating artboard with invalid name fails.
     */
    @Test
    fun createArtboardByInvalidName() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("multipleartboards.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            // Try to create artboard with non-existent name
            val exception = assertFailsWith<IllegalArgumentException> {
                testUtil.commandQueue.createArtboardByName(fileHandle, "nonexistent")
            }
            
            assertTrue(
                exception.message?.contains("Artboard operation failed") == true ||
                exception.message?.contains("Artboard not found") == true ||
                exception.message?.contains("Failed to create artboard") == true,
                "Expected error for non-existent artboard, got: ${exception.message}"
            )
            
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
    
    /**
     * Test that artboard handles are unique.
     */
    @Test
    fun artboardHandlesAreUnique() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            val artboardHandle1 = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val artboardHandle2 = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            
            // Handles should be different
            assertNotEquals(
                artboardHandle1.handle,
                artboardHandle2.handle,
                "Artboard handles should be unique"
            )
            
            // Cleanup
            testUtil.commandQueue.deleteArtboard(artboardHandle1)
            testUtil.commandQueue.deleteArtboard(artboardHandle2)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
    
    /**
     * Test file with no artboards.
     */
    @Test
    fun fileWithNoArtboard() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("noartboard.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            val artboardNames = testUtil.commandQueue.getArtboardNames(fileHandle)
            assertEquals(0, artboardNames.size, "Expected 0 artboards in noartboard.riv")
            
            // Try to create default artboard - should fail
            val exception = assertFailsWith<IllegalArgumentException> {
                testUtil.commandQueue.createDefaultArtboard(fileHandle)
            }
            
            assertTrue(
                exception.message?.contains("Artboard operation failed") == true ||
                exception.message?.contains("Failed to create") == true,
                "Expected error for file with no artboards"
            )
            
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
    
    /**
     * Test querying state machine names from an artboard.
     */
    @Test
    fun queryStateMachineNames() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            
            // Query state machine names
            val smNames = testUtil.commandQueue.getStateMachineNames(artboardHandle)
            
            // flux_capacitor should have at least one state machine
            assertTrue(smNames.isNotEmpty(), "Expected at least 1 state machine")
            
            // Cleanup
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
    
    /**
     * Test that long artboard names are handled correctly.
     */
    @Test
    fun longArtboardName() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("long_artboard_name.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            val artboardNames = testUtil.commandQueue.getArtboardNames(fileHandle)
            assertTrue(artboardNames.isNotEmpty(), "Expected at least 1 artboard")
            
            // The artboard name should be very long
            val longName = artboardNames[0]
            assertTrue(longName.length > 50, "Expected artboard name to be very long")
            
            // Should be able to create artboard by long name
            val artboardHandle = testUtil.commandQueue.createArtboardByName(fileHandle, longName)
            assertNotEquals(0L, artboardHandle.handle, "Artboard handle should be non-zero")
            
            // Cleanup
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
}