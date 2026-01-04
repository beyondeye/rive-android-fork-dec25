package app.rive.mp.test.commandqueue

import app.rive.mp.test.utils.MpCommandQueueTestUtil
import app.rive.mp.test.utils.MpTestContext
import app.rive.mp.test.utils.MpTestResources
import kotlinx.coroutines.test.runTest
import kotlin.test.*

/**
 * Phase A test for CommandQueue handle management.
 * Tests handle uniqueness, incrementing, and lifecycle.
 */
class MpCommandQueueHandleTest {
    
    init {
        MpTestContext.initPlatform()
    }
    
    @Test
    fun loadFile_returns_unique_handles() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes1 = MpTestResources.loadRiveFile("flux_capacitor")
            val bytes2 = MpTestResources.loadRiveFile("off_road_car_blog")
            
            val handle1 = testUtil.commandQueue.loadFile(bytes1)
            val handle2 = testUtil.commandQueue.loadFile(bytes2)
            
            assertNotEquals(handle1.handle, handle2.handle, "File handles should be unique")
            assertTrue(handle1.handle > 0, "Handle 1 should be positive")
            assertTrue(handle2.handle > 0, "Handle 2 should be positive")
            
            // Cleanup
            testUtil.commandQueue.deleteFile(handle1)
            testUtil.commandQueue.deleteFile(handle2)
        } finally {
            testUtil.cleanup()
        }
    }
    
    @Test
    fun handles_are_incrementing() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor")
            
            val handle1 = testUtil.commandQueue.loadFile(bytes)
            val handle2 = testUtil.commandQueue.loadFile(bytes)
            val handle3 = testUtil.commandQueue.loadFile(bytes)
            
            assertTrue(handle2.handle > handle1.handle, "Handle 2 should be greater than Handle 1")
            assertTrue(handle3.handle > handle2.handle, "Handle 3 should be greater than Handle 2")
            
            // Cleanup
            testUtil.commandQueue.deleteFile(handle1)
            testUtil.commandQueue.deleteFile(handle2)
            testUtil.commandQueue.deleteFile(handle3)
        } finally {
            testUtil.cleanup()
        }
    }
    
    @Test
    fun artboard_handles_are_unique() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            val artboardHandle1 = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val artboardHandle2 = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            
            assertNotEquals(
                artboardHandle1.handle,
                artboardHandle2.handle,
                "Artboard handles should be unique"
            )
            assertTrue(artboardHandle1.handle > 0, "Artboard handle 1 should be positive")
            assertTrue(artboardHandle2.handle > 0, "Artboard handle 2 should be positive")
            
            // Cleanup
            testUtil.commandQueue.deleteArtboard(artboardHandle1)
            testUtil.commandQueue.deleteArtboard(artboardHandle2)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
    
    @Test
    fun artboard_handles_are_incrementing() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            val artboardHandle1 = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val artboardHandle2 = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val artboardHandle3 = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            
            assertTrue(
                artboardHandle2.handle > artboardHandle1.handle,
                "Artboard handle 2 should be greater than handle 1"
            )
            assertTrue(
                artboardHandle3.handle > artboardHandle2.handle,
                "Artboard handle 3 should be greater than handle 2"
            )
            
            // Cleanup
            testUtil.commandQueue.deleteArtboard(artboardHandle1)
            testUtil.commandQueue.deleteArtboard(artboardHandle2)
            testUtil.commandQueue.deleteArtboard(artboardHandle3)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
    
    @Test
    fun deleted_file_handle_becomes_invalid() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            // Delete the file
            testUtil.commandQueue.deleteFile(fileHandle)
            
            // Try to use the deleted handle - should fail
            assertFailsWith<IllegalArgumentException> {
                testUtil.commandQueue.getArtboardNames(fileHandle)
            }
        } finally {
            testUtil.cleanup()
        }
    }
    
    @Test
    fun deleted_artboard_handle_becomes_invalid() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            
            // Delete the artboard
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            
            // Try to use the deleted handle - should fail
            assertFailsWith<IllegalArgumentException> {
                testUtil.commandQueue.getStateMachineNames(artboardHandle)
            }
            
            // Cleanup file
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
    
    @Test
    fun same_file_loaded_multiple_times_gets_different_handles() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor")
            
            val handles = mutableListOf<Long>()
            repeat(5) {
                val handle = testUtil.commandQueue.loadFile(bytes)
                handles.add(handle.handle)
            }
            
            // All handles should be unique
            assertEquals(handles.size, handles.toSet().size, "All handles should be unique")
            
            // All handles should be positive
            assertTrue(handles.all { it > 0 }, "All handles should be positive")
            
            // Cleanup
            handles.forEach { handle ->
                testUtil.commandQueue.deleteFile(
                    app.rive.mp.FileHandle(handle)
                )
            }
        } finally {
            testUtil.cleanup()
        }
    }
    
    @Test
    fun handles_remain_valid_across_operations() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("multipleartboards")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            // Create multiple artboards
            val artboard1 = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val artboard2 = testUtil.commandQueue.createArtboardByName(fileHandle, "artboard2")
            
            // File handle should still be valid after artboard creation
            val artboardNames = testUtil.commandQueue.getArtboardNames(fileHandle)
            assertEquals(3, artboardNames.size, "File should still have 3 artboards")
            
            // Delete one artboard
            testUtil.commandQueue.deleteArtboard(artboard1)
            
            // File and other artboard handles should still be valid
            val artboardNamesAfterDelete = testUtil.commandQueue.getArtboardNames(fileHandle)
            assertEquals(3, artboardNamesAfterDelete.size, "File should still have 3 artboards")
            
            // Second artboard should still be queryable
            val smNames = testUtil.commandQueue.getStateMachineNames(artboard2)
            assertNotNull(smNames, "State machine names should be queryable")
            
            // Cleanup
            testUtil.commandQueue.deleteArtboard(artboard2)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
}
