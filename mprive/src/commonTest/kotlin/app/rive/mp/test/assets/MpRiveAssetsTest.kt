package app.rive.mp.test.assets

import app.rive.mp.test.utils.MpCommandQueueTestUtil
import app.rive.mp.test.utils.MpTestContext
import app.rive.mp.test.utils.MpTestResources
import app.rive.mp.test.utils.loadImageFile
import app.rive.mp.test.utils.loadRiveFile
import kotlinx.coroutines.test.runTest
import kotlin.test.Test
import kotlin.test.assertFailsWith
import kotlin.test.assertNotEquals
import kotlin.test.assertTrue

/**
 * Tests for Phase E.1: Asset Management operations via CommandQueue.
 * 
 * These tests verify the asset decode/delete/register/unregister lifecycle
 * for images, audio, and fonts.
 * 
 * Note: Some tests may require a GPU context to fully execute. Tests that
 * don't require actual decoding (like API signature verification) should
 * pass in all environments.
 */
class MpRiveAssetsTest {
    
    init {
        MpTestContext.initPlatform()
    }
    
    // =============================================================================
    // Image Tests
    // =============================================================================
    
    /**
     * Test decoding a PNG image file.
     * 
     * This test verifies that:
     * 1. A valid PNG file can be decoded
     * 2. The returned handle is non-zero
     */
    @Test
    fun decodeImage_validPng_returnsHandle() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            // Load the test PNG image
            val imageBytes = MpTestResources.loadImageFile("eve.png")
            assertTrue(imageBytes.isNotEmpty(), "Image file should not be empty")
            
            // Decode the image
            val imageHandle = testUtil.commandQueue.decodeImage(imageBytes)
            
            // Verify handle is valid
            assertNotEquals(0L, imageHandle.handle, "Image handle should be non-zero")
            
            // Cleanup
            testUtil.commandQueue.deleteImage(imageHandle)
        } finally {
            testUtil.cleanup()
        }
    }
    
    /**
     * Test that decoding invalid image data fails appropriately.
     */
    @Test
    fun decodeImage_invalidData_throwsException() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            // Create invalid image bytes
            val invalidBytes = byteArrayOf(0, 1, 2, 3, 4, 5, 6, 7)
            
            // Attempt to decode - should fail
            assertFailsWith<IllegalArgumentException> {
                testUtil.commandQueue.decodeImage(invalidBytes)
            }
        } finally {
            testUtil.cleanup()
        }
    }
    
    /**
     * Test the full image lifecycle: decode -> register -> unregister -> delete
     */
    @Test
    fun imageLifecycle_fullCycle_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            // Load and decode image
            val imageBytes = MpTestResources.loadImageFile("eve.png")
            val imageHandle = testUtil.commandQueue.decodeImage(imageBytes)
            assertNotEquals(0L, imageHandle.handle, "Image handle should be non-zero")
            
            // Register the image with a name
            testUtil.commandQueue.registerImage("test-image", imageHandle)
            
            // Unregister the image
            testUtil.commandQueue.unregisterImage("test-image")
            
            // Delete the image
            testUtil.commandQueue.deleteImage(imageHandle)
            
            // If we got here without exceptions, the test passed
        } finally {
            testUtil.cleanup()
        }
    }
    
    /**
     * Test that multiple images can be decoded and each gets a unique handle.
     */
    @Test
    fun decodeImage_multipleImages_uniqueHandles() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val imageBytes = MpTestResources.loadImageFile("eve.png")
            
            // Decode the same image twice
            val handle1 = testUtil.commandQueue.decodeImage(imageBytes)
            val handle2 = testUtil.commandQueue.decodeImage(imageBytes)
            
            // Handles should be different
            assertNotEquals(
                handle1.handle,
                handle2.handle,
                "Each decode should return a unique handle"
            )
            
            // Cleanup
            testUtil.commandQueue.deleteImage(handle1)
            testUtil.commandQueue.deleteImage(handle2)
        } finally {
            testUtil.cleanup()
        }
    }
    
    /**
     * Test registering the same image with multiple names.
     */
    @Test
    fun registerImage_multipleNames_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val imageBytes = MpTestResources.loadImageFile("eve.png")
            val imageHandle = testUtil.commandQueue.decodeImage(imageBytes)
            
            // Register with multiple names
            testUtil.commandQueue.registerImage("name1", imageHandle)
            testUtil.commandQueue.registerImage("name2", imageHandle)
            testUtil.commandQueue.registerImage("name3", imageHandle)
            
            // Unregister all
            testUtil.commandQueue.unregisterImage("name1")
            testUtil.commandQueue.unregisterImage("name2")
            testUtil.commandQueue.unregisterImage("name3")
            
            // Delete the image
            testUtil.commandQueue.deleteImage(imageHandle)
        } finally {
            testUtil.cleanup()
        }
    }
    
    // =============================================================================
    // Audio Tests
    // =============================================================================
    
    /**
     * Test that decoding invalid audio data fails appropriately.
     * 
     * Note: We don't have a valid audio file in test resources,
     * so we test the error path.
     */
    @Test
    fun decodeAudio_invalidData_throwsException() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            // Create invalid audio bytes
            val invalidBytes = byteArrayOf(0, 1, 2, 3, 4, 5, 6, 7)
            
            // Attempt to decode - should fail
            assertFailsWith<IllegalArgumentException> {
                testUtil.commandQueue.decodeAudio(invalidBytes)
            }
        } finally {
            testUtil.cleanup()
        }
    }
    
    // =============================================================================
    // Font Tests
    // =============================================================================
    
    /**
     * Test that decoding invalid font data fails appropriately.
     * 
     * Note: We don't have a valid font file in test resources,
     * so we test the error path.
     */
    @Test
    fun decodeFont_invalidData_throwsException() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            // Create invalid font bytes
            val invalidBytes = byteArrayOf(0, 1, 2, 3, 4, 5, 6, 7)
            
            // Attempt to decode - should fail
            assertFailsWith<IllegalArgumentException> {
                testUtil.commandQueue.decodeFont(invalidBytes)
            }
        } finally {
            testUtil.cleanup()
        }
    }
    
    // =============================================================================
    // Integration Tests
    // =============================================================================
    
    /**
     * Test loading a Rive file that references external assets.
     * 
     * This test demonstrates the asset registration workflow:
     * 1. Decode an image
     * 2. Register it with a name matching the asset reference
     * 3. Load the Rive file (which should use the registered asset)
     * 
     * Note: This requires a Rive file with external asset references (cdn_image.riv)
     */
    @Test
    fun assetRegistration_withRiveFile_workflow() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            // First, decode and register an image
            val imageBytes = MpTestResources.loadImageFile("eve.png")
            val imageHandle = testUtil.commandQueue.decodeImage(imageBytes)
            
            // Register the image (the name should match what's in the Rive file)
            testUtil.commandQueue.registerImage("eve.png", imageHandle)
            
            // Now load a Rive file that references images
            // cdn_image.riv is a good candidate as it likely references external images
            val riveBytes = MpTestResources.loadRiveFile("cdn_image.riv")
            val fileHandle = testUtil.commandQueue.loadFile(riveBytes)
            
            // The file should load successfully (even if it uses the registered asset)
            assertNotEquals(0L, fileHandle.handle, "File handle should be non-zero")
            
            // Cleanup
            testUtil.commandQueue.deleteFile(fileHandle)
            testUtil.commandQueue.unregisterImage("eve.png")
            testUtil.commandQueue.deleteImage(imageHandle)
        } finally {
            testUtil.cleanup()
        }
    }
    
    /**
     * Test that empty byte arrays fail gracefully.
     */
    @Test
    fun decodeImage_emptyBytes_throwsException() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val emptyBytes = byteArrayOf()
            
            assertFailsWith<IllegalArgumentException> {
                testUtil.commandQueue.decodeImage(emptyBytes)
            }
        } finally {
            testUtil.cleanup()
        }
    }
}