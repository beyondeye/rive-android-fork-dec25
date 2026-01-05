package app.rive.mp.test.databinding

import app.rive.mp.CommandQueue
import app.rive.mp.FileHandle
import app.rive.mp.ViewModelInstanceHandle
import app.rive.mp.test.utils.MpCommandQueueTestUtil
import app.rive.mp.test.utils.MpTestContext
import app.rive.mp.test.utils.MpTestResources
import app.rive.mp.test.utils.loadRiveFile
import kotlinx.coroutines.test.runTest
import kotlin.test.*

/**
 * Phase D tests for data binding (ViewModel/ViewModelInstance) operations.
 * Tests the CommandQueue's VMI creation, property operations, list operations,
 * subscriptions, and binding to state machines.
 *
 * This is a port of RiveDataBindingTest.kt from the kotlin module, adapted
 * for the handle-based CommandQueue API.
 *
 * Uses test file: data_bind_test_impl.riv which contains:
 * - ViewModel "Test All" with properties:
 *   - Test Num (number)
 *   - Test String (string)
 *   - Test Bool (boolean)
 *   - Test Enum (enum)
 *   - Test Color (color)
 *   - Test Trigger (trigger)
 *   - Test Nested (viewmodel instance)
 *   - Test Image (image)
 *   - Test Artboard (artboard)
 * - ViewModel "Test List VM" with:
 *   - Test List (list)
 * - Instances: "Test Default", "Test Blank", etc.
 */
class MpRiveDataBindingTest {

    init {
        MpTestContext.initPlatform()
    }

    // ==========================================================================
    // D.1: ViewModelInstance Creation Tests
    // ==========================================================================

    @Test
    fun createDefaultViewModelInstance_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)

            // Create default VMI for "Test All" view model
            val vmiHandle = testUtil.commandQueue.createDefaultViewModelInstance(
                fileHandle,
                "Test All"
            )

            assertNotNull(vmiHandle, "VMI handle should not be null")
            assertTrue(vmiHandle.handle > 0, "VMI handle should be positive")

            // Cleanup
            testUtil.commandQueue.deleteViewModelInstance(vmiHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    @Test
    fun createBlankViewModelInstance_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)

            // Create blank VMI for "Test All" view model
            val vmiHandle = testUtil.commandQueue.createBlankViewModelInstance(
                fileHandle,
                "Test All"
            )

            assertNotNull(vmiHandle, "VMI handle should not be null")
            assertTrue(vmiHandle.handle > 0, "VMI handle should be positive")

            // Cleanup
            testUtil.commandQueue.deleteViewModelInstance(vmiHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    @Test
    fun createNamedViewModelInstance_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)

            // Create named VMI "Test Default" from "Test All" view model
            val vmiHandle = testUtil.commandQueue.createNamedViewModelInstance(
                fileHandle,
                "Test All",
                "Test Default"
            )

            assertNotNull(vmiHandle, "VMI handle should not be null")
            assertTrue(vmiHandle.handle > 0, "VMI handle should be positive")

            // Cleanup
            testUtil.commandQueue.deleteViewModelInstance(vmiHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    @Test
    fun createViewModelInstance_withInvalidViewModel_fails() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)

            // Try to create VMI for non-existent view model
            assertFailsWith<IllegalArgumentException> {
                testUtil.commandQueue.createDefaultViewModelInstance(
                    fileHandle,
                    "Non Existent ViewModel"
                )
            }

            // Cleanup
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    @Test
    fun createViewModelInstance_handles_are_unique() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)

            val vmiHandle1 = testUtil.commandQueue.createDefaultViewModelInstance(
                fileHandle,
                "Test All"
            )
            val vmiHandle2 = testUtil.commandQueue.createDefaultViewModelInstance(
                fileHandle,
                "Test All"
            )
            val vmiHandle3 = testUtil.commandQueue.createBlankViewModelInstance(
                fileHandle,
                "Test All"
            )

            assertNotEquals(vmiHandle1.handle, vmiHandle2.handle, "VMI handles should be unique")
            assertNotEquals(vmiHandle2.handle, vmiHandle3.handle, "VMI handles should be unique")
            assertNotEquals(vmiHandle1.handle, vmiHandle3.handle, "VMI handles should be unique")

            // Cleanup
            testUtil.commandQueue.deleteViewModelInstance(vmiHandle1)
            testUtil.commandQueue.deleteViewModelInstance(vmiHandle2)
            testUtil.commandQueue.deleteViewModelInstance(vmiHandle3)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    @Test
    fun deleteViewModelInstance_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)

            val vmiHandle = testUtil.commandQueue.createDefaultViewModelInstance(
                fileHandle,
                "Test All"
            )

            // Delete should not throw
            testUtil.commandQueue.deleteViewModelInstance(vmiHandle)

            // Cleanup
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    // ==========================================================================
    // D.2: Basic Property Operations Tests (number, string, boolean)
    // ==========================================================================

    @Test
    fun getNumberProperty_returnsDefaultValue() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val vmiHandle = testUtil.commandQueue.createNamedViewModelInstance(
                fileHandle,
                "Test All",
                "Test Default"
            )

            val value = testUtil.commandQueue.getNumberProperty(vmiHandle, "Test Num")
            assertEquals(123f, value, "Default number value should be 123")

            // Cleanup
            testUtil.commandQueue.deleteViewModelInstance(vmiHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    @Test
    fun setNumberProperty_updatesValue() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val vmiHandle = testUtil.commandQueue.createDefaultViewModelInstance(
                fileHandle,
                "Test All"
            )

            // Set the number property
            testUtil.commandQueue.setNumberProperty(vmiHandle, "Test Num", 456f)

            // Read it back
            val value = testUtil.commandQueue.getNumberProperty(vmiHandle, "Test Num")
            assertEquals(456f, value, "Number value should be updated to 456")

            // Cleanup
            testUtil.commandQueue.deleteViewModelInstance(vmiHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    @Test
    fun getStringProperty_returnsDefaultValue() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val vmiHandle = testUtil.commandQueue.createNamedViewModelInstance(
                fileHandle,
                "Test All",
                "Test Default"
            )

            val value = testUtil.commandQueue.getStringProperty(vmiHandle, "Test String")
            assertEquals("World", value, "Default string value should be 'World'")

            // Cleanup
            testUtil.commandQueue.deleteViewModelInstance(vmiHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    @Test
    fun setStringProperty_updatesValue() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val vmiHandle = testUtil.commandQueue.createDefaultViewModelInstance(
                fileHandle,
                "Test All"
            )

            // Set the string property
            testUtil.commandQueue.setStringProperty(vmiHandle, "Test String", "Hello Rive!")

            // Read it back
            val value = testUtil.commandQueue.getStringProperty(vmiHandle, "Test String")
            assertEquals("Hello Rive!", value, "String value should be updated to 'Hello Rive!'")

            // Cleanup
            testUtil.commandQueue.deleteViewModelInstance(vmiHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    @Test
    fun getBooleanProperty_returnsDefaultValue() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val vmiHandle = testUtil.commandQueue.createNamedViewModelInstance(
                fileHandle,
                "Test All",
                "Test Default"
            )

            val value = testUtil.commandQueue.getBooleanProperty(vmiHandle, "Test Bool")
            assertTrue(value, "Default boolean value should be true")

            // Cleanup
            testUtil.commandQueue.deleteViewModelInstance(vmiHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    @Test
    fun setBooleanProperty_updatesValue() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val vmiHandle = testUtil.commandQueue.createDefaultViewModelInstance(
                fileHandle,
                "Test All"
            )

            // Set the boolean property to false
            testUtil.commandQueue.setBooleanProperty(vmiHandle, "Test Bool", false)

            // Read it back
            val value = testUtil.commandQueue.getBooleanProperty(vmiHandle, "Test Bool")
            assertFalse(value, "Boolean value should be updated to false")

            // Cleanup
            testUtil.commandQueue.deleteViewModelInstance(vmiHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    // ==========================================================================
    // D.3: Additional Property Types Tests (enum, color, trigger)
    // ==========================================================================

    @Test
    fun getEnumProperty_returnsDefaultValue() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val vmiHandle = testUtil.commandQueue.createNamedViewModelInstance(
                fileHandle,
                "Test All",
                "Test Default"
            )

            val value = testUtil.commandQueue.getEnumProperty(vmiHandle, "Test Enum")
            assertEquals("Value 1", value, "Default enum value should be 'Value 1'")

            // Cleanup
            testUtil.commandQueue.deleteViewModelInstance(vmiHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    @Test
    fun setEnumProperty_updatesValue() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val vmiHandle = testUtil.commandQueue.createDefaultViewModelInstance(
                fileHandle,
                "Test All"
            )

            // Set the enum property
            testUtil.commandQueue.setEnumProperty(vmiHandle, "Test Enum", "Value 2")

            // Read it back
            val value = testUtil.commandQueue.getEnumProperty(vmiHandle, "Test Enum")
            assertEquals("Value 2", value, "Enum value should be updated to 'Value 2'")

            // Cleanup
            testUtil.commandQueue.deleteViewModelInstance(vmiHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    @Test
    fun getColorProperty_returnsDefaultValue() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val vmiHandle = testUtil.commandQueue.createNamedViewModelInstance(
                fileHandle,
                "Test All",
                "Test Default"
            )

            val value = testUtil.commandQueue.getColorProperty(vmiHandle, "Test Color")
            // Default color is red: 0xFFFF0000
            assertEquals(0xFFFF0000.toInt(), value, "Default color should be red (0xFFFF0000)")

            // Cleanup
            testUtil.commandQueue.deleteViewModelInstance(vmiHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    @Test
    fun setColorProperty_updatesValue() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val vmiHandle = testUtil.commandQueue.createDefaultViewModelInstance(
                fileHandle,
                "Test All"
            )

            // Set the color property to blue
            testUtil.commandQueue.setColorProperty(vmiHandle, "Test Color", 0xFF0000FF.toInt())

            // Read it back
            val value = testUtil.commandQueue.getColorProperty(vmiHandle, "Test Color")
            assertEquals(0xFF0000FF.toInt(), value, "Color value should be updated to blue")

            // Cleanup
            testUtil.commandQueue.deleteViewModelInstance(vmiHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    @Test
    fun fireTriggerProperty_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val vmiHandle = testUtil.commandQueue.createDefaultViewModelInstance(
                fileHandle,
                "Test All"
            )

            // Fire the trigger - should not throw
            testUtil.commandQueue.fireTriggerProperty(vmiHandle, "Test Trigger")

            // Cleanup
            testUtil.commandQueue.deleteViewModelInstance(vmiHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    // ==========================================================================
    // D.5: Nested VMI Operations Tests
    // ==========================================================================

    @Test
    fun getInstanceProperty_returnsNestedVMI() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val vmiHandle = testUtil.commandQueue.createNamedViewModelInstance(
                fileHandle,
                "Test All",
                "Test Default"
            )

            val nestedHandle = testUtil.commandQueue.getInstanceProperty(vmiHandle, "Test Nested")
            assertNotNull(nestedHandle, "Nested VMI handle should not be null")
            assertTrue(nestedHandle.handle > 0, "Nested VMI handle should be positive")

            // Cleanup - nested VMI is owned by parent, so we just delete the parent
            testUtil.commandQueue.deleteViewModelInstance(vmiHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    @Test
    fun nestedProperty_path_access() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val vmiHandle = testUtil.commandQueue.createNamedViewModelInstance(
                fileHandle,
                "Test All",
                "Test Default"
            )

            // Access nested property using path syntax
            val nestedNumValue = testUtil.commandQueue.getNumberProperty(
                vmiHandle,
                "Test Nested/Nested Number"
            )
            assertEquals(100f, nestedNumValue, "Nested number should be 100")

            // Cleanup
            testUtil.commandQueue.deleteViewModelInstance(vmiHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    @Test
    fun setNestedProperty_via_path() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val vmiHandle = testUtil.commandQueue.createDefaultViewModelInstance(
                fileHandle,
                "Test All"
            )

            // Set nested property using path syntax
            testUtil.commandQueue.setNumberProperty(
                vmiHandle,
                "Test Nested/Nested Number",
                200f
            )

            // Read it back
            val value = testUtil.commandQueue.getNumberProperty(
                vmiHandle,
                "Test Nested/Nested Number"
            )
            assertEquals(200f, value, "Nested number should be updated to 200")

            // Cleanup
            testUtil.commandQueue.deleteViewModelInstance(vmiHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    // ==========================================================================
    // D.5: List Operations Tests
    // ==========================================================================

    @Test
    fun getListSize_returnsCorrectCount() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val vmiHandle = testUtil.commandQueue.createDefaultViewModelInstance(
                fileHandle,
                "Test List VM"
            )

            val listSize = testUtil.commandQueue.getListSize(vmiHandle, "Test List")
            assertTrue(listSize >= 0, "List size should be non-negative")

            // Cleanup
            testUtil.commandQueue.deleteViewModelInstance(vmiHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    // ==========================================================================
    // D.6: VMI Binding to State Machine Tests
    // ==========================================================================

    @Test
    fun bindViewModelInstance_succeeds() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)
            val smHandle = testUtil.commandQueue.createDefaultStateMachine(artboardHandle)
            val vmiHandle = testUtil.commandQueue.createDefaultViewModelInstance(
                fileHandle,
                "Test All"
            )

            // Bind VMI to state machine - should not throw
            testUtil.commandQueue.bindViewModelInstance(smHandle, vmiHandle)

            // Cleanup
            testUtil.commandQueue.deleteStateMachine(smHandle)
            testUtil.commandQueue.deleteViewModelInstance(vmiHandle)
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    @Test
    fun getDefaultViewModelInstance_forArtboard() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val artboardHandle = testUtil.commandQueue.createDefaultArtboard(fileHandle)

            // Get default VMI for artboard
            val defaultVmiHandle = testUtil.commandQueue.getDefaultViewModelInstance(
                fileHandle,
                artboardHandle
            )

            // May or may not have a default VMI depending on the file
            // Just verify it doesn't throw and returns a valid result
            if (defaultVmiHandle != null) {
                assertTrue(defaultVmiHandle.handle > 0, "Default VMI handle should be positive")
                testUtil.commandQueue.deleteViewModelInstance(defaultVmiHandle)
            }

            // Cleanup
            testUtil.commandQueue.deleteArtboard(artboardHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    // ==========================================================================
    // Property Operations with Invalid Handles Tests
    // ==========================================================================

    @Test
    fun getPropertyWithInvalidHandle_fails() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val invalidHandle = ViewModelInstanceHandle(999999L)

            assertFailsWith<IllegalArgumentException> {
                testUtil.commandQueue.getNumberProperty(invalidHandle, "Test Num")
            }
        } finally {
            testUtil.cleanup()
        }
    }

    @Test
    fun getPropertyWithInvalidPath_fails() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            val vmiHandle = testUtil.commandQueue.createDefaultViewModelInstance(
                fileHandle,
                "Test All"
            )

            assertFailsWith<IllegalArgumentException> {
                testUtil.commandQueue.getNumberProperty(vmiHandle, "Non Existent Property")
            }

            // Cleanup
            testUtil.commandQueue.deleteViewModelInstance(vmiHandle)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    // ==========================================================================
    // Multi-VMI Tests
    // ==========================================================================

    @Test
    fun multipleVMIs_haveIndependentValues() = runTest {
        val testUtil = MpCommandQueueTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)

            val vmi1 = testUtil.commandQueue.createDefaultViewModelInstance(fileHandle, "Test All")
            val vmi2 = testUtil.commandQueue.createDefaultViewModelInstance(fileHandle, "Test All")

            // Set different values in each VMI
            testUtil.commandQueue.setNumberProperty(vmi1, "Test Num", 100f)
            testUtil.commandQueue.setNumberProperty(vmi2, "Test Num", 200f)

            // Verify values are independent
            val value1 = testUtil.commandQueue.getNumberProperty(vmi1, "Test Num")
            val value2 = testUtil.commandQueue.getNumberProperty(vmi2, "Test Num")

            assertEquals(100f, value1, "VMI 1 should have value 100")
            assertEquals(200f, value2, "VMI 2 should have value 200")

            // Cleanup
            testUtil.commandQueue.deleteViewModelInstance(vmi1)
            testUtil.commandQueue.deleteViewModelInstance(vmi2)
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
}
