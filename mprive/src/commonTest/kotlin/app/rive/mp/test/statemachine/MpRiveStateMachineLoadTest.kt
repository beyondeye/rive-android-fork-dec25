package app.rive.mp.test.statemachine

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
 * Tests for state machine loading and querying operations via CommandQueue.
 *
 * Ported from kotlin/src/androidTest/kotlin/app/rive/runtime/kotlin/core/RiveStateMachineLoadTest.kt
 *
 * ## Implemented Tests
 * - State machine creation (default and by name)
 * - State machine name queries
 * - Error handling for invalid handles/names
 * - Handle uniqueness
 *
 * Note: State machine input tests are in MpRiveStateMachineInstanceTest.kt
 */
class MpRiveStateMachineLoadTest {

    init {
        MpTestContext.initPlatform()
    }

    // =========================================================================
    // State Machine Creation Tests
    // =========================================================================

    /**
     * Test creating default state machine from an artboard.
     */
    @Test
    fun createDefaultStateMachine() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)

            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            // State machine handle should be valid (non-zero)
            assertNotEquals(0L, smHandle.handle, "State machine handle should be non-zero")

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test creating state machine by name.
     */
    @Test
    fun createStateMachineByName() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("multipleartboards.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createArtboardByName(fileHandle, "artboard1")

            // Get state machine names first
            val smNames = testUtil.commandQueue.getStateMachineNames(artboardHandle)
            assertTrue(smNames.isNotEmpty(), "Expected at least 1 state machine")

            // Create by name
            val smHandle = testUtil.commandQueue.createStateMachineByName(artboardHandle, smNames[0])

            // State machine handle should be valid (non-zero)
            assertNotEquals(0L, smHandle.handle, "State machine handle should be non-zero")

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test that state machine handles are unique.
     */
    @Test
    fun stateMachineHandlesAreUnique() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)

            // Create multiple state machines from the same artboard
            val smHandle1 = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)
            val smHandle2 = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            // Handles should be different
            assertNotEquals(
                smHandle1.handle,
                smHandle2.handle,
                "State machine handles should be unique"
            )

            // Both handles should be valid
            assertNotEquals(0L, smHandle1.handle)
            assertNotEquals(0L, smHandle2.handle)

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle1)
            testUtil.commandQueue.deleteStateMachine(smHandle2)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    // =========================================================================
    // State Machine Query Tests
    // =========================================================================

    /**
     * Test querying state machine names from artboard with single state machine.
     */
    @Test
    fun queryStateMachineNamesFromArtboard1() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("multipleartboards.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createArtboardByName(fileHandle, "artboard1")

            val smNames = testUtil.commandQueue.getStateMachineNames(artboardHandle)

            // artboard1 should have exactly 1 state machine
            assertEquals(1, smNames.size, "Expected 1 state machine in artboard1")
            assertEquals("artboard1stateMachine1", smNames[0])

            // Cleanup
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test querying state machine names from artboard with multiple state machines.
     */
    @Test
    fun queryStateMachineNamesFromArtboard2() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("multipleartboards.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createArtboardByName(fileHandle, "artboard2")

            val smNames = testUtil.commandQueue.getStateMachineNames(artboardHandle)

            // artboard2 should have 2 state machines
            assertEquals(2, smNames.size, "Expected 2 state machines in artboard2")
            assertTrue(smNames.contains("artboard2stateMachine1"), "Expected artboard2stateMachine1")
            assertTrue(smNames.contains("artboard2stateMachine2"), "Expected artboard2stateMachine2")

            // Cleanup
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test that files with no state machines get an auto-generated state machine.
     */
    @Test
    fun artboardWithNoStateMachines() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("noanimation.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)

            val smNames = testUtil.commandQueue.getStateMachineNames(artboardHandle)

            // Files without state machines get an auto-generated one
            assertEquals(1, smNames.size, "Expected 1 auto-generated state machine")
            assertEquals("Auto Generated State Machine", smNames[0])

            // Cleanup
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
     * Test that creating state machine with invalid artboard handle fails.
     */
    @Test
    fun createStateMachineWithInvalidArtboardHandle() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)

            // Delete the artboard to make the handle invalid
            testUtil.commandQueue.deleteArtboard(artboardHandle)

            // Try to create state machine with deleted artboard handle
            val exception = assertFailsWith<IllegalArgumentException> {
                testUtil.commandQueue.createDefaultStateMachine(artboardHandle)
            }

            assertTrue(
                exception.message?.contains("Invalid artboard handle") == true ||
                exception.message?.contains("State machine operation failed") == true ||
                exception.message?.contains("Failed to create") == true,
                "Expected error for invalid artboard handle, got: ${exception.message}"
            )

            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test that creating state machine by non-existent name fails.
     */
    @Test
    fun createStateMachineByNonExistentName() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)

            // Try to create state machine with non-existent name
            val exception = assertFailsWith<IllegalArgumentException> {
                testUtil.commandQueue.createStateMachineByName(artboardHandle, "nonexistent_sm")
            }

            assertTrue(
                exception.message?.contains("State machine not found") == true ||
                exception.message?.contains("State machine operation failed") == true ||
                exception.message?.contains("Failed to create") == true,
                "Expected error for non-existent state machine, got: ${exception.message}"
            )

            // Cleanup
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test that querying state machine names with invalid artboard handle fails.
     */
    @Test
    fun queryStateMachineNamesWithInvalidHandle() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)

            // Delete the artboard
            testUtil.commandQueue.deleteArtboard(artboardHandle)

            // Try to query state machine names with deleted artboard handle
            val exception = assertFailsWith<IllegalArgumentException> {
                testUtil.commandQueue.getStateMachineNames(artboardHandle)
            }

            assertTrue(
                exception.message?.contains("Invalid artboard handle") == true ||
                exception.message?.contains("Query failed") == true,
                "Expected error for invalid artboard handle, got: ${exception.message}"
            )

            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    // =========================================================================
    // Deletion Tests
    // =========================================================================

    /**
     * Test deleting a state machine.
     */
    @Test
    fun deleteStateMachine() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            // Delete the state machine (should not throw)
            testUtil.commandQueue.deleteStateMachine(smHandle)

            // Cleanup
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
}
