package app.rive.mp.test.file

import app.rive.mp.PropertyDataType
import app.rive.mp.test.rendering.AndroidRenderTestUtil
import app.rive.mp.test.utils.MpTestContext
import app.rive.mp.test.utils.MpTestResources
import app.rive.mp.test.utils.loadRiveFile
import kotlinx.coroutines.test.runTest
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue

/**
 * Tests for Phase E.2 File Introspection APIs.
 * 
 * These tests verify the following CommandQueue methods:
 * - [getViewModelInstanceNames] - Get named instances of a ViewModel
 * - [getViewModelProperties] - Get property definitions of a ViewModel
 * - [getEnums] - Get enum definitions in a file
 * 
 * ## Test Files Used
 * - `data_bind_test_impl.riv` - Contains ViewModels with various property types:
 *   - "Test All" ViewModel with: NUMBER, STRING, BOOLEAN, ENUM, COLOR, TRIGGER, VIEW_MODEL, ASSET_IMAGE, ARTBOARD
 *   - "Test List VM" ViewModel with: LIST property
 *   - "Empty VM" ViewModel with no properties
 *   - "Nested VM" ViewModel
 *   - Named instances: "Test Default", "Alternate Nested"
 */
class MpRiveFileIntrospectionTest {

    init {
        MpTestContext.initPlatform()
    }

    // =========================================================================
    // getViewModelInstanceNames Tests
    // =========================================================================

    /**
     * Test that getViewModelInstanceNames returns instance names for a ViewModel.
     * Uses "Test All" ViewModel which has at least one named instance "Test Default".
     */
    @Test
    fun getViewModelInstanceNames_returnsInstanceNames() = runTest {
        val testUtil = AndroidRenderTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            // Get instance names for "Test All" ViewModel
            val instanceNames = testUtil.commandQueue.getViewModelInstanceNames(
                fileHandle,
                "Test All"
            )
            
            // Should contain at least "Test Default" instance
            assertTrue(
                instanceNames.contains("Test Default"),
                "Expected 'Test Default' instance, got: $instanceNames"
            )
            
            // Cleanup
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test that getViewModelInstanceNames returns empty list for ViewModel with no named instances.
     */
    @Test
    fun getViewModelInstanceNames_emptyForViewModelWithNoInstances() = runTest {
        val testUtil = AndroidRenderTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            // "Empty VM" should have no named instances (only blank/default can be created)
            val instanceNames = testUtil.commandQueue.getViewModelInstanceNames(
                fileHandle,
                "Empty VM"
            )
            
            // Empty VM should have 0 instances (based on kotlin module test: assertEquals(0, vm.instanceCount))
            assertEquals(
                0,
                instanceNames.size,
                "Expected Empty VM to have no named instances, got: $instanceNames"
            )
            
            // Cleanup
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test that getViewModelInstanceNames throws for non-existent ViewModel.
     */
    @Test
    fun getViewModelInstanceNames_throwsForUnknownViewModel() = runTest {
        val testUtil = AndroidRenderTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            // Should throw for non-existent ViewModel
            assertFailsWith<IllegalArgumentException> {
                testUtil.commandQueue.getViewModelInstanceNames(
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

    // =========================================================================
    // getViewModelProperties Tests
    // =========================================================================

    /**
     * Test that getViewModelProperties returns property definitions for a ViewModel.
     * "Test All" ViewModel should have properties of various types.
     */
    @Test
    fun getViewModelProperties_returnsPropertyDefinitions() = runTest {
        val testUtil = AndroidRenderTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            // Get properties for "Test All" ViewModel
            val properties = testUtil.commandQueue.getViewModelProperties(
                fileHandle,
                "Test All"
            )
            
            // Should have multiple properties
            assertTrue(
                properties.isNotEmpty(),
                "Expected 'Test All' to have properties"
            )
            
            // Map properties by name for easier verification
            val propMap = properties.associateBy { it.name }
            
            // Verify expected property types (based on kotlin module test expectations)
            assertTrue(propMap.containsKey("Test Num"), "Missing 'Test Num' property")
            assertTrue(propMap.containsKey("Test String"), "Missing 'Test String' property")
            assertTrue(propMap.containsKey("Test Bool"), "Missing 'Test Bool' property")
            assertTrue(propMap.containsKey("Test Enum"), "Missing 'Test Enum' property")
            assertTrue(propMap.containsKey("Test Color"), "Missing 'Test Color' property")
            assertTrue(propMap.containsKey("Test Trigger"), "Missing 'Test Trigger' property")
            assertTrue(propMap.containsKey("Test Nested"), "Missing 'Test Nested' property")
            
            // Verify property types
            assertEquals(PropertyDataType.NUMBER, propMap["Test Num"]?.type, "Test Num should be NUMBER")
            assertEquals(PropertyDataType.STRING, propMap["Test String"]?.type, "Test String should be STRING")
            assertEquals(PropertyDataType.BOOLEAN, propMap["Test Bool"]?.type, "Test Bool should be BOOLEAN")
            assertEquals(PropertyDataType.ENUM, propMap["Test Enum"]?.type, "Test Enum should be ENUM")
            assertEquals(PropertyDataType.COLOR, propMap["Test Color"]?.type, "Test Color should be COLOR")
            assertEquals(PropertyDataType.TRIGGER, propMap["Test Trigger"]?.type, "Test Trigger should be TRIGGER")
            assertEquals(PropertyDataType.VIEW_MODEL, propMap["Test Nested"]?.type, "Test Nested should be VIEW_MODEL")
            
            // Cleanup
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test that getViewModelProperties returns empty list for ViewModel with no properties.
     */
    @Test
    fun getViewModelProperties_emptyForViewModelWithNoProperties() = runTest {
        val testUtil = AndroidRenderTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            // "Empty VM" should have no properties
            val properties = testUtil.commandQueue.getViewModelProperties(
                fileHandle,
                "Empty VM"
            )
            
            assertEquals(
                0,
                properties.size,
                "Expected 'Empty VM' to have no properties, got: $properties"
            )
            
            // Cleanup
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test that getViewModelProperties includes LIST type for list ViewModels.
     */
    @Test
    fun getViewModelProperties_includesListType() = runTest {
        val testUtil = AndroidRenderTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            // "Test List VM" should have a LIST property
            val properties = testUtil.commandQueue.getViewModelProperties(
                fileHandle,
                "Test List VM"
            )
            
            val listProp = properties.find { it.name == "Test List" }
            assertTrue(listProp != null, "Expected 'Test List' property")
            assertEquals(PropertyDataType.LIST, listProp?.type, "Test List should be LIST type")
            
            // Cleanup
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test that getViewModelProperties throws for non-existent ViewModel.
     */
    @Test
    fun getViewModelProperties_throwsForUnknownViewModel() = runTest {
        val testUtil = AndroidRenderTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            // Should throw for non-existent ViewModel
            assertFailsWith<IllegalArgumentException> {
                testUtil.commandQueue.getViewModelProperties(
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

    // =========================================================================
    // getEnums Tests
    // =========================================================================

    /**
     * Test that getEnums returns enum definitions from a file.
     * data_bind_test_impl.riv should have at least one enum for "Test Enum" property.
     */
    @Test
    fun getEnums_returnsEnumDefinitions() = runTest {
        val testUtil = AndroidRenderTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            // Get all enums from the file
            val enums = testUtil.commandQueue.getEnums(fileHandle)
            
            // Should have at least one enum (used by "Test Enum" property)
            assertTrue(
                enums.isNotEmpty(),
                "Expected file to have at least one enum definition"
            )
            
            // Each enum should have a name and non-empty values list
            for (riveEnum in enums) {
                assertTrue(riveEnum.name.isNotBlank(), "Enum name should not be blank")
                assertTrue(riveEnum.values.isNotEmpty(), "Enum '${riveEnum.name}' should have values")
            }
            
            // Cleanup
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test that getEnums returns empty list for a file with no enums.
     * Using shapes.riv which is a simple file without enums.
     */
    @Test
    fun getEnums_emptyForFileWithNoEnums() = runTest {
        val testUtil = AndroidRenderTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("shapes.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            // shapes.riv is a simple file that shouldn't have enums
            val enums = testUtil.commandQueue.getEnums(fileHandle)
            
            assertEquals(
                0,
                enums.size,
                "Expected shapes.riv to have no enums, got: ${enums.map { it.name }}"
            )
            
            // Cleanup
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }

    /**
     * Test that enum values list is correctly populated.
     */
    @Test
    fun getEnums_enumValuesPopulated() = runTest {
        val testUtil = AndroidRenderTestUtil(this)
        try {
            val bytes = MpTestResources.loadRiveFile("data_bind_test_impl.riv")
            val fileHandle = testUtil.commandQueue.loadFile(bytes)
            
            val enums = testUtil.commandQueue.getEnums(fileHandle)
            
            // Find an enum and verify it has multiple values
            if (enums.isNotEmpty()) {
                val firstEnum = enums.first()
                assertTrue(
                    firstEnum.values.isNotEmpty(),
                    "Enum '${firstEnum.name}' should have at least one value"
                )
                
                // Verify values are not blank strings
                for (value in firstEnum.values) {
                    assertTrue(value.isNotBlank(), "Enum value should not be blank")
                }
            }
            
            // Cleanup
            testUtil.commandQueue.deleteFile(fileHandle)
        } finally {
            testUtil.cleanup()
        }
    }
}