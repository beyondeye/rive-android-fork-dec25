#ifndef RIVE_ANDROID_COMMAND_SERVER_HPP
#define RIVE_ANDROID_COMMAND_SERVER_HPP

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <string>
#include "jni_refs.hpp"
#include "rive_log.hpp"

// Rive headers
#include "rive/file.hpp"
#include "rive/animation/state_machine_instance.hpp"
#include "rive/viewmodel/runtime/viewmodel_runtime.hpp"
#include "rive/viewmodel/runtime/viewmodel_instance_runtime.hpp"

namespace rive_android {

using rive_mp::GlobalRef;

// Forward declarations
class RenderContext;

/**
 * Command types that can be sent to the CommandServer.
 */
enum class CommandType {
    None,
    Stop,
    // Phase B: File operations
    LoadFile,
    DeleteFile,
    GetArtboardNames,
    GetStateMachineNames,
    GetViewModelNames,
    // Phase B.3: Artboard operations
    CreateDefaultArtboard,
    CreateArtboardByName,
    DeleteArtboard,
    // Phase C: State machine operations
    CreateDefaultStateMachine,
    CreateStateMachineByName,
    AdvanceStateMachine,
    DeleteStateMachine,
    // Phase C.4: State machine input operations
    GetInputCount,
    GetInputNames,
    GetInputInfo,
    GetNumberInput,
    SetNumberInput,
    GetBooleanInput,
    SetBooleanInput,
    FireTrigger,
    // Phase D: View model operations
    CreateBlankVMI,           // Create blank instance from named ViewModel
    CreateDefaultVMI,         // Create default instance from named ViewModel
    CreateNamedVMI,           // Create named instance from named ViewModel
    DeleteVMI,                // Delete a ViewModelInstance
    // Phase D.2: Property operations
    GetNumberProperty,        // Get number property value
    SetNumberProperty,        // Set number property value
    GetStringProperty,        // Get string property value
    SetStringProperty,        // Set string property value
    GetBooleanProperty,       // Get boolean property value
    SetBooleanProperty,       // Set boolean property value
    // Phase D.3: Additional property types
    GetEnumProperty,          // Get enum property value (as string)
    SetEnumProperty,          // Set enum property value (by string)
    GetColorProperty,         // Get color property value (0xAARRGGBB)
    SetColorProperty,         // Set color property value
    FireTriggerProperty,      // Fire a trigger property
    // Phase D.4: Property subscriptions
    SubscribeToProperty,      // Subscribe to property updates
    UnsubscribeFromProperty,  // Unsubscribe from property updates
    // Phase D.5: List operations
    GetListSize,              // Get the size of a list property
    GetListItem,              // Get an item from a list by index
    AddListItem,              // Add an item to the end of a list
    AddListItemAt,            // Add an item at a specific index
    RemoveListItem,           // Remove an item from a list by handle
    RemoveListItemAt,         // Remove an item from a list by index
    SwapListItems,            // Swap two items in a list
    // Phase D.5: Nested VMI operations
    GetInstanceProperty,      // Get a nested VMI property
    SetInstanceProperty,      // Set a nested VMI property
    // Phase D.5: Asset property operations
    SetImageProperty,         // Set an image property
    SetArtboardProperty,      // Set an artboard property
};

/**
 * A command to be executed by the CommandServer.
 */
struct Command {
    CommandType type = CommandType::None;
    int64_t requestID = 0;
    
    // Command-specific data
    std::vector<uint8_t> bytes;  // For LoadFile
    int64_t handle = 0;          // For DeleteFile, etc.
    std::string name;            // For CreateArtboardByName, CreateStateMachineByName
    float deltaTime = 0.0f;      // For AdvanceStateMachine (in seconds)

    // Input operation data
    std::string inputName;       // For input operations by name
    int32_t inputIndex = -1;     // For input operations by index
    float floatValue = 0.0f;     // For SetNumberInput
    bool boolValue = false;      // For SetBooleanInput

    // View model operation data (Phase D)
    std::string viewModelName;   // For CreateBlankVMI, CreateDefaultVMI, CreateNamedVMI
    std::string instanceName;    // For CreateNamedVMI

    // Property operation data (Phase D.2)
    std::string propertyPath;    // For property get/set operations
    std::string stringValue;     // For SetStringProperty, SetEnumProperty

    // Property operation data (Phase D.3)
    int32_t colorValue = 0;      // For SetColorProperty (0xAARRGGBB format)

    // Property subscription data (Phase D.4)
    int32_t propertyType = 0;    // For SubscribeToProperty, UnsubscribeFromProperty (PropertyDataType)

    // List operation data (Phase D.5)
    int32_t listIndex = -1;      // For GetListItem, AddListItemAt, RemoveListItemAt
    int32_t listIndexB = -1;     // For SwapListItems (second index)
    int64_t itemHandle = 0;      // For AddListItem, AddListItemAt, RemoveListItem

    // Nested VMI operation data (Phase D.5)
    int64_t nestedHandle = 0;    // For SetInstanceProperty

    // Asset property operation data (Phase D.5)
    int64_t assetHandle = 0;     // For SetImageProperty, SetArtboardProperty
    int64_t fileHandle = 0;      // For SetArtboardProperty (needed to create BindableArtboard)

    Command() = default;
    explicit Command(CommandType t, int64_t reqID = 0) 
        : type(t), requestID(reqID) {}
};

/**
 * Message types that can be sent from CommandServer to Kotlin.
 */
enum class MessageType {
    None,
    // File operations
    FileLoaded,
    FileError,
    FileDeleted,
    // Query operations
    ArtboardNamesListed,
    StateMachineNamesListed,
    ViewModelNamesListed,
    QueryError,
    // Artboard operations
    ArtboardCreated,
    ArtboardError,
    ArtboardDeleted,
    // State machine operations
    StateMachineCreated,
    StateMachineError,
    StateMachineDeleted,
    StateMachineSettled,
    // State machine input operations
    InputCountResult,
    InputNamesListed,
    InputInfoResult,
    NumberInputValue,
    BooleanInputValue,
    InputOperationSuccess,
    InputOperationError,
    // Phase D: View model operations
    VMICreated,
    VMIError,
    VMIDeleted,
    // Phase D.2: Property operations
    NumberPropertyValue,      // Returns float value
    StringPropertyValue,      // Returns string value
    BooleanPropertyValue,     // Returns boolean value
    PropertyError,            // Error with property operation
    PropertySetSuccess,       // Set operation succeeded
    // Phase D.3: Additional property types
    EnumPropertyValue,        // Returns enum value (as string)
    ColorPropertyValue,       // Returns color value (0xAARRGGBB)
    TriggerFired,             // Trigger was successfully fired
    // Phase D.4: Property subscription updates
    NumberPropertyUpdated,    // Subscribed number property changed
    StringPropertyUpdated,    // Subscribed string property changed
    BooleanPropertyUpdated,   // Subscribed boolean property changed
    EnumPropertyUpdated,      // Subscribed enum property changed
    ColorPropertyUpdated,     // Subscribed color property changed
    TriggerPropertyFired,     // Subscribed trigger property fired
    // Phase D.5: List operation results
    ListSizeResult,           // List size query result
    ListItemResult,           // List item query result (returns VMI handle)
    ListOperationSuccess,     // List modification succeeded
    ListOperationError,       // List operation failed
    // Phase D.5: Nested VMI operation results
    InstancePropertyResult,   // Nested VMI query result (returns VMI handle)
    InstancePropertySetSuccess,// Nested VMI set succeeded
    InstancePropertyError,    // Nested VMI operation failed
    // Phase D.5: Asset property operation results
    AssetPropertySetSuccess,  // Asset property set succeeded
    AssetPropertyError,       // Asset property operation failed
};

/**
 * A message to be sent from CommandServer to Kotlin.
 */
/**
 * Input type enum for state machine inputs.
 */
enum class InputType {
    NUMBER = 0,
    BOOLEAN = 1,
    TRIGGER = 2,
    UNKNOWN = -1
};

/**
 * Property data type enum - mirrors rive::DataType values.
 */
enum class PropertyDataType {
    NONE = 0,
    STRING = 1,
    NUMBER = 2,
    BOOLEAN = 3,
    COLOR = 4,
    LIST = 5,
    ENUM = 6,
    TRIGGER = 7,
    VIEW_MODEL = 8,
    INTEGER = 9,
    SYMBOL_LIST_INDEX = 10,
    ASSET_IMAGE = 11,
    ARTBOARD = 12
};

/**
 * Represents a subscription to property changes.
 */
struct PropertySubscription {
    int64_t vmiHandle;
    std::string propertyPath;
    PropertyDataType propertyType;

    bool operator==(const PropertySubscription& other) const {
        return vmiHandle == other.vmiHandle &&
               propertyPath == other.propertyPath &&
               propertyType == other.propertyType;
    }
};

struct Message {
    MessageType type = MessageType::None;
    int64_t requestID = 0;
    int64_t handle = 0;
    std::string error;
    std::vector<std::string> stringList;  // For query results

    // Input operation results
    int32_t intValue = 0;        // For InputCountResult
    float floatValue = 0.0f;     // For NumberInputValue, NumberPropertyValue
    bool boolValue = false;      // For BooleanInputValue, BooleanPropertyValue
    InputType inputType = InputType::UNKNOWN;  // For InputInfoResult
    std::string inputName;       // For InputInfoResult

    // Property operation results (Phase D.2)
    std::string stringValue;     // For StringPropertyValue, EnumPropertyValue

    // Property operation results (Phase D.3)
    int32_t colorValue = 0;      // For ColorPropertyValue (0xAARRGGBB format)

    // Property subscription update data (Phase D.4)
    int64_t vmiHandle = 0;       // For property update messages
    std::string propertyPath;    // For property update messages

    Message() = default;
    explicit Message(MessageType t, int64_t reqID = 0)
        : type(t), requestID(reqID) {}
};

/**
 * The CommandServer manages a dedicated thread for all Rive operations.
 * It receives commands via a thread-safe queue and executes them sequentially.
 * 
 * For Phase A, this is a minimal implementation focused on thread lifecycle:
 * - Start/stop the worker thread
 * - Process a command queue
 * - Handle basic lifecycle
 * 
 * Phase B+ will add:
 * - Resource management (files, artboards, state machines)
 * - Handle generation
 * - JNI callbacks
 */
class CommandServer {
public:
    /**
     * Constructs a CommandServer and starts the worker thread.
     * 
     * @param env The JNI environment.
     * @param commandQueue The Java CommandQueue object (for callbacks).
     * @param renderContext The native RenderContext pointer.
     */
    CommandServer(JNIEnv* env, jobject commandQueue, void* renderContext);
    
    /**
     * Destructs the CommandServer, stopping the worker thread and cleaning up resources.
     */
    ~CommandServer();
    
    // Prevent copying
    CommandServer(const CommandServer&) = delete;
    CommandServer& operator=(const CommandServer&) = delete;
    
    /**
     * Enqueues a command to be executed by the worker thread.
     * Thread-safe.
     * 
     * @param cmd The command to enqueue.
     */
    void enqueueCommand(Command cmd);
    
    /**
     * Polls for messages from the command server.
     * Delivers pending messages to Kotlin via JNI callbacks.
     */
    void pollMessages();
    
    /**
     * Gets all pending messages from the message queue.
     * Returns the messages and clears the queue.
     * 
     * @return A vector of pending messages.
     */
    std::vector<Message> getMessages();
    
    /**
     * Enqueues a LoadFile command.
     * 
     * @param requestID The request ID for async completion.
     * @param bytes The Rive file bytes.
     */
    void loadFile(int64_t requestID, const std::vector<uint8_t>& bytes);
    
    /**
     * Enqueues a DeleteFile command.
     * 
     * @param requestID The request ID for async completion.
     * @param fileHandle The handle of the file to delete.
     */
    void deleteFile(int64_t requestID, int64_t fileHandle);
    
    /**
     * Enqueues a GetArtboardNames command.
     * 
     * @param requestID The request ID for async completion.
     * @param fileHandle The handle of the file to query.
     */
    void getArtboardNames(int64_t requestID, int64_t fileHandle);
    
    /**
     * Enqueues a GetStateMachineNames command.
     * 
     * @param requestID The request ID for async completion.
     * @param artboardHandle The handle of the artboard to query.
     */
    void getStateMachineNames(int64_t requestID, int64_t artboardHandle);
    
    /**
     * Enqueues a GetViewModelNames command.
     * 
     * @param requestID The request ID for async completion.
     * @param fileHandle The handle of the file to query.
     */
    void getViewModelNames(int64_t requestID, int64_t fileHandle);
    
    /**
     * Enqueues a CreateDefaultArtboard command.
     * 
     * @param requestID The request ID for async completion.
     * @param fileHandle The handle of the file to create artboard from.
     */
    void createDefaultArtboard(int64_t requestID, int64_t fileHandle);
    
    /**
     * Enqueues a CreateArtboardByName command.
     * 
     * @param requestID The request ID for async completion.
     * @param fileHandle The handle of the file to create artboard from.
     * @param name The name of the artboard to create.
     */
    void createArtboardByName(int64_t requestID, int64_t fileHandle, const std::string& name);
    
    /**
     * Enqueues a DeleteArtboard command.
     * 
     * @param requestID The request ID for async completion.
     * @param artboardHandle The handle of the artboard to delete.
     */
    void deleteArtboard(int64_t requestID, int64_t artboardHandle);
    
    /**
     * Enqueues a CreateDefaultStateMachine command.
     * 
     * @param requestID The request ID for async completion.
     * @param artboardHandle The handle of the artboard to create state machine from.
     */
    void createDefaultStateMachine(int64_t requestID, int64_t artboardHandle);
    
    /**
     * Enqueues a CreateStateMachineByName command.
     * 
     * @param requestID The request ID for async completion.
     * @param artboardHandle The handle of the artboard to create state machine from.
     * @param name The name of the state machine to create.
     */
    void createStateMachineByName(int64_t requestID, int64_t artboardHandle, const std::string& name);
    
    /**
     * Enqueues an AdvanceStateMachine command.
     * 
     * @param requestID The request ID for async completion.
     * @param smHandle The handle of the state machine to advance.
     * @param deltaTime The time delta in seconds.
     */
    void advanceStateMachine(int64_t requestID, int64_t smHandle, float deltaTime);
    
    /**
     * Enqueues a DeleteStateMachine command.
     *
     * @param requestID The request ID for async completion.
     * @param smHandle The handle of the state machine to delete.
     */
    void deleteStateMachine(int64_t requestID, int64_t smHandle);

    // =========================================================================
    // State Machine Input Operations (Phase C.4)
    // =========================================================================

    /**
     * Enqueues a GetInputCount command.
     *
     * @param requestID The request ID for async completion.
     * @param smHandle The handle of the state machine.
     */
    void getInputCount(int64_t requestID, int64_t smHandle);

    /**
     * Enqueues a GetInputNames command.
     *
     * @param requestID The request ID for async completion.
     * @param smHandle The handle of the state machine.
     */
    void getInputNames(int64_t requestID, int64_t smHandle);

    /**
     * Enqueues a GetInputInfo command (get type and name by index).
     *
     * @param requestID The request ID for async completion.
     * @param smHandle The handle of the state machine.
     * @param inputIndex The index of the input.
     */
    void getInputInfo(int64_t requestID, int64_t smHandle, int32_t inputIndex);

    /**
     * Enqueues a GetNumberInput command.
     *
     * @param requestID The request ID for async completion.
     * @param smHandle The handle of the state machine.
     * @param inputName The name of the input.
     */
    void getNumberInput(int64_t requestID, int64_t smHandle, const std::string& inputName);

    /**
     * Enqueues a SetNumberInput command.
     *
     * @param requestID The request ID for async completion.
     * @param smHandle The handle of the state machine.
     * @param inputName The name of the input.
     * @param value The value to set.
     */
    void setNumberInput(int64_t requestID, int64_t smHandle, const std::string& inputName, float value);

    /**
     * Enqueues a GetBooleanInput command.
     *
     * @param requestID The request ID for async completion.
     * @param smHandle The handle of the state machine.
     * @param inputName The name of the input.
     */
    void getBooleanInput(int64_t requestID, int64_t smHandle, const std::string& inputName);

    /**
     * Enqueues a SetBooleanInput command.
     *
     * @param requestID The request ID for async completion.
     * @param smHandle The handle of the state machine.
     * @param inputName The name of the input.
     * @param value The value to set.
     */
    void setBooleanInput(int64_t requestID, int64_t smHandle, const std::string& inputName, bool value);

    /**
     * Enqueues a FireTrigger command.
     *
     * @param requestID The request ID for async completion.
     * @param smHandle The handle of the state machine.
     * @param inputName The name of the trigger input.
     */
    void fireTrigger(int64_t requestID, int64_t smHandle, const std::string& inputName);

    // =========================================================================
    // View Model Instance Operations (Phase D)
    // =========================================================================

    /**
     * Enqueues a CreateBlankVMI command.
     * Creates a blank (default-initialized) ViewModelInstance from a named ViewModel.
     *
     * @param requestID The request ID for async completion.
     * @param fileHandle The handle of the file containing the ViewModel.
     * @param viewModelName The name of the ViewModel to instantiate.
     */
    void createBlankVMI(int64_t requestID, int64_t fileHandle, const std::string& viewModelName);

    /**
     * Enqueues a CreateDefaultVMI command.
     * Creates the default ViewModelInstance from a named ViewModel.
     *
     * @param requestID The request ID for async completion.
     * @param fileHandle The handle of the file containing the ViewModel.
     * @param viewModelName The name of the ViewModel to instantiate.
     */
    void createDefaultVMI(int64_t requestID, int64_t fileHandle, const std::string& viewModelName);

    /**
     * Enqueues a CreateNamedVMI command.
     * Creates a named ViewModelInstance from a named ViewModel.
     *
     * @param requestID The request ID for async completion.
     * @param fileHandle The handle of the file containing the ViewModel.
     * @param viewModelName The name of the ViewModel to instantiate.
     * @param instanceName The name of the instance to create.
     */
    void createNamedVMI(int64_t requestID, int64_t fileHandle,
                        const std::string& viewModelName, const std::string& instanceName);

    /**
     * Enqueues a DeleteVMI command.
     *
     * @param requestID The request ID for async completion.
     * @param vmiHandle The handle of the ViewModelInstance to delete.
     */
    void deleteVMI(int64_t requestID, int64_t vmiHandle);

    // =========================================================================
    // Property Operations (Phase D.2)
    // =========================================================================

    /**
     * Enqueues a GetNumberProperty command.
     *
     * @param requestID The request ID for async completion.
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     */
    void getNumberProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath);

    /**
     * Enqueues a SetNumberProperty command.
     *
     * @param requestID The request ID for async completion.
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     * @param value The value to set.
     */
    void setNumberProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, float value);

    /**
     * Enqueues a GetStringProperty command.
     *
     * @param requestID The request ID for async completion.
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     */
    void getStringProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath);

    /**
     * Enqueues a SetStringProperty command.
     *
     * @param requestID The request ID for async completion.
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     * @param value The value to set.
     */
    void setStringProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, const std::string& value);

    /**
     * Enqueues a GetBooleanProperty command.
     *
     * @param requestID The request ID for async completion.
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     */
    void getBooleanProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath);

    /**
     * Enqueues a SetBooleanProperty command.
     *
     * @param requestID The request ID for async completion.
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     * @param value The value to set.
     */
    void setBooleanProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, bool value);

    // =========================================================================
    // Additional Property Types (Phase D.3)
    // =========================================================================

    /**
     * Enqueues a GetEnumProperty command.
     *
     * @param requestID The request ID for async completion.
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     */
    void getEnumProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath);

    /**
     * Enqueues a SetEnumProperty command.
     *
     * @param requestID The request ID for async completion.
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     * @param value The enum value to set (as string).
     */
    void setEnumProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, const std::string& value);

    /**
     * Enqueues a GetColorProperty command.
     *
     * @param requestID The request ID for async completion.
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     */
    void getColorProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath);

    /**
     * Enqueues a SetColorProperty command.
     *
     * @param requestID The request ID for async completion.
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     * @param value The color value (0xAARRGGBB format).
     */
    void setColorProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int32_t value);

    /**
     * Enqueues a FireTriggerProperty command.
     *
     * @param requestID The request ID for async completion.
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the trigger property.
     */
    void fireTriggerProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath);

    // ==========================================================================
    // Phase D.4: Property Subscriptions
    // ==========================================================================

    /**
     * Subscribes to property updates on a ViewModelInstance.
     *
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     * @param propertyType The type of the property (PropertyDataType value).
     */
    void subscribeToProperty(int64_t vmiHandle, const std::string& propertyPath, int32_t propertyType);

    /**
     * Unsubscribes from property updates on a ViewModelInstance.
     *
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     * @param propertyType The type of the property (PropertyDataType value).
     */
    void unsubscribeFromProperty(int64_t vmiHandle, const std::string& propertyPath, int32_t propertyType);

    // ==========================================================================
    // Phase D.5: List Operations
    // ==========================================================================

    /**
     * Enqueues a GetListSize command.
     */
    void getListSize(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath);

    /**
     * Enqueues a GetListItem command.
     */
    void getListItem(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int32_t index);

    /**
     * Enqueues an AddListItem command (append to end).
     */
    void addListItem(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int64_t itemHandle);

    /**
     * Enqueues an AddListItemAt command (insert at index).
     */
    void addListItemAt(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int32_t index, int64_t itemHandle);

    /**
     * Enqueues a RemoveListItem command (remove by handle).
     */
    void removeListItem(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int64_t itemHandle);

    /**
     * Enqueues a RemoveListItemAt command (remove by index).
     */
    void removeListItemAt(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int32_t index);

    /**
     * Enqueues a SwapListItems command.
     */
    void swapListItems(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int32_t indexA, int32_t indexB);

    // ==========================================================================
    // Phase D.5: Nested VMI Operations
    // ==========================================================================

    /**
     * Enqueues a GetInstanceProperty command.
     */
    void getInstanceProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath);

    /**
     * Enqueues a SetInstanceProperty command.
     */
    void setInstanceProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int64_t nestedHandle);

    // ==========================================================================
    // Phase D.5: Asset Property Operations
    // ==========================================================================

    /**
     * Enqueues a SetImageProperty command.
     * Pass 0 for imageHandle to clear the image.
     */
    void setImageProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int64_t imageHandle);

    /**
     * Enqueues a SetArtboardProperty command.
     * Pass 0 for artboardHandle to clear the artboard.
     * fileHandle is required to create a BindableArtboard from the artboard.
     */
    void setArtboardProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int64_t fileHandle, int64_t artboardHandle);

private:
    /**
     * The main loop for the worker thread.
     * Waits for commands and executes them.
     */
    void commandLoop();
    
    /**
     * Executes a single command.
     * 
     * @param cmd The command to execute.
     */
    void executeCommand(const Command& cmd);
    
    /**
     * Handles a LoadFile command.
     * 
     * @param cmd The command to execute.
     */
    void handleLoadFile(const Command& cmd);
    
    /**
     * Handles a DeleteFile command.
     * 
     * @param cmd The command to execute.
     */
    void handleDeleteFile(const Command& cmd);
    
    /**
     * Handles a GetArtboardNames command.
     * 
     * @param cmd The command to execute.
     */
    void handleGetArtboardNames(const Command& cmd);
    
    /**
     * Handles a GetStateMachineNames command.
     * 
     * @param cmd The command to execute.
     */
    void handleGetStateMachineNames(const Command& cmd);
    
    /**
     * Handles a GetViewModelNames command.
     * 
     * @param cmd The command to execute.
     */
    void handleGetViewModelNames(const Command& cmd);
    
    /**
     * Handles a CreateDefaultArtboard command.
     * 
     * @param cmd The command to execute.
     */
    void handleCreateDefaultArtboard(const Command& cmd);
    
    /**
     * Handles a CreateArtboardByName command.
     * 
     * @param cmd The command to execute.
     */
    void handleCreateArtboardByName(const Command& cmd);
    
    /**
     * Handles a DeleteArtboard command.
     * 
     * @param cmd The command to execute.
     */
    void handleDeleteArtboard(const Command& cmd);
    
    /**
     * Handles a CreateDefaultStateMachine command.
     * 
     * @param cmd The command to execute.
     */
    void handleCreateDefaultStateMachine(const Command& cmd);
    
    /**
     * Handles a CreateStateMachineByName command.
     * 
     * @param cmd The command to execute.
     */
    void handleCreateStateMachineByName(const Command& cmd);
    
    /**
     * Handles an AdvanceStateMachine command.
     * 
     * @param cmd The command to execute.
     */
    void handleAdvanceStateMachine(const Command& cmd);
    
    /**
     * Handles a DeleteStateMachine command.
     *
     * @param cmd The command to execute.
     */
    void handleDeleteStateMachine(const Command& cmd);

    // State machine input handlers (Phase C.4)
    void handleGetInputCount(const Command& cmd);
    void handleGetInputNames(const Command& cmd);
    void handleGetInputInfo(const Command& cmd);
    void handleGetNumberInput(const Command& cmd);
    void handleSetNumberInput(const Command& cmd);
    void handleGetBooleanInput(const Command& cmd);
    void handleSetBooleanInput(const Command& cmd);
    void handleFireTrigger(const Command& cmd);

    // View model instance handlers (Phase D)
    void handleCreateBlankVMI(const Command& cmd);
    void handleCreateDefaultVMI(const Command& cmd);
    void handleCreateNamedVMI(const Command& cmd);
    void handleDeleteVMI(const Command& cmd);

    // Property handlers (Phase D.2)
    void handleGetNumberProperty(const Command& cmd);
    void handleSetNumberProperty(const Command& cmd);
    void handleGetStringProperty(const Command& cmd);
    void handleSetStringProperty(const Command& cmd);
    void handleGetBooleanProperty(const Command& cmd);
    void handleSetBooleanProperty(const Command& cmd);

    // Additional property handlers (Phase D.3)
    void handleGetEnumProperty(const Command& cmd);
    void handleSetEnumProperty(const Command& cmd);
    void handleGetColorProperty(const Command& cmd);
    void handleSetColorProperty(const Command& cmd);
    void handleFireTriggerProperty(const Command& cmd);

    // Property subscription handlers (Phase D.4)
    void handleSubscribeToProperty(const Command& cmd);
    void handleUnsubscribeFromProperty(const Command& cmd);

    // List operation handlers (Phase D.5)
    void handleGetListSize(const Command& cmd);
    void handleGetListItem(const Command& cmd);
    void handleAddListItem(const Command& cmd);
    void handleAddListItemAt(const Command& cmd);
    void handleRemoveListItem(const Command& cmd);
    void handleRemoveListItemAt(const Command& cmd);
    void handleSwapListItems(const Command& cmd);

    // Nested VMI operation handlers (Phase D.5)
    void handleGetInstanceProperty(const Command& cmd);
    void handleSetInstanceProperty(const Command& cmd);

    // Asset property operation handlers (Phase D.5)
    void handleSetImageProperty(const Command& cmd);
    void handleSetArtboardProperty(const Command& cmd);

    /**
     * Checks subscriptions and emits property updates.
     * Called after property set operations.
     *
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     * @param propertyType The type of the property.
     */
    void emitPropertyUpdateIfSubscribed(int64_t vmiHandle, const std::string& propertyPath, PropertyDataType propertyType);

    /**
     * Enqueues a message to be sent to Kotlin.
     * Thread-safe.
     * 
     * @param msg The message to enqueue.
     */
    void enqueueMessage(Message msg);
    
    /**
     * Starts the worker thread.
     */
    void start();
    
    /**
     * Stops the worker thread and waits for it to finish.
     */
    void stop();
    
    // Thread management
    std::thread m_thread;
    std::queue<Command> m_commandQueue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_running{false};
    
    // JNI reference to the Java CommandQueue object (for callbacks in Phase B+)
    rive_mp::GlobalRef<jobject> m_commandQueueRef;
    
    // Render context (for Phase C+)
    void* m_renderContext;
    
    // Message queue for callbacks to Kotlin
    std::queue<Message> m_messageQueue;
    std::mutex m_messageMutex;
    
    // Phase B: Resource maps
    std::map<int64_t, rive::rcp<rive::File>> m_files;
    std::map<int64_t, std::unique_ptr<rive::ArtboardInstance>> m_artboards;
    std::atomic<int64_t> m_nextHandle{1};
    
    // Phase C: State machine resource map
    std::map<int64_t, std::unique_ptr<rive::StateMachineInstance>> m_stateMachines;

    // Phase D: View model instance resource map
    std::map<int64_t, rive::rcp<rive::ViewModelInstanceRuntime>> m_viewModelInstances;

    // Phase D.4: Property subscriptions
    std::vector<PropertySubscription> m_propertySubscriptions;
    std::mutex m_subscriptionsMutex;
};

} // namespace rive_android

#endif // RIVE_ANDROID_COMMAND_SERVER_HPP
