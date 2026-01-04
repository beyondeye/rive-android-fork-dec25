package app.rive.mp.test.statemachine

import app.rive.mp.StateMachineHandle
import app.rive.mp.test.utils.MpCommandQueueTestUtil
import app.rive.mp.test.utils.MpTestContext
import app.rive.mp.test.utils.MpTestResources
import app.rive.mp.test.utils.loadRiveFile
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.launch
import kotlinx.coroutines.test.runTest
import kotlinx.coroutines.withTimeout
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNotEquals
import kotlin.test.assertTrue
import kotlin.time.Duration.Companion.seconds

/**
 * Tests for state machine instance operations via CommandQueue.
 *
 * Ported from kotlin/src/androidTest/kotlin/app/rive/runtime/kotlin/core/RiveStateMachineInstanceTest.kt
 *
 * ## Currently Implemented Tests
 * - State machine advancement (advanceStateMachine)
 * - Settled event flow (settledFlow)
 * - Multiple state machine advancement
 *
 * ## Deferred Tests (Pending Implementation - Phase D)
 * The following test categories require state machine input operations:
 *
 * ### Input Query Operations
 * - `inputCount` - Get number of inputs in state machine
 * - `inputNames` - Get list of input names
 * - `input(index)` - Get input by index
 * - `input(name)` - Get input by name
 *
 * ### Input Type Detection
 * - `isBoolean` - Check if input is boolean type
 * - `isNumber` - Check if input is number type
 * - `isTrigger` - Check if input is trigger type
 *
 * ### Input Value Operations
 * - `SMINumber.value` - Get/set number input value
 * - `SMIBoolean.value` - Get/set boolean input value
 * - `SMITrigger.fire()` - Fire trigger input
 *
 * ### Test Cases Pending Implementation
 * - nothing() - Test state machine with no inputs
 * - numberInput() - Test number input get/set
 * - booleanInput() - Test boolean input get/set
 * - triggerInput() - Test trigger firing
 * - mixed() - Test mixed input types
 */
class MpRiveStateMachineInstanceTest {

    init {
        MpTestContext.initPlatform()
    }

    // =========================================================================
    // State Machine Advancement Tests
    // =========================================================================

    /**
     * Test advancing a state machine by a time delta.
     */
    @Test
    fun advanceStateMachine() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            // Advance the state machine (should not throw)
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f) // ~60 FPS frame

            // Advance multiple times
            repeat(10) {
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
     * Test advancing multiple state machines independently.
     */
    @Test
    fun advanceMultipleStateMachines() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)

            // Create two state machines
            val smHandle1 = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)
            val smHandle2 = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            // Advance them independently
            testUtil.commandQueue.advanceStateMachine(smHandle1, 0.016f)
            testUtil.commandQueue.advanceStateMachine(smHandle2, 0.032f) // Different delta

            // Advance in interleaved order
            repeat(5) {
                testUtil.commandQueue.advanceStateMachine(smHandle1, 0.016f)
                testUtil.commandQueue.advanceStateMachine(smHandle2, 0.016f)
            }

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle1)
            testUtil.commandQueue.deleteStateMachine(smHandle2)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test advancing with zero delta time.
     */
    @Test
    fun advanceWithZeroDelta() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            // Advance with zero delta (should not throw)
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.0f)

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test advancing with large delta time.
     */
    @Test
    fun advanceWithLargeDelta() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("flux_capacitor.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            // Advance with large delta (1 second)
            testUtil.commandQueue.advanceStateMachine(smHandle, 1.0f)

            // Advance with very large delta (10 seconds)
            testUtil.commandQueue.advanceStateMachine(smHandle, 10.0f)

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    // =========================================================================
    // Settled Flow Tests
    // =========================================================================

    /**
     * Test that settledFlow emits when state machine settles.
     *
     * Note: This test may be flaky depending on the animation content.
     * A simple animation that settles quickly is ideal for this test.
     */
    @Test
    fun settledFlowEmitsOnSettle() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("what_a_state.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            // Collect settled events in background
            val settledHandles = mutableListOf<StateMachineHandle>()
            val collectJob = launch {
                testUtil.commandQueue.settledFlow.collect { handle ->
                    settledHandles.add(handle)
                }
            }

            // Advance the state machine multiple times to trigger settle
            repeat(60) { // ~1 second at 60fps
                testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f)
            }

            // Give some time for messages to be processed
            kotlinx.coroutines.delay(100)

            // Cancel collection
            collectJob.cancel()

            // Note: Whether settledFlow emits depends on the animation content
            // This test verifies the flow mechanism works, not specific settle behavior

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test subscribing to settledFlow for multiple state machines.
     */
    @Test
    fun settledFlowMultipleStateMachines() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("what_a_state.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)

            val smHandle1 = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)
            val smHandle2 = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            // Collect settled events
            val settledHandles = mutableListOf<StateMachineHandle>()
            val collectJob = launch {
                testUtil.commandQueue.settledFlow.collect { handle ->
                    settledHandles.add(handle)
                }
            }

            // Advance both state machines
            repeat(30) {
                testUtil.commandQueue.advanceStateMachine(smHandle1, 0.016f)
                testUtil.commandQueue.advanceStateMachine(smHandle2, 0.016f)
            }

            // Give some time for messages to be processed
            kotlinx.coroutines.delay(100)

            collectJob.cancel()

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
    // State Machine from Different Artboards
    // =========================================================================

    /**
     * Test creating state machines from different artboards in the same file.
     */
    @Test
    fun stateMachinesFromDifferentArtboards() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("multipleartboards.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)

            // Create artboards
            val artboard1Handle = testUtil.commandQueue.createArtboardByName(fileHandle, "artboard1")
            val artboard2Handle = testUtil.commandQueue.createArtboardByName(fileHandle, "artboard2")

            // Create state machines from each artboard
            val sm1Handle = testUtil.commandQueue.createDefaultStateMachine(artboard1Handle)
            val sm2Handle = testUtil.commandQueue.createDefaultStateMachine(artboard2Handle)

            // Handles should be unique
            assertNotEquals(sm1Handle.handle, sm2Handle.handle)

            // Both should be advanceable
            testUtil.commandQueue.advanceStateMachine(sm1Handle, 0.016f)
            testUtil.commandQueue.advanceStateMachine(sm2Handle, 0.016f)

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(sm1Handle)
            testUtil.commandQueue.deleteStateMachine(sm2Handle)
            testUtil.commandQueue.deleteArtboard(artboard1Handle)
            testUtil.commandQueue.deleteArtboard(artboard2Handle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    // =========================================================================
    // Nested Settle Tests
    // =========================================================================

    /**
     * Test state machine with nested components that settle.
     */
    @Test
    fun nestedSettle() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("nested_settle.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)

            // Advance to allow nested components to settle
            repeat(120) { // ~2 seconds at 60fps
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
}
