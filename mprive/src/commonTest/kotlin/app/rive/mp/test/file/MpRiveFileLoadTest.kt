package app.rive.mp.test.file

import app.rive.mp.test.utils.MpCommandQueueTestUtil
import app.rive.mp.test.utils.MpTestContext
import app.rive.mp.test.utils.MpTestResources
import app.rive.mp.test.utils.loadRiveFile
import kotlinx.coroutines.test.runTest
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue

/**
 * Tests for file loading operations via CommandQueue.
 * Ported from kotlin/src/androidTest/kotlin/app/rive/runtime/kotlin/core/RiveFileLoadTest.kt
 */
class MpRiveFileLoadTest {
    
    init {
        MpTestContext.initPlatform()
    }
    
    /**
     * Test loading a malformed file (format version 6 - unsupported).
     * Should throw an exception.
     */
    @Test
    fun loadFormat6() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("sample6.riv")
            
            // This should throw an exception
            val exception = assertFailsWith<IllegalArgumentException> {
                testUtil.commandQueue.loadFile(bytes)
            }
            
            assertTrue(
                exception.message?.contains("Failed to load file") == true,
                "Expected file load error for unsupported format"
            )
        } finally {
            testUtil.cleanup()
        }
    }
    
    /**
     * Test loading a junk file (not a valid Rive file).
     * Should throw an exception.
     */
    @Test
    fun loadJunk() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("junk.riv")
            
            // This should throw an exception
            val exception = assertFailsWith<IllegalArgumentException> {
                testUtil.commandQueue.loadFile(bytes)
            }
            
            assertTrue(
                exception.message?.contains("Failed to load file") == true,
                "Expected file load error for malformed file"
            )
        } finally {
            testUtil.cleanup()
        }
    }
    
    /**
     * Test loading a valid Rive file (flux capacitor).
     * Should load successfully and have 1 animation.
     */
    @Test
    fun loadFormatFlux() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            // Get artboard names to verify the file loaded
            val artboardNames = testUtil.commandQueue.getArtboardNames(fileHandle)
            assertEquals(1, artboardNames.size, "Expected 1 artboard in flux_capacitor.riv")
            
            // Cleanup
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
    
    /**
     * Test loading a valid Rive file (off-road car).
     * Should load successfully.
     */
    @Test
    fun loadFormatBuggy() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("off_road_car_blog.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            // Get artboard names to verify the file loaded
            val artboardNames = testUtil.commandQueue.getArtboardNames(fileHandle)
            assertTrue(artboardNames.isNotEmpty(), "Expected at least 1 artboard in off_road_car_blog.riv")
            
            // Cleanup
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
    
    /**
     * Test that file handles are unique.
     */
    @Test
    fun fileHandlesAreUnique() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            
            val fileHandle1 = testUtil.commandQueue.loadFile(bytes)
            val fileHandle2 = testUtil.commandQueue.loadFile(bytes)
            
            // Handles should be different
            assertTrue(
                fileHandle1.handle != fileHandle2.handle,
                "File handles should be unique"
            )
            
            // Cleanup
            testUtil.commandQueue.deleteFile(fileHandle1)
            testUtil.commandQueue.deleteFile(fileHandle2)
        } finally {
            testUtil.cleanup()
        }
    }
    
    /**
     * Test that invalid file handles are rejected.
     */
    @Test
    fun invalidFileHandleRejected() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            // Delete the file
            testUtil.commandQueue.deleteFile(fileHandle)
            
            // Try to query artboards from deleted file - should fail
            val exception = assertFailsWith<IllegalArgumentException> {
                testUtil.commandQueue.getArtboardNames(fileHandle)
            }
            
            assertTrue(
                exception.message?.contains("Query failed") == true ||
                exception.message?.contains("Invalid file handle") == true,
                "Expected error for invalid file handle"
            )
        } finally {
            testUtil.cleanup()
        }
    }
}
