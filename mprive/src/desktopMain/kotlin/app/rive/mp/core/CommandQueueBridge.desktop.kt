package app.rive.mp.core

import app.rive.mp.CommandQueue
import kotlinx.atomicfu.atomic

/**
 * Desktop stub implementation of CommandQueueBridge.
 * 
 * This is a minimal stub for development and testing purposes.
 * It returns incrementing handles and simulates basic behavior.
 * 
 * NOTE: This is NOT a real implementation - Desktop support is deferred (Phase F).
 * For real rendering, use Android with JNI bindings.
 */
class DesktopCommandQueueBridge : CommandQueueBridge {
    
    // Counters for generating unique handles
    private val nextCommandQueueHandle = atomic(1000L)
    private val nextFileHandle = atomic(1L)
    private val nextArtboardHandle = atomic(1L)
    private val nextStateMachineHandle = atomic(1L)
    private val nextVmiHandle = atomic(1L)
    private val nextRenderTargetHandle = atomic(1L)
    private val nextDrawKey = atomic(1L)
    
    // Track valid handles
    private val validFileHandles = mutableSetOf<Long>()
    private val validArtboardHandles = mutableSetOf<Long>()
    private val validStateMachineHandles = mutableSetOf<Long>()
    private val validVmiHandles = mutableSetOf<Long>()
    
    // File metadata (fileHandle -> list of artboard names)
    private val fileArtboards = mutableMapOf<Long, List<String>>()
    private val fileViewModels = mutableMapOf<Long, List<String>>()
    
    // Artboard metadata (artboardHandle -> list of state machine names)
    private val artboardStateMachines = mutableMapOf<Long, List<String>>()
    
    override fun cppConstructor(renderContextPointer: Long): Long {
        return nextCommandQueueHandle.getAndIncrement()
    }
    
    override fun cppDelete(pointer: Long) {
        // No-op for stub
    }
    
    override fun cppCreateListeners(pointer: Long, receiver: CommandQueue): Listeners {
        return Listeners(null)
    }
    
    override fun cppPollMessages(pointer: Long, receiver: CommandQueue) {
        // No-op - desktop stub doesn't have async messages
    }
    
    // =========================================================================
    // File Operations
    // =========================================================================
    
    override fun cppLoadFile(pointer: Long, requestID: Long, bytes: ByteArray) {
        val fileHandle = nextFileHandle.getAndIncrement()
        validFileHandles.add(fileHandle)
        
        // Default test data - simulate multipleartboards.riv having 2 artboards
        fileArtboards[fileHandle] = listOf("artboard2", "artboard1")
        fileViewModels[fileHandle] = listOf()
        
        // TODO: In a real stub, we'd parse the file to get actual artboard names
    }
    
    override fun cppDeleteFile(pointer: Long, requestID: Long, fileHandle: Long) {
        validFileHandles.remove(fileHandle)
        fileArtboards.remove(fileHandle)
        fileViewModels.remove(fileHandle)
    }
    
    override fun cppGetArtboardNames(pointer: Long, requestID: Long, fileHandle: Long) {
        if (!validFileHandles.contains(fileHandle)) {
            throw IllegalArgumentException("Invalid file handle: $fileHandle")
        }
        // The callback would normally return artboard names
        // For stub, we need the CommandQueue to call onArtboardNamesListed
    }
    
    override fun cppGetStateMachineNames(pointer: Long, requestID: Long, artboardHandle: Long) {
        if (!validArtboardHandles.contains(artboardHandle)) {
            throw IllegalArgumentException("Invalid artboard handle: $artboardHandle")
        }
    }
    
    override fun cppGetViewModelNames(pointer: Long, requestID: Long, fileHandle: Long) {
        if (!validFileHandles.contains(fileHandle)) {
            throw IllegalArgumentException("Invalid file handle: $fileHandle")
        }
    }
    
    // =========================================================================
    // Artboard Operations
    // =========================================================================
    
    override fun cppCreateDefaultArtboard(pointer: Long, requestID: Long, fileHandle: Long): Long {
        if (!validFileHandles.contains(fileHandle)) {
            return 0L
        }
        val handle = nextArtboardHandle.getAndIncrement()
        validArtboardHandles.add(handle)
        artboardStateMachines[handle] = listOf("State Machine 1")
        return handle
    }
    
    override fun cppCreateArtboardByName(pointer: Long, requestID: Long, fileHandle: Long, name: String): Long {
        if (!validFileHandles.contains(fileHandle)) {
            return 0L
        }
        val artboardNames = fileArtboards[fileHandle] ?: return 0L
        if (!artboardNames.contains(name)) {
            return 0L
        }
        val handle = nextArtboardHandle.getAndIncrement()
        validArtboardHandles.add(handle)
        artboardStateMachines[handle] = listOf("State Machine 1")
        return handle
    }
    
    override fun cppDeleteArtboard(pointer: Long, requestID: Long, artboardHandle: Long) {
        validArtboardHandles.remove(artboardHandle)
        artboardStateMachines.remove(artboardHandle)
    }
    
    override fun cppResizeArtboard(pointer: Long, artboardHandle: Long, width: Int, height: Int, scaleFactor: Float) {
        // No-op for stub
    }
    
    override fun cppResetArtboardSize(pointer: Long, artboardHandle: Long) {
        // No-op for stub
    }
    
    // =========================================================================
    // State Machine Operations
    // =========================================================================
    
    override fun cppCreateDefaultStateMachine(pointer: Long, requestID: Long, artboardHandle: Long): Long {
        if (!validArtboardHandles.contains(artboardHandle)) {
            return 0L
        }
        val handle = nextStateMachineHandle.getAndIncrement()
        validStateMachineHandles.add(handle)
        return handle
    }
    
    override fun cppCreateStateMachineByName(pointer: Long, requestID: Long, artboardHandle: Long, name: String): Long {
        if (!validArtboardHandles.contains(artboardHandle)) {
            return 0L
        }
        val handle = nextStateMachineHandle.getAndIncrement()
        validStateMachineHandles.add(handle)
        return handle
    }
    
    override fun cppDeleteStateMachine(pointer: Long, requestID: Long, stateMachineHandle: Long) {
        validStateMachineHandles.remove(stateMachineHandle)
    }
    
    override fun cppAdvanceStateMachine(pointer: Long, stateMachineHandle: Long, deltaTimeNs: Long) {
        // No-op for stub
    }
    
    // =========================================================================
    // State Machine Input Manipulation
    // =========================================================================
    
    override fun cppSetStateMachineNumberInput(pointer: Long, stateMachineHandle: Long, inputName: String, value: Float) {
        // No-op for stub
    }
    
    override fun cppSetStateMachineBooleanInput(pointer: Long, stateMachineHandle: Long, inputName: String, value: Boolean) {
        // No-op for stub
    }
    
    override fun cppFireStateMachineTrigger(pointer: Long, stateMachineHandle: Long, inputName: String) {
        // No-op for stub
    }
    
    // =========================================================================
    // State Machine Input Query
    // =========================================================================
    
    override fun cppGetInputCount(pointer: Long, requestID: Long, smHandle: Long) {
        // No-op for stub
    }
    
    override fun cppGetInputNames(pointer: Long, requestID: Long, smHandle: Long) {
        // No-op for stub
    }
    
    override fun cppGetInputInfo(pointer: Long, requestID: Long, smHandle: Long, inputIndex: Int) {
        // No-op for stub
    }
    
    override fun cppGetNumberInput(pointer: Long, requestID: Long, smHandle: Long, inputName: String) {
        // No-op for stub
    }
    
    override fun cppSetNumberInput(pointer: Long, requestID: Long, smHandle: Long, inputName: String, value: Float) {
        // No-op for stub
    }
    
    override fun cppGetBooleanInput(pointer: Long, requestID: Long, smHandle: Long, inputName: String) {
        // No-op for stub
    }
    
    override fun cppSetBooleanInput(pointer: Long, requestID: Long, smHandle: Long, inputName: String, value: Boolean) {
        // No-op for stub
    }
    
    override fun cppFireTrigger(pointer: Long, requestID: Long, smHandle: Long, inputName: String) {
        // No-op for stub
    }
    
    // =========================================================================
    // ViewModelInstance Operations
    // =========================================================================
    
    override fun cppCreateBlankVMI(pointer: Long, requestID: Long, fileHandle: Long, viewModelName: String): Long {
        if (!validFileHandles.contains(fileHandle)) {
            return 0L
        }
        val handle = nextVmiHandle.getAndIncrement()
        validVmiHandles.add(handle)
        return handle
    }
    
    override fun cppCreateDefaultVMI(pointer: Long, requestID: Long, fileHandle: Long, viewModelName: String): Long {
        if (!validFileHandles.contains(fileHandle)) {
            return 0L
        }
        val handle = nextVmiHandle.getAndIncrement()
        validVmiHandles.add(handle)
        return handle
    }
    
    override fun cppCreateNamedVMI(pointer: Long, requestID: Long, fileHandle: Long, viewModelName: String, instanceName: String): Long {
        if (!validFileHandles.contains(fileHandle)) {
            return 0L
        }
        val handle = nextVmiHandle.getAndIncrement()
        validVmiHandles.add(handle)
        return handle
    }
    
    override fun cppDeleteVMI(pointer: Long, requestID: Long, vmiHandle: Long) {
        validVmiHandles.remove(vmiHandle)
    }
    
    override fun cppBindViewModelInstance(pointer: Long, requestID: Long, smHandle: Long, vmiHandle: Long) {
        // No-op for stub
    }
    
    override fun cppGetDefaultViewModelInstance(pointer: Long, requestID: Long, fileHandle: Long, artboardHandle: Long) {
        // No-op for stub
    }
    
    // =========================================================================
    // Property Operations
    // =========================================================================
    
    override fun cppGetNumberProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String) {}
    override fun cppSetNumberProperty(pointer: Long, vmiHandle: Long, propertyPath: String, value: Float) {}
    override fun cppGetStringProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String) {}
    override fun cppSetStringProperty(pointer: Long, vmiHandle: Long, propertyPath: String, value: String) {}
    override fun cppGetBooleanProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String) {}
    override fun cppSetBooleanProperty(pointer: Long, vmiHandle: Long, propertyPath: String, value: Boolean) {}
    override fun cppGetEnumProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String) {}
    override fun cppSetEnumProperty(pointer: Long, vmiHandle: Long, propertyPath: String, value: String) {}
    override fun cppGetColorProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String) {}
    override fun cppSetColorProperty(pointer: Long, vmiHandle: Long, propertyPath: String, value: Int) {}
    override fun cppFireTriggerProperty(pointer: Long, vmiHandle: Long, propertyPath: String) {}
    
    // =========================================================================
    // Property Subscriptions
    // =========================================================================
    
    override fun cppSubscribeToProperty(pointer: Long, vmiHandle: Long, propertyPath: String, propertyType: Int) {}
    override fun cppUnsubscribeFromProperty(pointer: Long, vmiHandle: Long, propertyPath: String, propertyType: Int) {}
    
    // =========================================================================
    // List Operations
    // =========================================================================
    
    override fun cppGetListSize(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String) {}
    override fun cppGetListItem(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, index: Int) {}
    override fun cppAddListItem(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, itemHandle: Long) {}
    override fun cppAddListItemAt(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, index: Int, itemHandle: Long) {}
    override fun cppRemoveListItem(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, itemHandle: Long) {}
    override fun cppRemoveListItemAt(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, index: Int) {}
    override fun cppSwapListItems(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, indexA: Int, indexB: Int) {}
    
    // =========================================================================
    // Nested VMI Operations
    // =========================================================================
    
    override fun cppGetInstanceProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String) {}
    override fun cppSetInstanceProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, nestedHandle: Long) {}
    
    // =========================================================================
    // Asset Property Operations
    // =========================================================================
    
    override fun cppSetImageProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, imageHandle: Long) {}
    override fun cppSetArtboardProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, fileHandle: Long, artboardHandle: Long) {}
    
    // =========================================================================
    // Pointer Events
    // =========================================================================
    
    override fun cppPointerMove(pointer: Long, stateMachineHandle: Long, fit: Byte, alignment: Byte, layoutScale: Float, surfaceWidth: Float, surfaceHeight: Float, pointerID: Int, x: Float, y: Float) {}
    override fun cppPointerDown(pointer: Long, stateMachineHandle: Long, fit: Byte, alignment: Byte, layoutScale: Float, surfaceWidth: Float, surfaceHeight: Float, pointerID: Int, x: Float, y: Float) {}
    override fun cppPointerUp(pointer: Long, stateMachineHandle: Long, fit: Byte, alignment: Byte, layoutScale: Float, surfaceWidth: Float, surfaceHeight: Float, pointerID: Int, x: Float, y: Float) {}
    override fun cppPointerExit(pointer: Long, stateMachineHandle: Long, fit: Byte, alignment: Byte, layoutScale: Float, surfaceWidth: Float, surfaceHeight: Float, pointerID: Int, x: Float, y: Float) {}
    
    // =========================================================================
    // Render Target Operations
    // =========================================================================
    
    override fun cppCreateRenderTarget(pointer: Long, requestID: Long, width: Int, height: Int, sampleCount: Int) {}
    
    override fun cppDeleteRenderTarget(pointer: Long, requestID: Long, renderTargetHandle: Long) {}
    
    override fun cppCreateRiveRenderTarget(pointer: Long, width: Int, height: Int): Long {
        return nextRenderTargetHandle.getAndIncrement()
    }
    
    override fun cppCreateDrawKey(pointer: Long): Long {
        return nextDrawKey.getAndIncrement()
    }
    
    // =========================================================================
    // Drawing Operations
    // =========================================================================
    
    override fun cppDraw(pointer: Long, requestID: Long, artboardHandle: Long, smHandle: Long, surfacePtr: Long, renderTargetPtr: Long, drawKey: Long, surfaceWidth: Int, surfaceHeight: Int, fitMode: Int, alignmentMode: Int, clearColor: Int, scaleFactor: Float) {}
    
    override fun cppDrawToBuffer(pointer: Long, renderContextPointer: Long, surfaceNativePointer: Long, drawKey: Long, artboardHandle: Long, stateMachineHandle: Long, renderTargetPointer: Long, width: Int, height: Int, fit: Byte, alignment: Byte, scaleFactor: Float, clearColor: Int, buffer: ByteArray) {}
    
    override fun cppRunOnCommandServer(pointer: Long, work: () -> Unit) {
        work() // Run synchronously on desktop stub
    }
    
    // =========================================================================
    // Batch Sprite Rendering
    // =========================================================================
    
    override fun cppDrawMultiple(pointer: Long, renderContextPointer: Long, surfaceNativePointer: Long, drawKey: Long, renderTargetPointer: Long, viewportWidth: Int, viewportHeight: Int, clearColor: Int, artboardHandles: LongArray, stateMachineHandles: LongArray, transforms: FloatArray, artboardWidths: FloatArray, artboardHeights: FloatArray, count: Int) {}
    
    override fun cppDrawMultipleToBuffer(pointer: Long, renderContextPointer: Long, surfaceNativePointer: Long, drawKey: Long, renderTargetPointer: Long, viewportWidth: Int, viewportHeight: Int, clearColor: Int, artboardHandles: LongArray, stateMachineHandles: LongArray, transforms: FloatArray, artboardWidths: FloatArray, artboardHeights: FloatArray, count: Int, buffer: ByteArray) {}
}

/**
 * Create the desktop implementation of CommandQueueBridge.
 */
actual fun createCommandQueueBridge(): CommandQueueBridge = DesktopCommandQueueBridge()