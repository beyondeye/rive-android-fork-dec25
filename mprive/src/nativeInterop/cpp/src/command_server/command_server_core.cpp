#include "command_server.hpp"
#include "rive_log.hpp"

namespace rive_android {

CommandServer::CommandServer(JNIEnv* env, jobject commandQueue, void* renderContext)
    : m_commandQueueRef(env, commandQueue)
    , m_renderContext(renderContext)
{
    LOGI("CommandServer: Constructing");
    start();
}

CommandServer::~CommandServer()
{
    LOGI("CommandServer: Destructing");
    stop();
}

void CommandServer::start()
{
    LOGI("CommandServer: Starting worker thread");
    m_running.store(true);
    m_thread = std::thread(&CommandServer::commandLoop, this);
}

void CommandServer::stop()
{
    LOGI("CommandServer: Stopping worker thread");
    
    // Signal the thread to stop
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_running.store(false);
    }
    m_cv.notify_all();
    
    // Wait for the thread to finish
    if (m_thread.joinable()) {
        LOGI("CommandServer: Waiting for worker thread to finish");
        m_thread.join();
        LOGI("CommandServer: Worker thread finished");
    }
}

void CommandServer::enqueueCommand(Command cmd)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_commandQueue.push(std::move(cmd));
    }
    m_cv.notify_one();
}

void CommandServer::commandLoop()
{
    LOGI("CommandServer: Worker thread started");
    
    // Phase C+: Initialize OpenGL context here
    // m_renderContext->initialize();
    
    while (true) {
        Command cmd;
        
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            
            // Wait for a command or stop signal
            m_cv.wait(lock, [this] { 
                return !m_commandQueue.empty() || !m_running.load(); 
            });
            
            // Check if we should stop
            if (!m_running.load() && m_commandQueue.empty()) {
                LOGI("CommandServer: Worker thread stopping");
                break;
            }
            
            // Get the next command if available
            if (!m_commandQueue.empty()) {
                cmd = std::move(m_commandQueue.front());
                m_commandQueue.pop();
            }
        }
        
        // Execute the command outside the lock
        if (cmd.type != CommandType::None) {
            executeCommand(cmd);
        }
    }
    
    // Phase C+: Cleanup OpenGL context here
    // m_renderContext->destroy();
    
    LOGI("CommandServer: Worker thread stopped");
}

void CommandServer::executeCommand(const Command& cmd)
{
    switch (cmd.type) {
        case CommandType::None:
            // Do nothing
            break;
            
        case CommandType::Stop:
            LOGI("CommandServer: Received Stop command");
            // The stop logic is handled in commandLoop
            break;
            
        case CommandType::LoadFile:
            handleLoadFile(cmd);
            break;
            
        case CommandType::DeleteFile:
            handleDeleteFile(cmd);
            break;
            
        case CommandType::GetArtboardNames:
            handleGetArtboardNames(cmd);
            break;
            
        case CommandType::GetStateMachineNames:
            handleGetStateMachineNames(cmd);
            break;
            
        case CommandType::GetViewModelNames:
            handleGetViewModelNames(cmd);
            break;

        // Phase E.2: File Introspection APIs
        case CommandType::GetViewModelInstanceNames:
            handleGetViewModelInstanceNames(cmd);
            break;

        case CommandType::GetViewModelProperties:
            handleGetViewModelProperties(cmd);
            break;

        case CommandType::GetEnums:
            handleGetEnums(cmd);
            break;
            
        case CommandType::CreateDefaultArtboard:
            handleCreateDefaultArtboard(cmd);
            break;
            
        case CommandType::CreateArtboardByName:
            handleCreateArtboardByName(cmd);
            break;
            
        case CommandType::DeleteArtboard:
            handleDeleteArtboard(cmd);
            break;

        // Phase E.3: Artboard Resizing
        case CommandType::ResizeArtboard:
            handleResizeArtboard(cmd);
            break;

        case CommandType::ResetArtboardSize:
            handleResetArtboardSize(cmd);
            break;
            
        case CommandType::CreateDefaultStateMachine:
            handleCreateDefaultStateMachine(cmd);
            break;
            
        case CommandType::CreateStateMachineByName:
            handleCreateStateMachineByName(cmd);
            break;
            
        case CommandType::AdvanceStateMachine:
            handleAdvanceStateMachine(cmd);
            break;
            
        case CommandType::DeleteStateMachine:
            handleDeleteStateMachine(cmd);
            break;

        // Phase C.4: State machine input operations
        case CommandType::GetInputCount:
            handleGetInputCount(cmd);
            break;

        case CommandType::GetInputNames:
            handleGetInputNames(cmd);
            break;

        case CommandType::GetInputInfo:
            handleGetInputInfo(cmd);
            break;

        case CommandType::GetNumberInput:
            handleGetNumberInput(cmd);
            break;

        case CommandType::SetNumberInput:
            handleSetNumberInput(cmd);
            break;

        case CommandType::GetBooleanInput:
            handleGetBooleanInput(cmd);
            break;

        case CommandType::SetBooleanInput:
            handleSetBooleanInput(cmd);
            break;

        case CommandType::FireTrigger:
            handleFireTrigger(cmd);
            break;

        // Phase D: View model instance operations
        case CommandType::CreateBlankVMI:
            handleCreateBlankVMI(cmd);
            break;

        case CommandType::CreateDefaultVMI:
            handleCreateDefaultVMI(cmd);
            break;

        case CommandType::CreateNamedVMI:
            handleCreateNamedVMI(cmd);
            break;

        case CommandType::DeleteVMI:
            handleDeleteVMI(cmd);
            break;

        // Property operations (Phase D.2)
        case CommandType::GetNumberProperty:
            handleGetNumberProperty(cmd);
            break;

        case CommandType::SetNumberProperty:
            handleSetNumberProperty(cmd);
            break;

        case CommandType::GetStringProperty:
            handleGetStringProperty(cmd);
            break;

        case CommandType::SetStringProperty:
            handleSetStringProperty(cmd);
            break;

        case CommandType::GetBooleanProperty:
            handleGetBooleanProperty(cmd);
            break;

        case CommandType::SetBooleanProperty:
            handleSetBooleanProperty(cmd);
            break;

        // Additional property types (Phase D.3)
        case CommandType::GetEnumProperty:
            handleGetEnumProperty(cmd);
            break;

        case CommandType::SetEnumProperty:
            handleSetEnumProperty(cmd);
            break;

        case CommandType::GetColorProperty:
            handleGetColorProperty(cmd);
            break;

        case CommandType::SetColorProperty:
            handleSetColorProperty(cmd);
            break;

        case CommandType::FireTriggerProperty:
            handleFireTriggerProperty(cmd);
            break;

        case CommandType::SubscribeToProperty:
            handleSubscribeToProperty(cmd);
            break;

        case CommandType::UnsubscribeFromProperty:
            handleUnsubscribeFromProperty(cmd);
            break;

        // Phase D.5: List operations
        case CommandType::GetListSize:
            handleGetListSize(cmd);
            break;

        case CommandType::GetListItem:
            handleGetListItem(cmd);
            break;

        case CommandType::AddListItem:
            handleAddListItem(cmd);
            break;

        case CommandType::AddListItemAt:
            handleAddListItemAt(cmd);
            break;

        case CommandType::RemoveListItem:
            handleRemoveListItem(cmd);
            break;

        case CommandType::RemoveListItemAt:
            handleRemoveListItemAt(cmd);
            break;

        case CommandType::SwapListItems:
            handleSwapListItems(cmd);
            break;

        // Phase D.5: Nested VMI operations
        case CommandType::GetInstanceProperty:
            handleGetInstanceProperty(cmd);
            break;

        case CommandType::SetInstanceProperty:
            handleSetInstanceProperty(cmd);
            break;

        // Phase D.5: Asset property operations
        case CommandType::SetImageProperty:
            handleSetImageProperty(cmd);
            break;

        case CommandType::SetArtboardProperty:
            handleSetArtboardProperty(cmd);
            break;

        // Phase D.6: VMI Binding to State Machine
        case CommandType::BindViewModelInstance:
            handleBindViewModelInstance(cmd);
            break;

        case CommandType::GetDefaultVMI:
            handleGetDefaultVMI(cmd);
            break;

        // Phase C.2.3: Render target operations
        case CommandType::CreateRenderTarget:
            handleCreateRenderTarget(cmd);
            break;

        case CommandType::DeleteRenderTarget:
            handleDeleteRenderTarget(cmd);
            break;

        // Phase C.2.6: Rendering operations
        case CommandType::Draw:
            handleDraw(cmd);
            break;

        // Phase E.3: Pointer events
        case CommandType::PointerMove:
            handlePointerMove(cmd);
            break;

        case CommandType::PointerDown:
            handlePointerDown(cmd);
            break;

        case CommandType::PointerUp:
            handlePointerUp(cmd);
            break;

        case CommandType::PointerExit:
            handlePointerExit(cmd);
            break;

        // Phase E.1: Asset operations
        case CommandType::DecodeImage:
            handleDecodeImage(cmd);
            break;

        case CommandType::DeleteImage:
            handleDeleteImage(cmd);
            break;

        case CommandType::RegisterImage:
            handleRegisterImage(cmd);
            break;

        case CommandType::UnregisterImage:
            handleUnregisterImage(cmd);
            break;

        case CommandType::DecodeAudio:
            handleDecodeAudio(cmd);
            break;

        case CommandType::DeleteAudio:
            handleDeleteAudio(cmd);
            break;

        case CommandType::RegisterAudio:
            handleRegisterAudio(cmd);
            break;

        case CommandType::UnregisterAudio:
            handleUnregisterAudio(cmd);
            break;

        case CommandType::DecodeFont:
            handleDecodeFont(cmd);
            break;

        case CommandType::DeleteFont:
            handleDeleteFont(cmd);
            break;

        case CommandType::RegisterFont:
            handleRegisterFont(cmd);
            break;

        case CommandType::UnregisterFont:
            handleUnregisterFont(cmd);
            break;

        default:
            LOGW("CommandServer: Unknown command type: %d",
                 static_cast<int>(cmd.type));
            break;
    }
}

void CommandServer::pollMessages()
{
    // Poll messages from the command server and send them to Kotlin
    // This is called from the main thread (Kotlin side)
    
    // This method is deprecated in favor of getMessages()
    // But kept for backward compatibility
    LOGI("CommandServer: pollMessages() called (deprecated)");
}

std::vector<Message> CommandServer::getMessages()
{
    // Get all pending messages from the message queue
    // This is called from the main thread (Kotlin side)
    
    std::vector<Message> messages;
    
    {
        std::lock_guard<std::mutex> lock(m_messageMutex);
        while (!m_messageQueue.empty()) {
            messages.push_back(std::move(m_messageQueue.front()));
            m_messageQueue.pop();
        }
    }
    
    return messages;
}

void CommandServer::enqueueMessage(Message msg)
{
    {
        std::lock_guard<std::mutex> lock(m_messageMutex);
        m_messageQueue.push(std::move(msg));
    }
    // Note: We don't notify here because pollMessages is called from Kotlin
}

} // namespace rive_android