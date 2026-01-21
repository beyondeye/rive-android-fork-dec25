#ifndef RIVE_ANDROID_COMMAND_SERVER_TYPES_HPP
#define RIVE_ANDROID_COMMAND_SERVER_TYPES_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace rive_android {

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
    // Phase E.2: File Introspection APIs
    GetViewModelInstanceNames,
    GetViewModelProperties,
    GetEnums,
    // Phase B.3: Artboard operations
    CreateDefaultArtboard,
    CreateArtboardByName,
    DeleteArtboard,
    // Phase E.3: Artboard Resizing (for Fit.Layout)
    ResizeArtboard,
    ResetArtboardSize,
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
    // Phase F: Event operations
    GetReportedEventCount,    // Get count of events fired since last advance
    GetReportedEventAt,       // Get event at specific index
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
    // Phase D.6: VMI Binding to State Machine
    BindViewModelInstance,    // Bind VMI to state machine for data binding
    GetDefaultVMI,            // Get the default VMI for an artboard (from file)
    // Phase C.2.3: Render target operations
    CreateRenderTarget,       // Create a render target for offscreen rendering
    DeleteRenderTarget,       // Delete a render target
    // Phase C.2.6: Rendering operations
    Draw,                     // Draw artboard to surface
    // Phase E.3: Pointer events
    PointerMove,              // Pointer/mouse move event
    PointerDown,              // Pointer/mouse down event
    PointerUp,                // Pointer/mouse up event
    PointerExit,              // Pointer/mouse exit event
    // Phase E.1: Asset operations
    DecodeImage,              // Decode image from bytes
    DeleteImage,              // Delete a decoded image
    RegisterImage,            // Register image by name for asset loading
    UnregisterImage,          // Unregister image by name
    DecodeAudio,              // Decode audio from bytes
    DeleteAudio,              // Delete a decoded audio
    RegisterAudio,            // Register audio by name for asset loading
    UnregisterAudio,          // Unregister audio by name
    DecodeFont,               // Decode font from bytes
    DeleteFont,               // Delete a decoded font
    RegisterFont,             // Register font by name for asset loading
    UnregisterFont,           // Unregister font by name
    // Synchronous execution on worker thread
    RunOnce,                  // Execute a function on the worker thread (for GL operations)
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
    ViewModelInstanceNamesListed,
    ViewModelPropertiesListed,
    EnumsListed,
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
    // Phase F: Event operation results
    EventCountResult,         // Returns intValue with event count
    EventDataResult,          // Returns event data
    EventOperationError,      // Event operation failed
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
    // Phase D.6: VMI Binding results
    VMIBindingSuccess,        // VMI bound to state machine successfully
    VMIBindingError,          // VMI binding failed
    DefaultVMIResult,         // Default VMI query result (returns VMI handle or 0)
    DefaultVMIError,          // Default VMI query failed
    // Phase C.2.3: Render target results
    RenderTargetCreated,      // Render target created successfully (returns handle)
    RenderTargetError,        // Render target operation failed
    RenderTargetDeleted,      // Render target deleted successfully
    // Phase C.2.6: Rendering results
    DrawComplete,             // Draw operation completed successfully
    DrawError,                // Draw operation failed
    // Phase E.1: Asset operation results
    ImageDecoded,             // Image decoded successfully (returns imageHandle)
    ImageError,               // Image decode/operation failed
    AudioDecoded,             // Audio decoded successfully (returns audioHandle)
    AudioError,               // Audio decode/operation failed
    FontDecoded,              // Font decoded successfully (returns fontHandle)
    FontError,                // Font decode/operation failed
};

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

    // Event operation data (Phase F)
    int32_t eventIndex = -1;     // For GetReportedEventAt

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

    // VMI binding operation data (Phase D.6)
    int64_t vmiHandle = 0;       // For BindViewModelInstance (VMI to bind to SM)

    // Render target operation data (Phase C.2.3)
    int32_t rtWidth = 0;         // For CreateRenderTarget (width in pixels)
    int32_t rtHeight = 0;        // For CreateRenderTarget (height in pixels)
    int32_t sampleCount = 0;     // For CreateRenderTarget (MSAA sample count, 0 = no MSAA)

    // Draw operation data (Phase C.2.6)
    int64_t artboardHandle = 0;  // For Draw
    int64_t smHandle = 0;        // For Draw (state machine to get animation state)
    int64_t surfacePtr = 0;      // For Draw (native EGL surface pointer)
    int64_t renderTargetPtr = 0; // For Draw (Rive render target pointer)
    int64_t drawKey = 0;         // For Draw (unique draw operation key)
    int32_t surfaceWidth = 0;    // For Draw
    int32_t surfaceHeight = 0;   // For Draw
    int32_t fitMode = 0;         // For Draw (Fit enum ordinal)
    int32_t alignmentMode = 0;   // For Draw (Alignment enum ordinal)
    uint32_t clearColor = 0xFF000000; // For Draw (0xAARRGGBB format)
    float scaleFactor = 1.0f;    // For Draw (for high DPI displays)

    // Pointer event data (Phase E.3)
    int8_t pointerFit = 0;       // For pointer events (Fit enum ordinal)
    int8_t pointerAlignment = 0; // For pointer events (Alignment enum ordinal)
    float layoutScale = 1.0f;    // For pointer events (layout scale factor)
    float pointerSurfaceWidth = 0.0f;  // For pointer events
    float pointerSurfaceHeight = 0.0f; // For pointer events
    int32_t pointerID = 0;       // For pointer events (multi-touch support)
    float pointerX = 0.0f;       // For pointer events (x coordinate)
    float pointerY = 0.0f;       // For pointer events (y coordinate)

    // RunOnce callback (Phase C.2.6 - synchronous GL operations)
    std::function<void()> runOnceCallback;

    Command() = default;
    explicit Command(CommandType t, int64_t reqID = 0) 
        : type(t), requestID(reqID) {}
};

/**
 * A message to be sent from CommandServer to Kotlin.
 */
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

    // Event operation results (Phase F)
    std::string eventName;       // For EventDataResult
    int32_t eventTypeCode = 0;   // For EventDataResult (128=General, 131=OpenURL, 407=Audio)
    float eventDelay = 0.0f;     // For EventDataResult (delay in seconds)
    std::string eventUrl;        // For EventDataResult (OpenURLEvent only)
    int32_t eventTargetValue = 0;// For EventDataResult (0=_blank, 1=_parent, 2=_self, 3=_top)
    uint32_t eventAssetId = 0;   // For EventDataResult (AudioEvent only)
    // Event properties stored as parallel vectors (name, type, value)
    std::vector<std::string> eventPropertyNames;
    std::vector<int32_t> eventPropertyTypes;  // 0=bool, 1=float, 2=string
    std::vector<bool> eventPropertyBools;
    std::vector<float> eventPropertyFloats;
    std::vector<std::string> eventPropertyStrings;

    Message() = default;
    explicit Message(MessageType t, int64_t reqID = 0)
        : type(t), requestID(reqID) {}
};

} // namespace rive_android

#endif // RIVE_ANDROID_COMMAND_SERVER_TYPES_HPP