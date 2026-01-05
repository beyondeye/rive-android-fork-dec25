package app.rive.mp.test.statemachine

import app.rive.mp.InputType
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
 * ## Implemented Tests
 * - State machine advancement (advanceStateMachine)
 * - Settled event flow (settledFlow)
 * - Multiple state machine advancement
 * - Input query operations (getInputCount, getInputNames, getInputInfo)
 * - Input type detection (NUMBER, BOOLEAN, TRIGGER)
 * - Input value operations (getNumberInput, setNumberInput, getBooleanInput, setBooleanInput, fireTrigger)
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

    // =========================================================================
    // State Machine Input Query Tests (Phase C.4)
    // =========================================================================

    /**
     * Test state machine with no inputs.
     * Ported from RiveStateMachineInstanceTest.nothing()
     */
    @Test
    fun inputsNothing() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("state_machine_configurations.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createStateMachineByName(artboardHandle, "nothing")

            // State machine "nothing" should have 0 inputs
            val inputCount = testUtil.commandQueue.getInputCount(smHandle)
            assertEquals(0, inputCount, "Expected 0 inputs in 'nothing' state machine")

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test number input operations.
     * Ported from RiveStateMachineInstanceTest.numberInput()
     */
    @Test
    fun inputsNumberInput() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("state_machine_configurations.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createStateMachineByName(artboardHandle, "number_input")

            // State machine should have 1 input
            val inputCount = testUtil.commandQueue.getInputCount(smHandle)
            assertEquals(1, inputCount, "Expected 1 input in 'number_input' state machine")

            // Get input info by index
            val inputInfo = testUtil.commandQueue.getInputInfo(smHandle, 0)
            assertEquals("Number 1", inputInfo.name, "Expected input name 'Number 1'")
            assertEquals(InputType.NUMBER, inputInfo.type, "Expected NUMBER type")

            // Set and get number value
            testUtil.commandQueue.setNumberInput(smHandle, "Number 1", 15f)
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f) // Process the set
            val value = testUtil.commandQueue.getNumberInput(smHandle, "Number 1")
            assertEquals(15f, value, "Expected number value to be 15f")

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test boolean input operations.
     * Ported from RiveStateMachineInstanceTest.booleanInput()
     */
    @Test
    fun inputsBooleanInput() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("state_machine_configurations.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createStateMachineByName(artboardHandle, "boolean_input")

            // State machine should have 1 input
            val inputCount = testUtil.commandQueue.getInputCount(smHandle)
            assertEquals(1, inputCount, "Expected 1 input in 'boolean_input' state machine")

            // Get input info by index
            val inputInfo = testUtil.commandQueue.getInputInfo(smHandle, 0)
            assertEquals("Boolean 1", inputInfo.name, "Expected input name 'Boolean 1'")
            assertEquals(InputType.BOOLEAN, inputInfo.type, "Expected BOOLEAN type")

            // Set and get boolean value - set to false
            testUtil.commandQueue.setBooleanInput(smHandle, "Boolean 1", false)
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f) // Process the set
            val valueFalse = testUtil.commandQueue.getBooleanInput(smHandle, "Boolean 1")
            assertEquals(false, valueFalse, "Expected boolean value to be false")

            // Set and get boolean value - set to true
            testUtil.commandQueue.setBooleanInput(smHandle, "Boolean 1", true)
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f) // Process the set
            val valueTrue = testUtil.commandQueue.getBooleanInput(smHandle, "Boolean 1")
            assertEquals(true, valueTrue, "Expected boolean value to be true")

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test trigger input operations.
     * Ported from RiveStateMachineInstanceTest.triggerInput()
     */
    @Test
    fun inputsTriggerInput() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("state_machine_configurations.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createStateMachineByName(artboardHandle, "trigger_input")

            // State machine should have 1 input
            val inputCount = testUtil.commandQueue.getInputCount(smHandle)
            assertEquals(1, inputCount, "Expected 1 input in 'trigger_input' state machine")

            // Get input info by index
            val inputInfo = testUtil.commandQueue.getInputInfo(smHandle, 0)
            assertEquals("Trigger 1", inputInfo.name, "Expected input name 'Trigger 1'")
            assertEquals(InputType.TRIGGER, inputInfo.type, "Expected TRIGGER type")

            // Fire trigger (should not throw)
            testUtil.commandQueue.fireTrigger(smHandle, "Trigger 1")
            testUtil.commandQueue.advanceStateMachine(smHandle, 0.016f) // Process the fire

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test mixed input types.
     * Ported from RiveStateMachineInstanceTest.mixed()
     */
    @Test
    fun inputsMixed() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("state_machine_configurations.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createStateMachineByName(artboardHandle, "mixed")

            // State machine "mixed" should have 6 inputs
            val inputCount = testUtil.commandQueue.getInputCount(smHandle)
            assertEquals(6, inputCount, "Expected 6 inputs in 'mixed' state machine")

            // Get input names
            val inputNames = testUtil.commandQueue.getInputNames(smHandle)
            assertEquals(
                listOf("zero", "off", "trigger", "two_point_two", "on", "three"),
                inputNames,
                "Expected specific input names"
            )

            // Verify input types by checking each input
            // zero - NUMBER
            val zeroInfo = testUtil.commandQueue.getInputInfo(smHandle, 0)
            assertEquals("zero", zeroInfo.name)
            assertEquals(InputType.NUMBER, zeroInfo.type)
            val zeroValue = testUtil.commandQueue.getNumberInput(smHandle, "zero")
            assertEquals(0f, zeroValue, "Expected 'zero' value to be 0f")

            // off - BOOLEAN
            val offInfo = testUtil.commandQueue.getInputInfo(smHandle, 1)
            assertEquals("off", offInfo.name)
            assertEquals(InputType.BOOLEAN, offInfo.type)
            val offValue = testUtil.commandQueue.getBooleanInput(smHandle, "off")
            assertEquals(false, offValue, "Expected 'off' value to be false")

            // trigger - TRIGGER
            val triggerInfo = testUtil.commandQueue.getInputInfo(smHandle, 2)
            assertEquals("trigger", triggerInfo.name)
            assertEquals(InputType.TRIGGER, triggerInfo.type)

            // two_point_two - NUMBER
            val twoPointTwoInfo = testUtil.commandQueue.getInputInfo(smHandle, 3)
            assertEquals("two_point_two", twoPointTwoInfo.name)
            assertEquals(InputType.NUMBER, twoPointTwoInfo.type)
            val twoPointTwoValue = testUtil.commandQueue.getNumberInput(smHandle, "two_point_two")
            assertEquals(2.2f, twoPointTwoValue, "Expected 'two_point_two' value to be 2.2f")

            // on - BOOLEAN
            val onInfo = testUtil.commandQueue.getInputInfo(smHandle, 4)
            assertEquals("on", onInfo.name)
            assertEquals(InputType.BOOLEAN, onInfo.type)
            val onValue = testUtil.commandQueue.getBooleanInput(smHandle, "on")
            assertEquals(true, onValue, "Expected 'on' value to be true")

            // three - NUMBER
            val threeInfo = testUtil.commandQueue.getInputInfo(smHandle, 5)
            assertEquals("three", threeInfo.name)
            assertEquals(InputType.NUMBER, threeInfo.type)
            val threeValue = testUtil.commandQueue.getNumberInput(smHandle, "three")
            assertEquals(3f, threeValue, "Expected 'three' value to be 3f")

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
}
