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
 * Tests for asset management operations via CommandQueue (Phase E.1).
 *
 * These tests verify that the asset decode, register, and unregister operations
 * work correctly for images, audio, and fonts.
 *
 * ## Test Categories
 * - Image decoding and registration
 * - Font decoding and registration
 * - Asset deletion
 * - Error handling
 *
 * ## Note on Implementation
 * The C++ handlers for asset operations currently have placeholder implementations
 * (TODO comments for actual decoding). These tests verify the full Kotlin’JNI’C++
 * roundtrip works correctly and that handles are properly returned.
 */
class MpRiveAssetsTest {

    init {
        MpTestContext.initPlatform()
    }

    // =========================================================================
    // Image Decoding Tests
    // =========================================================================

    /**
     * Test that decodeImage returns a valid handle for a PNG image.
     */
    @Test
    fun decodeImage_validPng_returnsHandle() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val imageBytes = MpTestResources.loadImageFile("eve.png")
            assertTrue(imageBytes.isNotEmpty(), "Image bytes should not be empty")

            val imageHandle = testUtil.commandQueue.decodeImage(imageBytes)
            assertNotEquals(0L, imageHandle.handle, "Image handle should not be 0")

            // Cleanup
            testUtil.commandQueue.deleteImage(imageHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test that decoding an invalid image throws an exception.
     */
    @Test
    fun decodeImage_invalidBytes_throwsException() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val invalidBytes = byteArrayOf(0x00, 0x01, 0x02, 0x03, 0x04)

            assertFailsWith<IllegalArgumentException> {
                testUtil.commandQueue.decodeImage(invalidBytes)
            }
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test that decoding an empty byte array throws an exception.
     */
    @Test
    fun decodeImage_emptyBytes_throwsException() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            assertFailsWith<IllegalArgumentException> {
                testUtil.commandQueue.decodeImage(byteArrayOf())
            }
        } finally {
            testUtil.cleanup()
        }
    }

    // =========================================================================
    // Image Registration Tests
    // =========================================================================

    /**
     * Test that an image can be registered and unregistered.
     */
    @Test
    fun registerImage_validImage_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val imageBytes = MpTestResources.loadImageFile("eve.png")
            val imageHandle = testUtil.commandQueue.decodeImage(imageBytes)

            // Register should not throw
            testUtil.commandQueue.registerImage("test-image", imageHandle)

            // Unregister should not throw
            testUtil.commandQueue.unregisterImage("test-image")

            // Cleanup
            testUtil.commandQueue.deleteImage(imageHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test that the same image can be registered with multiple names.
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

            // Cleanup
            testUtil.commandQueue.deleteImage(imageHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    // =========================================================================
    // Font Decoding Tests
    // =========================================================================

    /**
     * Test that decodeFont returns a valid handle for a TTF font.
     */
    @Test
    fun decodeFont_validTtf_returnsHandle() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val fontBytes = MpTestResources.loadResource("rive/font.ttf")
            assertTrue(fontBytes.isNotEmpty(), "Font bytes should not be empty")

            val fontHandle = testUtil.commandQueue.decodeFont(fontBytes)
            assertNotEquals(0L, fontHandle.handle, "Font handle should not be 0")

            // Cleanup
            testUtil.commandQueue.deleteFont(fontHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test that decoding invalid font bytes throws an exception.
     */
    @Test
    fun decodeFont_invalidBytes_throwsException() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val invalidBytes = byteArrayOf(0x00, 0x01, 0x02, 0x03, 0x04)

            assertFailsWith<IllegalArgumentException> {
                testUtil.commandQueue.decodeFont(invalidBytes)
            }
        } finally {
            testUtil.cleanup()
        }
    }

    // =========================================================================
    // Font Registration Tests
    // =========================================================================

    /**
     * Test that a font can be registered and unregistered.
     */
    @Test
    fun registerFont_validFont_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val fontBytes = MpTestResources.loadResource("rive/font.ttf")
            val fontHandle = testUtil.commandQueue.decodeFont(fontBytes)

            // Register should not throw
            testUtil.commandQueue.registerFont("test-font", fontHandle)

            // Unregister should not throw
            testUtil.commandQueue.unregisterFont("test-font")

            // Cleanup
            testUtil.commandQueue.deleteFont(fontHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    // =========================================================================
    // Asset Deletion Tests
    // =========================================================================

    /**
     * Test that deleting an image does not throw.
     */
    @Test
    fun deleteImage_validHandle_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val imageBytes = MpTestResources.loadImageFile("eve.png")
            val imageHandle = testUtil.commandQueue.decodeImage(imageBytes)

            // Delete should not throw
            testUtil.commandQueue.deleteImage(imageHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test that deleting a font does not throw.
     */
    @Test
    fun deleteFont_validHandle_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val fontBytes = MpTestResources.loadResource("rive/font.ttf")
            val fontHandle = testUtil.commandQueue.decodeFont(fontBytes)

            // Delete should not throw
            testUtil.commandQueue.deleteFont(fontHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    // =========================================================================
    // Integration Tests
    // =========================================================================

    /**
     * Test loading a Rive file that references external assets.
     * This test verifies the asset registration workflow.
     */
    @Test
    fun loadFileWithAssets_afterRegistration_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            // Load and register an image asset first
            val imageBytes = MpTestResources.loadImageFile("eve.png")
            val imageHandle = testUtil.commandQueue.decodeImage(imageBytes)
            testUtil.commandQueue.registerImage("eve.png", imageHandle)

            // Now load a Rive file (even if it doesn't reference eve.png,
            // this tests the registration infrastructure works)
            val riveBytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(riveBytes)

            // Clean up in reverse order
            testUtil.commandQueue.deleteFile(fileHandle)
            testUtil.commandQueue.unregisterImage("eve.png")
            testUtil.commandQueue.deleteImage(imageHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test the full asset lifecycle: decode -> register -> use -> unregister -> delete
     */
    @Test
    fun assetLifecycle_fullCycle_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            // 1. Decode image
            val imageBytes = MpTestResources.loadImageFile("eve.png")
            val imageHandle = testUtil.commandQueue.decodeImage(imageBytes)
            assertNotEquals(0L, imageHandle.handle)

            // 2. Register image
            testUtil.commandQueue.registerImage("lifecycle-test-image", imageHandle)

            // 3. Decode font
            val fontBytes = MpTestResources.loadResource("rive/font.ttf")
            val fontHandle = testUtil.commandQueue.decodeFont(fontBytes)
            assertNotEquals(0L, fontHandle.handle)

            // 4. Register font
            testUtil.commandQueue.registerFont("lifecycle-test-font", fontHandle)

            // 5. Load a Rive file to simulate "use"
            val riveBytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(riveBytes)

            // 6. Cleanup in proper order
            testUtil.commandQueue.deleteFile(fileHandle)
            testUtil.commandQueue.unregisterFont("lifecycle-test-font")
            testUtil.commandQueue.deleteFont(fontHandle)
            testUtil.commandQueue.unregisterImage("lifecycle-test-image")
            testUtil.commandQueue.deleteImage(imageHandle)
        } finally {
            testUtil.cleanup()
        }
    }
}