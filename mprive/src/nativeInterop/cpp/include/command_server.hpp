#ifndef RIVE_ANDROID_COMMAND_SERVER_HPP
#define RIVE_ANDROID_COMMAND_SERVER_HPP

#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include "jni_refs.hpp"
#include "command_server_types.hpp"

// Rive headers
#include "rive/file.hpp"
#include "rive/bindable_artboard.hpp"
#include "rive/animation/state_machine_instance.hpp"
#include "rive/animation/linear_animation_instance.hpp"
#include "rive/viewmodel/runtime/viewmodel_runtime.hpp"
#include "rive/viewmodel/runtime/viewmodel_instance_runtime.hpp"
#include "utils/no_op_factory.hpp"

// Forward declarations for Rive GPU types
namespace rive {
namespace gpu {
class RenderTargetGL;
}
}

namespace rive_android {

using rive_mp::GlobalRef;

// Forward declarations
class RenderContext;

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
     * Executes a function on the worker thread where the GL context is active.
     * This is used for operations that require OpenGL (like creating render targets).
     * The function is executed synchronously - this call blocks until the function completes.
     *
     * @param func The function to execute on the worker thread.
     */
    void runOnce(std::function<void()> func);
    
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
    
    // =========================================================================
    // Phase E.2: File Introspection APIs
    // =========================================================================

    /**
     * Enqueues a GetViewModelInstanceNames command.
     * Gets the names of all ViewModel instances for a given ViewModel in a file.
     * 
     * @param requestID The request ID for async completion.
     * @param fileHandle The handle of the file to query.
     * @param viewModelName The name of the ViewModel to query instances for.
     */
    void getViewModelInstanceNames(int64_t requestID, int64_t fileHandle, const std::string& viewModelName);

    /**
     * Enqueues a GetViewModelProperties command.
     * Gets the properties defined on a ViewModel in a file.
     * 
     * @param requestID The request ID for async completion.
     * @param fileHandle The handle of the file to query.
     * @param viewModelName The name of the ViewModel to query properties for.
     */
    void getViewModelProperties(int64_t requestID, int64_t fileHandle, const std::string& viewModelName);

    /**
     * Enqueues a GetEnums command.
     * Gets all enum definitions in a file.
     * 
     * @param requestID The request ID for async completion.
     * @param fileHandle The handle of the file to query.
     */
    void getEnums(int64_t requestID, int64_t fileHandle);

    /**
     * Enqueues a CreateDefaultArtboard command.
     * 
     * @param requestID The request ID for async completion.
     * @param fileHandle The handle of the file to create artboard from.
     */
    void createDefaultArtboard(int64_t requestID, int64_t fileHandle);
    
    /**
     * Creates the default artboard synchronously (called from JNI thread).
     * 
     * @param fileHandle The handle of the file to create artboard from.
     * @return The artboard handle, or 0 if creation failed.
     */
    int64_t createDefaultArtboardSync(int64_t fileHandle);
    
    /**
     * Enqueues a CreateArtboardByName command.
     * 
     * @param requestID The request ID for async completion.
     * @param fileHandle The handle of the file to create artboard from.
     * @param name The name of the artboard to create.
     */
    void createArtboardByName(int64_t requestID, int64_t fileHandle, const std::string& name);
    
    /**
     * Creates an artboard by name synchronously (called from JNI thread).
     * 
     * @param fileHandle The handle of the file to create artboard from.
     * @param name The name of the artboard to create.
     * @return The artboard handle, or 0 if creation failed.
     */
    int64_t createArtboardByNameSync(int64_t fileHandle, const std::string& name);
    
    /**
     * Enqueues a DeleteArtboard command.
     * 
     * @param requestID The request ID for async completion.
     * @param artboardHandle The handle of the artboard to delete.
     */
    void deleteArtboard(int64_t requestID, int64_t artboardHandle);
    
    // =========================================================================
    // Phase E.3: Artboard Resizing (for Fit.Layout)
    // =========================================================================

    /**
     * Resizes an artboard to the specified dimensions.
     * This is required for Fit.Layout mode where the artboard must match surface dimensions.
     * Fire-and-forget operation.
     *
     * @param artboardHandle The handle of the artboard to resize.
     * @param width The new width in pixels.
     * @param height The new height in pixels.
     * @param scaleFactor Scale factor for high DPI displays (default 1.0).
     */
    void resizeArtboard(int64_t artboardHandle, int32_t width, int32_t height, float scaleFactor);

    /**
     * Resets an artboard to its original dimensions defined in the Rive file.
     * Fire-and-forget operation.
     *
     * @param artboardHandle The handle of the artboard to reset.
     */
    void resetArtboardSize(int64_t artboardHandle);

    /**
     * Enqueues a CreateDefaultStateMachine command.
     * 
     * @param requestID The request ID for async completion.
     * @param artboardHandle The handle of the artboard to create state machine from.
     */
    void createDefaultStateMachine(int64_t requestID, int64_t artboardHandle);
    
    /**
     * Creates the default state machine synchronously (called from JNI thread).
     * 
     * @param artboardHandle The handle of the artboard to create state machine from.
     * @return The state machine handle, or 0 if creation failed.
     */
    int64_t createDefaultStateMachineSync(int64_t artboardHandle);
    
    /**
     * Enqueues a CreateStateMachineByName command.
     * 
     * @param requestID The request ID for async completion.
     * @param artboardHandle The handle of the artboard to create state machine from.
     * @param name The name of the state machine to create.
     */
    void createStateMachineByName(int64_t requestID, int64_t artboardHandle, const std::string& name);
    
    /**
     * Creates a state machine by name synchronously (called from JNI thread).
     * 
     * @param artboardHandle The handle of the artboard to create state machine from.
     * @param name The name of the state machine to create.
     * @return The state machine handle, or 0 if creation failed.
     */
    int64_t createStateMachineByNameSync(int64_t artboardHandle, const std::string& name);
    
    /**
     * Enqueues an AdvanceStateMachine command.
     * 
     * This matches the kotlin/src/main reference implementation.
     * No requestID needed - this is a fire-and-forget operation.
     * 
     * @param smHandle The handle of the state machine to advance.
     * @param deltaTime The time delta in seconds.
     */
    void advanceStateMachine(int64_t smHandle, float deltaTime);
    
    /**
     * Enqueues a DeleteStateMachine command.
     *
     * @param requestID The request ID for async completion.
     * @param smHandle The handle of the state machine to delete.
     */
    void deleteStateMachine(int64_t requestID, int64_t smHandle);

    // =========================================================================
    // Linear Animation Operations
    // =========================================================================

    /**
     * Creates the default (first) animation synchronously.
     *
     * @param artboardHandle The handle of the artboard to create animation from.
     * @return The animation handle, or 0 if creation failed.
     */
    int64_t createDefaultAnimationSync(int64_t artboardHandle);

    /**
     * Creates an animation by name synchronously.
     *
     * @param artboardHandle The handle of the artboard to create animation from.
     * @param name The name of the animation to create.
     * @return The animation handle, or 0 if creation failed.
     */
    int64_t createAnimationByNameSync(int64_t artboardHandle, const std::string& name);

    /**
     * Advances and applies an animation to its artboard.
     * Returns whether the animation is still playing (false for oneShot that completed).
     *
     * @param animHandle The handle of the animation.
     * @param artboardHandle The handle of the artboard.
     * @param deltaTime The time delta in seconds.
     * @param advanceArtboard If true, also advances the artboard (use when no state machine is active).
     * @return true if animation is still playing, false if completed.
     */
    bool advanceAndApplyAnimation(int64_t animHandle, int64_t artboardHandle, float deltaTime, bool advanceArtboard);

    /**
     * Deletes an animation instance.
     *
     * @param animHandle The handle of the animation to delete.
     */
    void deleteAnimation(int64_t animHandle);

    /**
     * Sets the animation's current time position.
     *
     * @param animHandle The handle of the animation.
     * @param time The time in seconds.
     */
    void setAnimationTime(int64_t animHandle, float time);

    /**
     * Sets the animation's loop mode.
     *
     * @param animHandle The handle of the animation.
     * @param loopMode 0=oneShot, 1=loop, 2=pingPong.
     */
    void setAnimationLoop(int64_t animHandle, int32_t loopMode);

    /**
     * Sets the animation's playback direction.
     *
     * @param animHandle The handle of the animation.
     * @param direction 1=forwards, -1=backwards.
     */
    void setAnimationDirection(int64_t animHandle, int32_t direction);

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

    // ==========================================================================
    // Phase D.6: VMI Binding to State Machine
    // ==========================================================================

    /**
     * Enqueues a BindViewModelInstance command.
     * Binds a VMI to a state machine for data binding.
     */
    void bindViewModelInstance(int64_t requestID, int64_t smHandle, int64_t vmiHandle);

    /**
     * Enqueues a GetDefaultVMI command.
     * Gets the default VMI for an artboard from its file.
     */
    void getDefaultViewModelInstance(int64_t requestID, int64_t fileHandle, int64_t artboardHandle);

    // ==========================================================================
    // Phase C.2.3: Render Target Operations
    // ==========================================================================

    /**
     * Enqueues a CreateRenderTarget command.
     * Creates an offscreen render target for rendering Rive content.
     *
     * @param requestID The request ID for async completion.
     * @param width The width of the render target in pixels.
     * @param height The height of the render target in pixels.
     * @param sampleCount MSAA sample count (0 = no MSAA, typical values: 4, 8, 16).
     */
    void createRenderTarget(int64_t requestID, int32_t width, int32_t height, int32_t sampleCount);

    /**
     * Enqueues a DeleteRenderTarget command.
     * Deletes a render target and frees its resources.
     *
     * @param requestID The request ID for async completion.
     * @param renderTargetHandle The handle of the render target to delete.
     */
    void deleteRenderTarget(int64_t requestID, int64_t renderTargetHandle);

    // ==========================================================================
    // Phase C.2.6: Rendering Operations
    // ==========================================================================

    /**
     * Enqueues a Draw command.
     * Draws an artboard with its state machine state to a surface.
     *
     * @param requestID The request ID for async completion.
     * @param artboardHandle Handle to the artboard to draw.
     * @param smHandle Handle to the state machine (for animation state).
     * @param surfacePtr Native EGL surface pointer.
     * @param renderTargetPtr Rive render target pointer.
     * @param drawKey Unique draw operation key.
     * @param width Surface width in pixels.
     * @param height Surface height in pixels.
     * @param fitMode Fit enum ordinal (0=FILL, 1=CONTAIN, etc.).
     * @param alignmentMode Alignment enum ordinal.
     * @param clearColor Background clear color (0xAARRGGBB format).
     * @param scaleFactor Scale factor for high DPI displays (default 1.0).
     */
    void draw(int64_t requestID,
              int64_t artboardHandle,
              int64_t smHandle,
              int64_t surfacePtr,
              int64_t renderTargetPtr,
              int64_t drawKey,
              int32_t width,
              int32_t height,
              int32_t fitMode,
              int32_t alignmentMode,
              uint32_t clearColor,
              float scaleFactor);

    // ==========================================================================
    // Phase E.3: Pointer Events
    // ==========================================================================

    /**
     * Enqueues a PointerMove command.
     * Sends a pointer move event to a state machine with coordinate transformation.
     *
     * @param smHandle Handle to the state machine.
     * @param fit Fit mode for coordinate transformation.
     * @param alignment Alignment mode for coordinate transformation.
     * @param layoutScale Layout scale factor.
     * @param surfaceWidth Surface width in pixels.
     * @param surfaceHeight Surface height in pixels.
     * @param pointerID Pointer ID for multi-touch support.
     * @param x X coordinate in surface space.
     * @param y Y coordinate in surface space.
     */
    void pointerMove(int64_t smHandle, int8_t fit, int8_t alignment,
                     float layoutScale, float surfaceWidth, float surfaceHeight,
                     int32_t pointerID, float x, float y);

    /**
     * Enqueues a PointerDown command.
     * Sends a pointer down (press) event to a state machine.
     */
    void pointerDown(int64_t smHandle, int8_t fit, int8_t alignment,
                     float layoutScale, float surfaceWidth, float surfaceHeight,
                     int32_t pointerID, float x, float y);

    /**
     * Enqueues a PointerUp command.
     * Sends a pointer up (release) event to a state machine.
     */
    void pointerUp(int64_t smHandle, int8_t fit, int8_t alignment,
                   float layoutScale, float surfaceWidth, float surfaceHeight,
                   int32_t pointerID, float x, float y);

    /**
     * Enqueues a PointerExit command.
     * Sends a pointer exit event to a state machine.
     */
    void pointerExit(int64_t smHandle, int8_t fit, int8_t alignment,
                     float layoutScale, float surfaceWidth, float surfaceHeight,
                     int32_t pointerID, float x, float y);

    // ==========================================================================
    // Phase E.1: Asset Operations
    // ==========================================================================

    /**
     * Enqueues a DecodeImage command.
     * Decodes image data from bytes and returns a handle via callback.
     *
     * @param requestID The request ID for async completion.
     * @param bytes The image bytes (PNG, JPEG, etc.).
     */
    void decodeImage(int64_t requestID, const std::vector<uint8_t>& bytes);

    /**
     * Deletes a decoded image (synchronous, fire-and-forget).
     *
     * @param imageHandle The handle of the image to delete.
     */
    void deleteImage(int64_t imageHandle);

    /**
     * Registers an image by name for asset loading.
     *
     * @param name The name to register the image under.
     * @param imageHandle The handle of the decoded image.
     */
    void registerImage(const std::string& name, int64_t imageHandle);

    /**
     * Unregisters an image by name.
     *
     * @param name The name of the image to unregister.
     */
    void unregisterImage(const std::string& name);

    /**
     * Enqueues a DecodeAudio command.
     * Decodes audio data from bytes and returns a handle via callback.
     *
     * @param requestID The request ID for async completion.
     * @param bytes The audio bytes (WAV, MP3, etc.).
     */
    void decodeAudio(int64_t requestID, const std::vector<uint8_t>& bytes);

    /**
     * Deletes a decoded audio clip (synchronous, fire-and-forget).
     *
     * @param audioHandle The handle of the audio to delete.
     */
    void deleteAudio(int64_t audioHandle);

    /**
     * Registers an audio clip by name for asset loading.
     *
     * @param name The name to register the audio under.
     * @param audioHandle The handle of the decoded audio.
     */
    void registerAudio(const std::string& name, int64_t audioHandle);

    /**
     * Unregisters an audio clip by name.
     *
     * @param name The name of the audio to unregister.
     */
    void unregisterAudio(const std::string& name);

    /**
     * Enqueues a DecodeFont command.
     * Decodes font data from bytes and returns a handle via callback.
     *
     * @param requestID The request ID for async completion.
     * @param bytes The font bytes (TTF, OTF, etc.).
     */
    void decodeFont(int64_t requestID, const std::vector<uint8_t>& bytes);

    /**
     * Deletes a decoded font (synchronous, fire-and-forget).
     *
     * @param fontHandle The handle of the font to delete.
     */
    void deleteFont(int64_t fontHandle);

    /**
     * Registers a font by name for asset loading.
     *
     * @param name The name to register the font under.
     * @param fontHandle The handle of the decoded font.
     */
    void registerFont(const std::string& name, int64_t fontHandle);

    /**
     * Unregisters a font by name.
     *
     * @param name The name of the font to unregister.
     */
    void unregisterFont(const std::string& name);

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
    
    // Phase E.2: File Introspection handlers
    void handleGetViewModelInstanceNames(const Command& cmd);
    void handleGetViewModelProperties(const Command& cmd);
    void handleGetEnums(const Command& cmd);

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

    // VMI binding operation handlers (Phase D.6)
    void handleBindViewModelInstance(const Command& cmd);
    void handleGetDefaultVMI(const Command& cmd);

    // Render target operation handlers (Phase C.2.3)
    void handleCreateRenderTarget(const Command& cmd);
    void handleDeleteRenderTarget(const Command& cmd);

    // Rendering operation handlers (Phase C.2.6)
    void handleDraw(const Command& cmd);

    // Artboard resizing handlers (Phase E.3)
    void handleResizeArtboard(const Command& cmd);
    void handleResetArtboardSize(const Command& cmd);

    // Pointer event handlers (Phase E.3)
    void handlePointerMove(const Command& cmd);
    void handlePointerDown(const Command& cmd);
    void handlePointerUp(const Command& cmd);
    void handlePointerExit(const Command& cmd);

    // Asset operation handlers (Phase E.1)
    void handleDecodeImage(const Command& cmd);
    void handleDeleteImage(const Command& cmd);
    void handleRegisterImage(const Command& cmd);
    void handleUnregisterImage(const Command& cmd);
    void handleDecodeAudio(const Command& cmd);
    void handleDeleteAudio(const Command& cmd);
    void handleRegisterAudio(const Command& cmd);
    void handleUnregisterAudio(const Command& cmd);
    void handleDecodeFont(const Command& cmd);
    void handleDeleteFont(const Command& cmd);
    void handleRegisterFont(const Command& cmd);
    void handleUnregisterFont(const Command& cmd);

    /**
     * Transforms surface coordinates to artboard coordinates.
     * Used by pointer event handlers.
     *
     * @param smHandle State machine handle (to get artboard bounds).
     * @param fit Fit mode for transformation.
     * @param alignment Alignment mode for transformation.
     * @param layoutScale Layout scale factor.
     * @param surfaceWidth Surface width in pixels.
     * @param surfaceHeight Surface height in pixels.
     * @param surfaceX X coordinate in surface space.
     * @param surfaceY Y coordinate in surface space.
     * @param outX Output X coordinate in artboard space.
     * @param outY Output Y coordinate in artboard space.
     * @return true if transformation succeeded, false otherwise.
     */
    bool transformToArtboardCoords(int64_t smHandle, int8_t fit, int8_t alignment,
                                   float layoutScale, float surfaceWidth, float surfaceHeight,
                                   float surfaceX, float surfaceY,
                                   float& outX, float& outY);

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

    // Fallback factory for when no render context is available (for tests)
    std::unique_ptr<rive::NoOpFactory> m_noOpFactory;
    
    // Message queue for callbacks to Kotlin
    std::queue<Message> m_messageQueue;
    std::mutex m_messageMutex;
    
    // Phase B: Resource maps (protected by m_resourceMutex for thread safety)
    std::map<int64_t, rive::rcp<rive::File>> m_files;
    std::map<int64_t, rive::rcp<rive::BindableArtboard>> m_artboards;
    std::atomic<int64_t> m_nextHandle{1};
    
    // Mutex to protect resource maps when accessed from multiple threads
    // (sync methods run on calling thread, async handlers run on worker thread)
    mutable std::mutex m_resourceMutex;
    
    // Phase C: State machine resource map
    std::map<int64_t, std::unique_ptr<rive::StateMachineInstance>> m_stateMachines;

    // Linear animation resource map (for files without auto-playing state machines)
    std::map<int64_t, std::unique_ptr<rive::LinearAnimationInstance>> m_animations;

    // Phase C.2.3: Render target resource map
    std::map<int64_t, rive::gpu::RenderTargetGL*> m_renderTargets;

    // Phase D: View model instance resource map
    std::map<int64_t, rive::rcp<rive::ViewModelInstanceRuntime>> m_viewModelInstances;

    // Phase D.4: Property subscriptions
    std::vector<PropertySubscription> m_propertySubscriptions;
    std::mutex m_subscriptionsMutex;

    // Phase E.1: Asset resource maps
    // Note: Using void* as placeholder - actual types depend on Rive image/audio/font implementations
    std::map<int64_t, void*> m_images;        // Image handle -> decoded image data
    std::map<int64_t, void*> m_audioClips;    // Audio handle -> decoded audio data
    std::map<int64_t, void*> m_fonts;         // Font handle -> decoded font data
    std::map<std::string, int64_t> m_registeredImages;  // name -> image handle
    std::map<std::string, int64_t> m_registeredAudio;   // name -> audio handle
    std::map<std::string, int64_t> m_registeredFonts;   // name -> font handle
};

} // namespace rive_android

#endif // RIVE_ANDROID_COMMAND_SERVER_HPP