#include "command_server.hpp"
#include "render_context.hpp"
#include "rive_log.hpp"
#include "rive/viewmodel/viewmodel.hpp"
#include "rive/animation/state_machine_instance.hpp"
#include "rive/animation/state_machine_input_instance.hpp"
#include "rive/animation/state_machine_bool.hpp"
#include "rive/animation/state_machine_number.hpp"
#include "rive/animation/state_machine_trigger.hpp"
#include "rive/renderer/rive_renderer.hpp"
#include "rive/renderer/gl/render_target_gl.hpp"
#include "rive/math/aabb.hpp"
#include "utils/no_op_factory.hpp"
#include <cassert>

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
            
        case CommandType::CreateDefaultArtboard:
            handleCreateDefaultArtboard(cmd);
            break;
            
        case CommandType::CreateArtboardByName:
            handleCreateArtboardByName(cmd);
            break;
            
        case CommandType::DeleteArtboard:
            handleDeleteArtboard(cmd);
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

void CommandServer::loadFile(int64_t requestID, const std::vector<uint8_t>& bytes)
{
    LOGI("CommandServer: Enqueuing LoadFile command (requestID=%lld, size=%zu)",
         static_cast<long long>(requestID), bytes.size());
    
    Command cmd(CommandType::LoadFile, requestID);
    cmd.bytes = bytes;
    
    enqueueCommand(std::move(cmd));
}

void CommandServer::deleteFile(int64_t requestID, int64_t fileHandle)
{
    LOGI("CommandServer: Enqueuing DeleteFile command (requestID=%lld, handle=%lld)",
         static_cast<long long>(requestID), static_cast<long long>(fileHandle));
    
    Command cmd(CommandType::DeleteFile, requestID);
    cmd.handle = fileHandle;
    
    enqueueCommand(std::move(cmd));
}

void CommandServer::handleLoadFile(const Command& cmd)
{
    LOGI("CommandServer: Handling LoadFile command (requestID=%lld, size=%zu)",
         static_cast<long long>(cmd.requestID), cmd.bytes.size());

    // Get the factory - use render context factory if available, otherwise NoOpFactory
    rive::Factory* factory = nullptr;
    if (m_renderContext != nullptr) {
        // TODO: Get factory from render context in Phase C
        // factory = static_cast<RenderContext*>(m_renderContext)->getFactory();
    }
    if (factory == nullptr) {
        // Use NoOpFactory for tests or when no render context
        if (!m_noOpFactory) {
            m_noOpFactory = std::make_unique<rive::NoOpFactory>();
        }
        factory = m_noOpFactory.get();
    }

    // Import the Rive file
    auto file = rive::File::import(
        rive::Span<const uint8_t>(cmd.bytes.data(), cmd.bytes.size()),
        factory,
        nullptr,  // ImportResult
        nullptr   // No asset loader for now (Phase E)
    );
    
    if (file) {
        // Generate a unique handle
        int64_t handle = m_nextHandle.fetch_add(1);
        
        // Store the file
        m_files[handle] = file;
        
        LOGI("CommandServer: File loaded successfully (handle=%lld)", 
             static_cast<long long>(handle));
        
        // Send success message
        Message msg(MessageType::FileLoaded, cmd.requestID);
        msg.handle = handle;
        enqueueMessage(std::move(msg));
    } else {
        // Send error message
        LOGE("CommandServer: Failed to load file");
        
        Message msg(MessageType::FileError, cmd.requestID);
        msg.error = "Failed to import Rive file";
        enqueueMessage(std::move(msg));
    }
}

void CommandServer::handleDeleteFile(const Command& cmd)
{
    LOGI("CommandServer: Handling DeleteFile command (requestID=%lld, handle=%lld)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle));
    
    auto it = m_files.find(cmd.handle);
    if (it != m_files.end()) {
        m_files.erase(it);
        
        LOGI("CommandServer: File deleted successfully (handle=%lld)", 
             static_cast<long long>(cmd.handle));
        
        // Send success message
        Message msg(MessageType::FileDeleted, cmd.requestID);
        msg.handle = cmd.handle;
        enqueueMessage(std::move(msg));
    } else {
        LOGW("CommandServer: Attempted to delete non-existent file (handle=%lld)",
             static_cast<long long>(cmd.handle));
        
        // Send error message
        Message msg(MessageType::FileError, cmd.requestID);
        msg.error = "Invalid file handle";
        enqueueMessage(std::move(msg));
    }
}

void CommandServer::enqueueMessage(Message msg)
{
    {
        std::lock_guard<std::mutex> lock(m_messageMutex);
        m_messageQueue.push(std::move(msg));
    }
    // Note: We don't notify here because pollMessages is called from Kotlin
}

void CommandServer::getArtboardNames(int64_t requestID, int64_t fileHandle)
{
    LOGI("CommandServer: Enqueuing GetArtboardNames command (requestID=%lld, fileHandle=%lld)",
         static_cast<long long>(requestID), static_cast<long long>(fileHandle));
    
    Command cmd(CommandType::GetArtboardNames, requestID);
    cmd.handle = fileHandle;
    
    enqueueCommand(std::move(cmd));
}

void CommandServer::getStateMachineNames(int64_t requestID, int64_t artboardHandle)
{
    LOGI("CommandServer: Enqueuing GetStateMachineNames command (requestID=%lld, artboardHandle=%lld)",
         static_cast<long long>(requestID), static_cast<long long>(artboardHandle));
    
    Command cmd(CommandType::GetStateMachineNames, requestID);
    cmd.handle = artboardHandle;
    
    enqueueCommand(std::move(cmd));
}

void CommandServer::getViewModelNames(int64_t requestID, int64_t fileHandle)
{
    LOGI("CommandServer: Enqueuing GetViewModelNames command (requestID=%lld, fileHandle=%lld)",
         static_cast<long long>(requestID), static_cast<long long>(fileHandle));
    
    Command cmd(CommandType::GetViewModelNames, requestID);
    cmd.handle = fileHandle;
    
    enqueueCommand(std::move(cmd));
}

void CommandServer::handleGetArtboardNames(const Command& cmd)
{
    LOGI("CommandServer: Handling GetArtboardNames command (requestID=%lld, fileHandle=%lld)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle));
    
    auto it = m_files.find(cmd.handle);
    if (it == m_files.end()) {
        LOGW("CommandServer: Invalid file handle: %lld", static_cast<long long>(cmd.handle));
        
        Message msg(MessageType::QueryError, cmd.requestID);
        msg.error = "Invalid file handle";
        enqueueMessage(std::move(msg));
        return;
    }
    
    std::vector<std::string> names;
    auto& file = it->second;
    
    for (size_t i = 0; i < file->artboardCount(); i++) {
        auto artboard = file->artboard(i);
        if (artboard) {
            names.push_back(artboard->name());
        }
    }
    
    LOGI("CommandServer: Found %zu artboard names", names.size());
    
    Message msg(MessageType::ArtboardNamesListed, cmd.requestID);
    msg.stringList = std::move(names);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleGetStateMachineNames(const Command& cmd)
{
    LOGI("CommandServer: Handling GetStateMachineNames command (requestID=%lld, artboardHandle=%lld)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle));
    
    std::vector<std::string> names;
    
    {
        std::lock_guard<std::mutex> lock(m_resourceMutex);
        
        auto it = m_artboards.find(cmd.handle);
        if (it == m_artboards.end()) {
            LOGW("CommandServer: Invalid artboard handle: %lld", static_cast<long long>(cmd.handle));
            
            Message msg(MessageType::QueryError, cmd.requestID);
            msg.error = "Invalid artboard handle";
            enqueueMessage(std::move(msg));
            return;
        }
        
        auto& artboard = it->second;
        
        for (size_t i = 0; i < artboard->stateMachineCount(); i++) {
            auto sm = artboard->stateMachine(i);
            if (sm) {
                names.push_back(sm->name());
            }
        }
    }
    
    LOGI("CommandServer: Found %zu state machine names", names.size());
    
    Message msg(MessageType::StateMachineNamesListed, cmd.requestID);
    msg.stringList = std::move(names);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleGetViewModelNames(const Command& cmd)
{
    LOGI("CommandServer: Handling GetViewModelNames command (requestID=%lld, fileHandle=%lld)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle));
    
    auto it = m_files.find(cmd.handle);
    if (it == m_files.end()) {
        LOGW("CommandServer: Invalid file handle: %lld", static_cast<long long>(cmd.handle));
        
        Message msg(MessageType::QueryError, cmd.requestID);
        msg.error = "Invalid file handle";
        enqueueMessage(std::move(msg));
        return;
    }
    
    std::vector<std::string> names;
    auto& file = it->second;
    
    // Get view model names from the file
    for (size_t i = 0; i < file->viewModelCount(); i++) {
        auto vm = file->viewModel(i);
        if (vm) {
            names.push_back(vm->name());
        }
    }
    
    LOGI("CommandServer: Found %zu view model names", names.size());
    
    Message msg(MessageType::ViewModelNamesListed, cmd.requestID);
    msg.stringList = std::move(names);
    enqueueMessage(std::move(msg));
}

void CommandServer::createDefaultArtboard(int64_t requestID, int64_t fileHandle)
{
    LOGI("CommandServer: Enqueuing CreateDefaultArtboard command (requestID=%lld, fileHandle=%lld)",
         static_cast<long long>(requestID), static_cast<long long>(fileHandle));
    
    Command cmd(CommandType::CreateDefaultArtboard, requestID);
    cmd.handle = fileHandle;
    
    enqueueCommand(std::move(cmd));
}

void CommandServer::createArtboardByName(int64_t requestID, int64_t fileHandle, const std::string& name)
{
    LOGI("CommandServer: Enqueuing CreateArtboardByName command (requestID=%lld, fileHandle=%lld, name=%s)",
         static_cast<long long>(requestID), static_cast<long long>(fileHandle), name.c_str());
    
    Command cmd(CommandType::CreateArtboardByName, requestID);
    cmd.handle = fileHandle;
    cmd.name = name;
    
    enqueueCommand(std::move(cmd));
}

int64_t CommandServer::createDefaultArtboardSync(int64_t fileHandle)
{
    LOGI("CommandServer: Creating default artboard synchronously (fileHandle=%lld)",
         static_cast<long long>(fileHandle));
    
    std::lock_guard<std::mutex> lock(m_resourceMutex);
    
    auto it = m_files.find(fileHandle);
    if (it == m_files.end()) {
        LOGW("CommandServer: Invalid file handle: %lld", static_cast<long long>(fileHandle));
        return 0;
    }
    
    // Create the default artboard (returns unique_ptr<ArtboardInstance>)
    auto artboard = it->second->artboardDefault();
    if (!artboard) {
        LOGW("CommandServer: Failed to create default artboard");
        return 0;
    }
    
    // Generate a unique handle
    int64_t handle = m_nextHandle.fetch_add(1);
    
    // Store the artboard
    m_artboards[handle] = std::move(artboard);
    
    LOGI("CommandServer: Artboard created synchronously (handle=%lld)", 
         static_cast<long long>(handle));
    
    return handle;
}

int64_t CommandServer::createArtboardByNameSync(int64_t fileHandle, const std::string& name)
{
    LOGI("CommandServer: Creating artboard by name synchronously (fileHandle=%lld, name=%s)",
         static_cast<long long>(fileHandle), name.c_str());
    
    std::lock_guard<std::mutex> lock(m_resourceMutex);
    
    auto it = m_files.find(fileHandle);
    if (it == m_files.end()) {
        LOGW("CommandServer: Invalid file handle: %lld", static_cast<long long>(fileHandle));
        return 0;
    }
    
    // Create the artboard by name (returns unique_ptr<ArtboardInstance>)
    auto artboard = it->second->artboardNamed(name);
    if (!artboard) {
        LOGW("CommandServer: Failed to create artboard with name: %s", name.c_str());
        return 0;
    }
    
    // Generate a unique handle
    int64_t handle = m_nextHandle.fetch_add(1);
    
    // Store the artboard
    m_artboards[handle] = std::move(artboard);
    
    LOGI("CommandServer: Artboard created synchronously (handle=%lld, name=%s)", 
         static_cast<long long>(handle), name.c_str());
    
    return handle;
}

void CommandServer::deleteArtboard(int64_t requestID, int64_t artboardHandle)
{
    LOGI("CommandServer: Enqueuing DeleteArtboard command (requestID=%lld, artboardHandle=%lld)",
         static_cast<long long>(requestID), static_cast<long long>(artboardHandle));
    
    Command cmd(CommandType::DeleteArtboard, requestID);
    cmd.handle = artboardHandle;
    
    enqueueCommand(std::move(cmd));
}

void CommandServer::handleCreateDefaultArtboard(const Command& cmd)
{
    LOGI("CommandServer: Handling CreateDefaultArtboard command (requestID=%lld, fileHandle=%lld)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle));
    
    auto it = m_files.find(cmd.handle);
    if (it == m_files.end()) {
        LOGW("CommandServer: Invalid file handle: %lld", static_cast<long long>(cmd.handle));
        
        Message msg(MessageType::ArtboardError, cmd.requestID);
        msg.error = "Invalid file handle";
        enqueueMessage(std::move(msg));
        return;
    }
    
    // Create the default artboard (returns unique_ptr<ArtboardInstance>)
    auto artboard = it->second->artboardDefault();
    if (!artboard) {
        LOGW("CommandServer: Failed to create default artboard");
        
        Message msg(MessageType::ArtboardError, cmd.requestID);
        msg.error = "Failed to create default artboard";
        enqueueMessage(std::move(msg));
        return;
    }
    
    // Generate a unique handle
    int64_t handle = m_nextHandle.fetch_add(1);
    
    // Store the artboard
    m_artboards[handle] = std::move(artboard);
    
    LOGI("CommandServer: Artboard created successfully (handle=%lld)", 
         static_cast<long long>(handle));
    
    // Send success message
    Message msg(MessageType::ArtboardCreated, cmd.requestID);
    msg.handle = handle;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleCreateArtboardByName(const Command& cmd)
{
    LOGI("CommandServer: Handling CreateArtboardByName command (requestID=%lld, fileHandle=%lld, name=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.name.c_str());
    
    auto it = m_files.find(cmd.handle);
    if (it == m_files.end()) {
        LOGW("CommandServer: Invalid file handle: %lld", static_cast<long long>(cmd.handle));
        
        Message msg(MessageType::ArtboardError, cmd.requestID);
        msg.error = "Invalid file handle";
        enqueueMessage(std::move(msg));
        return;
    }
    
    // Create the artboard by name (returns unique_ptr<ArtboardInstance>)
    auto artboard = it->second->artboardNamed(cmd.name);
    if (!artboard) {
        LOGW("CommandServer: Failed to create artboard with name: %s", cmd.name.c_str());
        
        Message msg(MessageType::ArtboardError, cmd.requestID);
        msg.error = "Artboard not found: " + cmd.name;
        enqueueMessage(std::move(msg));
        return;
    }
    
    // Generate a unique handle
    int64_t handle = m_nextHandle.fetch_add(1);
    
    // Store the artboard
    m_artboards[handle] = std::move(artboard);
    
    LOGI("CommandServer: Artboard created successfully (handle=%lld, name=%s)", 
         static_cast<long long>(handle), cmd.name.c_str());
    
    // Send success message
    Message msg(MessageType::ArtboardCreated, cmd.requestID);
    msg.handle = handle;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleDeleteArtboard(const Command& cmd)
{
    LOGI("CommandServer: Handling DeleteArtboard command (requestID=%lld, handle=%lld)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle));
    
    auto it = m_artboards.find(cmd.handle);
    if (it != m_artboards.end()) {
        m_artboards.erase(it);
        
        LOGI("CommandServer: Artboard deleted successfully (handle=%lld)", 
             static_cast<long long>(cmd.handle));
        
        // Send success message
        Message msg(MessageType::ArtboardDeleted, cmd.requestID);
        msg.handle = cmd.handle;
        enqueueMessage(std::move(msg));
    } else {
        LOGW("CommandServer: Attempted to delete non-existent artboard (handle=%lld)",
             static_cast<long long>(cmd.handle));
        
        // Send error message
        Message msg(MessageType::ArtboardError, cmd.requestID);
        msg.error = "Invalid artboard handle";
        enqueueMessage(std::move(msg));
    }
}

void CommandServer::createDefaultStateMachine(int64_t requestID, int64_t artboardHandle)
{
    LOGI("CommandServer: Enqueuing CreateDefaultStateMachine command (requestID=%lld, artboardHandle=%lld)",
         static_cast<long long>(requestID), static_cast<long long>(artboardHandle));
    
    Command cmd(CommandType::CreateDefaultStateMachine, requestID);
    cmd.handle = artboardHandle;
    
    enqueueCommand(std::move(cmd));
}

void CommandServer::createStateMachineByName(int64_t requestID, int64_t artboardHandle, const std::string& name)
{
    LOGI("CommandServer: Enqueuing CreateStateMachineByName command (requestID=%lld, artboardHandle=%lld, name=%s)",
         static_cast<long long>(requestID), static_cast<long long>(artboardHandle), name.c_str());
    
    Command cmd(CommandType::CreateStateMachineByName, requestID);
    cmd.handle = artboardHandle;
    cmd.name = name;
    
    enqueueCommand(std::move(cmd));
}

void CommandServer::advanceStateMachine(int64_t requestID, int64_t smHandle, float deltaTime)
{
    LOGI("CommandServer: Enqueuing AdvanceStateMachine command (requestID=%lld, smHandle=%lld, deltaTime=%f)",
         static_cast<long long>(requestID), static_cast<long long>(smHandle), deltaTime);
    
    Command cmd(CommandType::AdvanceStateMachine, requestID);
    cmd.handle = smHandle;
    cmd.deltaTime = deltaTime;
    
    enqueueCommand(std::move(cmd));
}

void CommandServer::deleteStateMachine(int64_t requestID, int64_t smHandle)
{
    LOGI("CommandServer: Enqueuing DeleteStateMachine command (requestID=%lld, smHandle=%lld)",
         static_cast<long long>(requestID), static_cast<long long>(smHandle));
    
    Command cmd(CommandType::DeleteStateMachine, requestID);
    cmd.handle = smHandle;
    
    enqueueCommand(std::move(cmd));
}

void CommandServer::handleCreateDefaultStateMachine(const Command& cmd)
{
    LOGI("CommandServer: Handling CreateDefaultStateMachine command (requestID=%lld, artboardHandle=%lld)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle));
    
    auto it = m_artboards.find(cmd.handle);
    if (it == m_artboards.end()) {
        LOGW("CommandServer: Invalid artboard handle: %lld", static_cast<long long>(cmd.handle));
        
        Message msg(MessageType::StateMachineError, cmd.requestID);
        msg.error = "Invalid artboard handle";
        enqueueMessage(std::move(msg));
        return;
    }
    
    // Create the default state machine
    auto sm = it->second->defaultStateMachine();
    if (!sm) {
        LOGW("CommandServer: Failed to create default state machine");
        
        Message msg(MessageType::StateMachineError, cmd.requestID);
        msg.error = "Failed to create default state machine";
        enqueueMessage(std::move(msg));
        return;
    }
    
    // Generate a unique handle
    int64_t handle = m_nextHandle.fetch_add(1);
    
    // Store the state machine
    m_stateMachines[handle] = std::move(sm);
    
    LOGI("CommandServer: State machine created successfully (handle=%lld)", 
         static_cast<long long>(handle));
    
    // Send success message
    Message msg(MessageType::StateMachineCreated, cmd.requestID);
    msg.handle = handle;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleCreateStateMachineByName(const Command& cmd)
{
    LOGI("CommandServer: Handling CreateStateMachineByName command (requestID=%lld, artboardHandle=%lld, name=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.name.c_str());
    
    auto it = m_artboards.find(cmd.handle);
    if (it == m_artboards.end()) {
        LOGW("CommandServer: Invalid artboard handle: %lld", static_cast<long long>(cmd.handle));
        
        Message msg(MessageType::StateMachineError, cmd.requestID);
        msg.error = "Invalid artboard handle";
        enqueueMessage(std::move(msg));
        return;
    }
    
    // Create the state machine by name
    auto sm = it->second->stateMachineNamed(cmd.name);
    if (!sm) {
        LOGW("CommandServer: Failed to create state machine with name: %s", cmd.name.c_str());
        
        Message msg(MessageType::StateMachineError, cmd.requestID);
        msg.error = "State machine not found: " + cmd.name;
        enqueueMessage(std::move(msg));
        return;
    }
    
    // Generate a unique handle
    int64_t handle = m_nextHandle.fetch_add(1);
    
    // Store the state machine
    m_stateMachines[handle] = std::move(sm);
    
    LOGI("CommandServer: State machine created successfully (handle=%lld, name=%s)", 
         static_cast<long long>(handle), cmd.name.c_str());
    
    // Send success message
    Message msg(MessageType::StateMachineCreated, cmd.requestID);
    msg.handle = handle;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleAdvanceStateMachine(const Command& cmd)
{
    LOGI("CommandServer: Handling AdvanceStateMachine command (requestID=%lld, smHandle=%lld, deltaTime=%f)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.deltaTime);
    
    auto it = m_stateMachines.find(cmd.handle);
    if (it == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));
        
        Message msg(MessageType::StateMachineError, cmd.requestID);
        msg.error = "Invalid state machine handle";
        enqueueMessage(std::move(msg));
        return;
    }
    
    // Advance the state machine
    it->second->advance(cmd.deltaTime);
    
    // Check if the state machine has settled
    bool settled = !it->second->needsAdvance();
    
    LOGI("CommandServer: State machine advanced (handle=%lld, settled=%d)", 
         static_cast<long long>(cmd.handle), settled);
    
    // If settled, send settled message
    if (settled) {
        Message msg(MessageType::StateMachineSettled, cmd.requestID);
        msg.handle = cmd.handle;
        enqueueMessage(std::move(msg));
    }
}

void CommandServer::handleDeleteStateMachine(const Command& cmd)
{
    LOGI("CommandServer: Handling DeleteStateMachine command (requestID=%lld, handle=%lld)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle));

    auto it = m_stateMachines.find(cmd.handle);
    if (it != m_stateMachines.end()) {
        m_stateMachines.erase(it);

        LOGI("CommandServer: State machine deleted successfully (handle=%lld)",
             static_cast<long long>(cmd.handle));

        // Send success message
        Message msg(MessageType::StateMachineDeleted, cmd.requestID);
        msg.handle = cmd.handle;
        enqueueMessage(std::move(msg));
    } else {
        LOGW("CommandServer: Attempted to delete non-existent state machine (handle=%lld)",
             static_cast<long long>(cmd.handle));

        // Send error message
        Message msg(MessageType::StateMachineError, cmd.requestID);
        msg.error = "Invalid state machine handle";
        enqueueMessage(std::move(msg));
    }
}

// =============================================================================
// Phase C.4: State Machine Input Operations
// =============================================================================

void CommandServer::getInputCount(int64_t requestID, int64_t smHandle)
{
    LOGI("CommandServer: Enqueuing GetInputCount command (requestID=%lld, smHandle=%lld)",
         static_cast<long long>(requestID), static_cast<long long>(smHandle));

    Command cmd(CommandType::GetInputCount, requestID);
    cmd.handle = smHandle;

    enqueueCommand(std::move(cmd));
}

void CommandServer::getInputNames(int64_t requestID, int64_t smHandle)
{
    LOGI("CommandServer: Enqueuing GetInputNames command (requestID=%lld, smHandle=%lld)",
         static_cast<long long>(requestID), static_cast<long long>(smHandle));

    Command cmd(CommandType::GetInputNames, requestID);
    cmd.handle = smHandle;

    enqueueCommand(std::move(cmd));
}

void CommandServer::getInputInfo(int64_t requestID, int64_t smHandle, int32_t inputIndex)
{
    LOGI("CommandServer: Enqueuing GetInputInfo command (requestID=%lld, smHandle=%lld, index=%d)",
         static_cast<long long>(requestID), static_cast<long long>(smHandle), inputIndex);

    Command cmd(CommandType::GetInputInfo, requestID);
    cmd.handle = smHandle;
    cmd.inputIndex = inputIndex;

    enqueueCommand(std::move(cmd));
}

void CommandServer::getNumberInput(int64_t requestID, int64_t smHandle, const std::string& inputName)
{
    LOGI("CommandServer: Enqueuing GetNumberInput command (requestID=%lld, smHandle=%lld, name=%s)",
         static_cast<long long>(requestID), static_cast<long long>(smHandle), inputName.c_str());

    Command cmd(CommandType::GetNumberInput, requestID);
    cmd.handle = smHandle;
    cmd.inputName = inputName;

    enqueueCommand(std::move(cmd));
}

void CommandServer::setNumberInput(int64_t requestID, int64_t smHandle, const std::string& inputName, float value)
{
    LOGI("CommandServer: Enqueuing SetNumberInput command (requestID=%lld, smHandle=%lld, name=%s, value=%f)",
         static_cast<long long>(requestID), static_cast<long long>(smHandle), inputName.c_str(), value);

    Command cmd(CommandType::SetNumberInput, requestID);
    cmd.handle = smHandle;
    cmd.inputName = inputName;
    cmd.floatValue = value;

    enqueueCommand(std::move(cmd));
}

void CommandServer::getBooleanInput(int64_t requestID, int64_t smHandle, const std::string& inputName)
{
    LOGI("CommandServer: Enqueuing GetBooleanInput command (requestID=%lld, smHandle=%lld, name=%s)",
         static_cast<long long>(requestID), static_cast<long long>(smHandle), inputName.c_str());

    Command cmd(CommandType::GetBooleanInput, requestID);
    cmd.handle = smHandle;
    cmd.inputName = inputName;

    enqueueCommand(std::move(cmd));
}

void CommandServer::setBooleanInput(int64_t requestID, int64_t smHandle, const std::string& inputName, bool value)
{
    LOGI("CommandServer: Enqueuing SetBooleanInput command (requestID=%lld, smHandle=%lld, name=%s, value=%d)",
         static_cast<long long>(requestID), static_cast<long long>(smHandle), inputName.c_str(), value);

    Command cmd(CommandType::SetBooleanInput, requestID);
    cmd.handle = smHandle;
    cmd.inputName = inputName;
    cmd.boolValue = value;

    enqueueCommand(std::move(cmd));
}

void CommandServer::fireTrigger(int64_t requestID, int64_t smHandle, const std::string& inputName)
{
    LOGI("CommandServer: Enqueuing FireTrigger command (requestID=%lld, smHandle=%lld, name=%s)",
         static_cast<long long>(requestID), static_cast<long long>(smHandle), inputName.c_str());

    Command cmd(CommandType::FireTrigger, requestID);
    cmd.handle = smHandle;
    cmd.inputName = inputName;

    enqueueCommand(std::move(cmd));
}

// =============================================================================
// Input Handler Implementations
// =============================================================================

void CommandServer::handleGetInputCount(const Command& cmd)
{
    LOGI("CommandServer: Handling GetInputCount command (requestID=%lld, smHandle=%lld)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle));

    auto it = m_stateMachines.find(cmd.handle);
    if (it == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Invalid state machine handle";
        enqueueMessage(std::move(msg));
        return;
    }

    int32_t count = static_cast<int32_t>(it->second->inputCount());

    LOGI("CommandServer: Input count: %d", count);

    Message msg(MessageType::InputCountResult, cmd.requestID);
    msg.intValue = count;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleGetInputNames(const Command& cmd)
{
    LOGI("CommandServer: Handling GetInputNames command (requestID=%lld, smHandle=%lld)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle));

    auto it = m_stateMachines.find(cmd.handle);
    if (it == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Invalid state machine handle";
        enqueueMessage(std::move(msg));
        return;
    }

    std::vector<std::string> names;
    auto& sm = it->second;

    for (size_t i = 0; i < sm->inputCount(); i++) {
        auto input = sm->input(i);
        if (input) {
            names.push_back(input->name());
        }
    }

    LOGI("CommandServer: Found %zu input names", names.size());

    Message msg(MessageType::InputNamesListed, cmd.requestID);
    msg.stringList = std::move(names);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleGetInputInfo(const Command& cmd)
{
    LOGI("CommandServer: Handling GetInputInfo command (requestID=%lld, smHandle=%lld, index=%d)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.inputIndex);

    auto it = m_stateMachines.find(cmd.handle);
    if (it == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Invalid state machine handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& sm = it->second;
    if (cmd.inputIndex < 0 || static_cast<size_t>(cmd.inputIndex) >= sm->inputCount()) {
        LOGW("CommandServer: Input index out of bounds: %d (count=%zu)", cmd.inputIndex, sm->inputCount());

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Input index out of bounds";
        enqueueMessage(std::move(msg));
        return;
    }

    auto input = sm->input(cmd.inputIndex);
    if (!input) {
        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Failed to get input at index";
        enqueueMessage(std::move(msg));
        return;
    }

    // Determine input type by checking the underlying StateMachineInput type
    InputType inputType = InputType::UNKNOWN;
    if (input->input()->is<rive::StateMachineNumber>()) {
        inputType = InputType::NUMBER;
    } else if (input->input()->is<rive::StateMachineBool>()) {
        inputType = InputType::BOOLEAN;
    } else if (input->input()->is<rive::StateMachineTrigger>()) {
        inputType = InputType::TRIGGER;
    }

    LOGI("CommandServer: Input info - name=%s, type=%d", input->name().c_str(), static_cast<int>(inputType));

    Message msg(MessageType::InputInfoResult, cmd.requestID);
    msg.inputName = input->name();
    msg.inputType = inputType;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleGetNumberInput(const Command& cmd)
{
    LOGI("CommandServer: Handling GetNumberInput command (requestID=%lld, smHandle=%lld, name=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.inputName.c_str());

    auto it = m_stateMachines.find(cmd.handle);
    if (it == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Invalid state machine handle";
        enqueueMessage(std::move(msg));
        return;
    }

    // Find the input by name
    auto& sm = it->second;
    rive::SMIInput* foundInput = nullptr;
    for (size_t i = 0; i < sm->inputCount(); i++) {
        auto input = sm->input(i);
        if (input && input->name() == cmd.inputName) {
            foundInput = input;
            break;
        }
    }

    if (!foundInput) {
        LOGW("CommandServer: Input not found: %s", cmd.inputName.c_str());

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Input not found: " + cmd.inputName;
        enqueueMessage(std::move(msg));
        return;
    }

    if (!foundInput->input()->is<rive::StateMachineNumber>()) {
        LOGW("CommandServer: Input is not a number: %s", cmd.inputName.c_str());

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Input is not a number: " + cmd.inputName;
        enqueueMessage(std::move(msg));
        return;
    }

    auto numberInput = reinterpret_cast<rive::SMINumber*>(foundInput);
    float value = numberInput->value();

    LOGI("CommandServer: Number input value: %f", value);

    Message msg(MessageType::NumberInputValue, cmd.requestID);
    msg.floatValue = value;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleSetNumberInput(const Command& cmd)
{
    LOGI("CommandServer: Handling SetNumberInput command (requestID=%lld, smHandle=%lld, name=%s, value=%f)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.inputName.c_str(), cmd.floatValue);

    auto it = m_stateMachines.find(cmd.handle);
    if (it == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Invalid state machine handle";
        enqueueMessage(std::move(msg));
        return;
    }

    // Find the input by name
    auto& sm = it->second;
    rive::SMIInput* foundInput = nullptr;
    for (size_t i = 0; i < sm->inputCount(); i++) {
        auto input = sm->input(i);
        if (input && input->name() == cmd.inputName) {
            foundInput = input;
            break;
        }
    }

    if (!foundInput) {
        LOGW("CommandServer: Input not found: %s", cmd.inputName.c_str());

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Input not found: " + cmd.inputName;
        enqueueMessage(std::move(msg));
        return;
    }

    if (!foundInput->input()->is<rive::StateMachineNumber>()) {
        LOGW("CommandServer: Input is not a number: %s", cmd.inputName.c_str());

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Input is not a number: " + cmd.inputName;
        enqueueMessage(std::move(msg));
        return;
    }

    auto numberInput = reinterpret_cast<rive::SMINumber*>(foundInput);
    numberInput->value(cmd.floatValue);

    LOGI("CommandServer: Number input set to: %f", cmd.floatValue);

    Message msg(MessageType::InputOperationSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleGetBooleanInput(const Command& cmd)
{
    LOGI("CommandServer: Handling GetBooleanInput command (requestID=%lld, smHandle=%lld, name=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.inputName.c_str());

    auto it = m_stateMachines.find(cmd.handle);
    if (it == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Invalid state machine handle";
        enqueueMessage(std::move(msg));
        return;
    }

    // Find the input by name
    auto& sm = it->second;
    rive::SMIInput* foundInput = nullptr;
    for (size_t i = 0; i < sm->inputCount(); i++) {
        auto input = sm->input(i);
        if (input && input->name() == cmd.inputName) {
            foundInput = input;
            break;
        }
    }

    if (!foundInput) {
        LOGW("CommandServer: Input not found: %s", cmd.inputName.c_str());

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Input not found: " + cmd.inputName;
        enqueueMessage(std::move(msg));
        return;
    }

    if (!foundInput->input()->is<rive::StateMachineBool>()) {
        LOGW("CommandServer: Input is not a boolean: %s", cmd.inputName.c_str());

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Input is not a boolean: " + cmd.inputName;
        enqueueMessage(std::move(msg));
        return;
    }

    auto boolInput = reinterpret_cast<rive::SMIBool*>(foundInput);
    bool value = boolInput->value();

    LOGI("CommandServer: Boolean input value: %d", value);

    Message msg(MessageType::BooleanInputValue, cmd.requestID);
    msg.boolValue = value;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleSetBooleanInput(const Command& cmd)
{
    LOGI("CommandServer: Handling SetBooleanInput command (requestID=%lld, smHandle=%lld, name=%s, value=%d)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.inputName.c_str(), cmd.boolValue);

    auto it = m_stateMachines.find(cmd.handle);
    if (it == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Invalid state machine handle";
        enqueueMessage(std::move(msg));
        return;
    }

    // Find the input by name
    auto& sm = it->second;
    rive::SMIInput* foundInput = nullptr;
    for (size_t i = 0; i < sm->inputCount(); i++) {
        auto input = sm->input(i);
        if (input && input->name() == cmd.inputName) {
            foundInput = input;
            break;
        }
    }

    if (!foundInput) {
        LOGW("CommandServer: Input not found: %s", cmd.inputName.c_str());

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Input not found: " + cmd.inputName;
        enqueueMessage(std::move(msg));
        return;
    }

    if (!foundInput->input()->is<rive::StateMachineBool>()) {
        LOGW("CommandServer: Input is not a boolean: %s", cmd.inputName.c_str());

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Input is not a boolean: " + cmd.inputName;
        enqueueMessage(std::move(msg));
        return;
    }

    auto boolInput = reinterpret_cast<rive::SMIBool*>(foundInput);
    boolInput->value(cmd.boolValue);

    LOGI("CommandServer: Boolean input set to: %d", cmd.boolValue);

    Message msg(MessageType::InputOperationSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleFireTrigger(const Command& cmd)
{
    LOGI("CommandServer: Handling FireTrigger command (requestID=%lld, smHandle=%lld, name=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.inputName.c_str());

    auto it = m_stateMachines.find(cmd.handle);
    if (it == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Invalid state machine handle";
        enqueueMessage(std::move(msg));
        return;
    }

    // Find the input by name
    auto& sm = it->second;
    rive::SMIInput* foundInput = nullptr;
    for (size_t i = 0; i < sm->inputCount(); i++) {
        auto input = sm->input(i);
        if (input && input->name() == cmd.inputName) {
            foundInput = input;
            break;
        }
    }

    if (!foundInput) {
        LOGW("CommandServer: Input not found: %s", cmd.inputName.c_str());

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Input not found: " + cmd.inputName;
        enqueueMessage(std::move(msg));
        return;
    }

    if (!foundInput->input()->is<rive::StateMachineTrigger>()) {
        LOGW("CommandServer: Input is not a trigger: %s", cmd.inputName.c_str());

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Input is not a trigger: " + cmd.inputName;
        enqueueMessage(std::move(msg));
        return;
    }

    auto triggerInput = reinterpret_cast<rive::SMITrigger*>(foundInput);
    triggerInput->fire();

    LOGI("CommandServer: Trigger fired: %s", cmd.inputName.c_str());

    Message msg(MessageType::InputOperationSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

// =============================================================================
// Phase D: View Model Instance Operations
// =============================================================================

void CommandServer::createBlankVMI(int64_t requestID, int64_t fileHandle, const std::string& viewModelName)
{
    LOGI("CommandServer: Enqueuing CreateBlankVMI command (requestID=%lld, fileHandle=%lld, vmName=%s)",
         static_cast<long long>(requestID), static_cast<long long>(fileHandle), viewModelName.c_str());

    Command cmd(CommandType::CreateBlankVMI, requestID);
    cmd.handle = fileHandle;
    cmd.viewModelName = viewModelName;

    enqueueCommand(std::move(cmd));
}

void CommandServer::createDefaultVMI(int64_t requestID, int64_t fileHandle, const std::string& viewModelName)
{
    LOGI("CommandServer: Enqueuing CreateDefaultVMI command (requestID=%lld, fileHandle=%lld, vmName=%s)",
         static_cast<long long>(requestID), static_cast<long long>(fileHandle), viewModelName.c_str());

    Command cmd(CommandType::CreateDefaultVMI, requestID);
    cmd.handle = fileHandle;
    cmd.viewModelName = viewModelName;

    enqueueCommand(std::move(cmd));
}

void CommandServer::createNamedVMI(int64_t requestID, int64_t fileHandle,
                                    const std::string& viewModelName, const std::string& instanceName)
{
    LOGI("CommandServer: Enqueuing CreateNamedVMI command (requestID=%lld, fileHandle=%lld, vmName=%s, instName=%s)",
         static_cast<long long>(requestID), static_cast<long long>(fileHandle),
         viewModelName.c_str(), instanceName.c_str());

    Command cmd(CommandType::CreateNamedVMI, requestID);
    cmd.handle = fileHandle;
    cmd.viewModelName = viewModelName;
    cmd.instanceName = instanceName;

    enqueueCommand(std::move(cmd));
}

void CommandServer::deleteVMI(int64_t requestID, int64_t vmiHandle)
{
    LOGI("CommandServer: Enqueuing DeleteVMI command (requestID=%lld, vmiHandle=%lld)",
         static_cast<long long>(requestID), static_cast<long long>(vmiHandle));

    Command cmd(CommandType::DeleteVMI, requestID);
    cmd.handle = vmiHandle;

    enqueueCommand(std::move(cmd));
}

// =============================================================================
// View Model Instance Handler Implementations
// =============================================================================

void CommandServer::handleCreateBlankVMI(const Command& cmd)
{
    LOGI("CommandServer: Handling CreateBlankVMI command (requestID=%lld, fileHandle=%lld, vmName=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.viewModelName.c_str());

    auto it = m_files.find(cmd.handle);
    if (it == m_files.end()) {
        LOGW("CommandServer: Invalid file handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::VMIError, cmd.requestID);
        msg.error = "Invalid file handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& file = it->second;

    // Get the ViewModelRuntime by name
    auto* vmRuntime = file->viewModelByName(cmd.viewModelName);
    if (!vmRuntime) {
        LOGW("CommandServer: ViewModel not found: %s", cmd.viewModelName.c_str());

        Message msg(MessageType::VMIError, cmd.requestID);
        msg.error = "ViewModel not found: " + cmd.viewModelName;
        enqueueMessage(std::move(msg));
        return;
    }

    // Create a blank instance (createInstance returns blank)
    auto instance = vmRuntime->createInstance();
    if (!instance) {
        LOGW("CommandServer: Failed to create blank VMI");

        Message msg(MessageType::VMIError, cmd.requestID);
        msg.error = "Failed to create blank ViewModelInstance";
        enqueueMessage(std::move(msg));
        return;
    }

    // Generate a unique handle
    int64_t handle = m_nextHandle.fetch_add(1);

    // Store the instance
    m_viewModelInstances[handle] = instance;

    LOGI("CommandServer: Blank VMI created successfully (handle=%lld, vmName=%s)",
         static_cast<long long>(handle), cmd.viewModelName.c_str());

    // Send success message
    Message msg(MessageType::VMICreated, cmd.requestID);
    msg.handle = handle;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleCreateDefaultVMI(const Command& cmd)
{
    LOGI("CommandServer: Handling CreateDefaultVMI command (requestID=%lld, fileHandle=%lld, vmName=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.viewModelName.c_str());

    auto it = m_files.find(cmd.handle);
    if (it == m_files.end()) {
        LOGW("CommandServer: Invalid file handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::VMIError, cmd.requestID);
        msg.error = "Invalid file handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& file = it->second;

    // Get the ViewModelRuntime by name
    auto* vmRuntime = file->viewModelByName(cmd.viewModelName);
    if (!vmRuntime) {
        LOGW("CommandServer: ViewModel not found: %s", cmd.viewModelName.c_str());

        Message msg(MessageType::VMIError, cmd.requestID);
        msg.error = "ViewModel not found: " + cmd.viewModelName;
        enqueueMessage(std::move(msg));
        return;
    }

    // Create the default instance
    auto instance = vmRuntime->createDefaultInstance();
    if (!instance) {
        LOGW("CommandServer: Failed to create default VMI");

        Message msg(MessageType::VMIError, cmd.requestID);
        msg.error = "Failed to create default ViewModelInstance";
        enqueueMessage(std::move(msg));
        return;
    }

    // Generate a unique handle
    int64_t handle = m_nextHandle.fetch_add(1);

    // Store the instance
    m_viewModelInstances[handle] = instance;

    LOGI("CommandServer: Default VMI created successfully (handle=%lld, vmName=%s)",
         static_cast<long long>(handle), cmd.viewModelName.c_str());

    // Send success message
    Message msg(MessageType::VMICreated, cmd.requestID);
    msg.handle = handle;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleCreateNamedVMI(const Command& cmd)
{
    LOGI("CommandServer: Handling CreateNamedVMI command (requestID=%lld, fileHandle=%lld, vmName=%s, instName=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle),
         cmd.viewModelName.c_str(), cmd.instanceName.c_str());

    auto it = m_files.find(cmd.handle);
    if (it == m_files.end()) {
        LOGW("CommandServer: Invalid file handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::VMIError, cmd.requestID);
        msg.error = "Invalid file handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& file = it->second;

    // Get the ViewModelRuntime by name
    auto* vmRuntime = file->viewModelByName(cmd.viewModelName);
    if (!vmRuntime) {
        LOGW("CommandServer: ViewModel not found: %s", cmd.viewModelName.c_str());

        Message msg(MessageType::VMIError, cmd.requestID);
        msg.error = "ViewModel not found: " + cmd.viewModelName;
        enqueueMessage(std::move(msg));
        return;
    }

    // Create instance by name
    auto instance = vmRuntime->createInstanceFromName(cmd.instanceName);
    if (!instance) {
        LOGW("CommandServer: Instance not found: %s", cmd.instanceName.c_str());

        Message msg(MessageType::VMIError, cmd.requestID);
        msg.error = "Instance not found: " + cmd.instanceName;
        enqueueMessage(std::move(msg));
        return;
    }

    // Generate a unique handle
    int64_t handle = m_nextHandle.fetch_add(1);

    // Store the instance
    m_viewModelInstances[handle] = instance;

    LOGI("CommandServer: Named VMI created successfully (handle=%lld, vmName=%s, instName=%s)",
         static_cast<long long>(handle), cmd.viewModelName.c_str(), cmd.instanceName.c_str());

    // Send success message
    Message msg(MessageType::VMICreated, cmd.requestID);
    msg.handle = handle;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleDeleteVMI(const Command& cmd)
{
    LOGI("CommandServer: Handling DeleteVMI command (requestID=%lld, handle=%lld)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle));

    auto it = m_viewModelInstances.find(cmd.handle);
    if (it != m_viewModelInstances.end()) {
        m_viewModelInstances.erase(it);

        LOGI("CommandServer: VMI deleted successfully (handle=%lld)",
             static_cast<long long>(cmd.handle));

        // Send success message
        Message msg(MessageType::VMIDeleted, cmd.requestID);
        msg.handle = cmd.handle;
        enqueueMessage(std::move(msg));
    } else {
        LOGW("CommandServer: Attempted to delete non-existent VMI (handle=%lld)",
             static_cast<long long>(cmd.handle));

        // Send error message
        Message msg(MessageType::VMIError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
    }
}

// =============================================================================
// Property Operations - Public API (Phase D.2)
// =============================================================================

void CommandServer::getNumberProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath)
{
    LOGI("CommandServer: Enqueuing GetNumberProperty command (requestID=%lld, vmiHandle=%lld, path=%s)",
         static_cast<long long>(requestID), static_cast<long long>(vmiHandle), propertyPath.c_str());

    Command cmd(CommandType::GetNumberProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;

    enqueueCommand(std::move(cmd));
}

void CommandServer::setNumberProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, float value)
{
    LOGI("CommandServer: Enqueuing SetNumberProperty command (requestID=%lld, vmiHandle=%lld, path=%s, value=%f)",
         static_cast<long long>(requestID), static_cast<long long>(vmiHandle), propertyPath.c_str(), value);

    Command cmd(CommandType::SetNumberProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.floatValue = value;

    enqueueCommand(std::move(cmd));
}

void CommandServer::getStringProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath)
{
    LOGI("CommandServer: Enqueuing GetStringProperty command (requestID=%lld, vmiHandle=%lld, path=%s)",
         static_cast<long long>(requestID), static_cast<long long>(vmiHandle), propertyPath.c_str());

    Command cmd(CommandType::GetStringProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;

    enqueueCommand(std::move(cmd));
}

void CommandServer::setStringProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, const std::string& value)
{
    LOGI("CommandServer: Enqueuing SetStringProperty command (requestID=%lld, vmiHandle=%lld, path=%s, value=%s)",
         static_cast<long long>(requestID), static_cast<long long>(vmiHandle), propertyPath.c_str(), value.c_str());

    Command cmd(CommandType::SetStringProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.stringValue = value;

    enqueueCommand(std::move(cmd));
}

void CommandServer::getBooleanProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath)
{
    LOGI("CommandServer: Enqueuing GetBooleanProperty command (requestID=%lld, vmiHandle=%lld, path=%s)",
         static_cast<long long>(requestID), static_cast<long long>(vmiHandle), propertyPath.c_str());

    Command cmd(CommandType::GetBooleanProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;

    enqueueCommand(std::move(cmd));
}

void CommandServer::setBooleanProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, bool value)
{
    LOGI("CommandServer: Enqueuing SetBooleanProperty command (requestID=%lld, vmiHandle=%lld, path=%s, value=%d)",
         static_cast<long long>(requestID), static_cast<long long>(vmiHandle), propertyPath.c_str(), value ? 1 : 0);

    Command cmd(CommandType::SetBooleanProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.boolValue = value;

    enqueueCommand(std::move(cmd));
}

// =============================================================================
// Property Operations - Handler Implementations (Phase D.2)
// =============================================================================

void CommandServer::handleGetNumberProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling GetNumberProperty command (requestID=%lld, vmiHandle=%lld, path=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.propertyPath.c_str());

    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        LOGW("CommandServer: Invalid VMI handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& vmi = it->second;
    auto* prop = vmi->propertyNumber(cmd.propertyPath);
    if (!prop) {
        LOGW("CommandServer: Number property not found: %s", cmd.propertyPath.c_str());

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Number property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    float value = prop->value();

    LOGI("CommandServer: GetNumberProperty succeeded (path=%s, value=%f)",
         cmd.propertyPath.c_str(), value);

    Message msg(MessageType::NumberPropertyValue, cmd.requestID);
    msg.floatValue = value;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleSetNumberProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling SetNumberProperty command (requestID=%lld, vmiHandle=%lld, path=%s, value=%f)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle),
         cmd.propertyPath.c_str(), cmd.floatValue);

    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        LOGW("CommandServer: Invalid VMI handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& vmi = it->second;
    auto* prop = vmi->propertyNumber(cmd.propertyPath);
    if (!prop) {
        LOGW("CommandServer: Number property not found: %s", cmd.propertyPath.c_str());

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Number property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    prop->value(cmd.floatValue);

    LOGI("CommandServer: SetNumberProperty succeeded (path=%s, value=%f)",
         cmd.propertyPath.c_str(), cmd.floatValue);

    // Emit update to subscribers (Phase D.4)
    emitPropertyUpdateIfSubscribed(cmd.handle, cmd.propertyPath, PropertyDataType::NUMBER);

    Message msg(MessageType::PropertySetSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleGetStringProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling GetStringProperty command (requestID=%lld, vmiHandle=%lld, path=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.propertyPath.c_str());

    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        LOGW("CommandServer: Invalid VMI handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& vmi = it->second;
    auto* prop = vmi->propertyString(cmd.propertyPath);
    if (!prop) {
        LOGW("CommandServer: String property not found: %s", cmd.propertyPath.c_str());

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "String property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    const std::string& value = prop->value();

    LOGI("CommandServer: GetStringProperty succeeded (path=%s, value=%s)",
         cmd.propertyPath.c_str(), value.c_str());

    Message msg(MessageType::StringPropertyValue, cmd.requestID);
    msg.stringValue = value;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleSetStringProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling SetStringProperty command (requestID=%lld, vmiHandle=%lld, path=%s, value=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle),
         cmd.propertyPath.c_str(), cmd.stringValue.c_str());

    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        LOGW("CommandServer: Invalid VMI handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& vmi = it->second;
    auto* prop = vmi->propertyString(cmd.propertyPath);
    if (!prop) {
        LOGW("CommandServer: String property not found: %s", cmd.propertyPath.c_str());

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "String property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    prop->value(cmd.stringValue);

    LOGI("CommandServer: SetStringProperty succeeded (path=%s, value=%s)",
         cmd.propertyPath.c_str(), cmd.stringValue.c_str());

    // Emit update to subscribers (Phase D.4)
    emitPropertyUpdateIfSubscribed(cmd.handle, cmd.propertyPath, PropertyDataType::STRING);

    Message msg(MessageType::PropertySetSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleGetBooleanProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling GetBooleanProperty command (requestID=%lld, vmiHandle=%lld, path=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.propertyPath.c_str());

    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        LOGW("CommandServer: Invalid VMI handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& vmi = it->second;
    auto* prop = vmi->propertyBoolean(cmd.propertyPath);
    if (!prop) {
        LOGW("CommandServer: Boolean property not found: %s", cmd.propertyPath.c_str());

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Boolean property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    bool value = prop->value();

    LOGI("CommandServer: GetBooleanProperty succeeded (path=%s, value=%d)",
         cmd.propertyPath.c_str(), value ? 1 : 0);

    Message msg(MessageType::BooleanPropertyValue, cmd.requestID);
    msg.boolValue = value;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleSetBooleanProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling SetBooleanProperty command (requestID=%lld, vmiHandle=%lld, path=%s, value=%d)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle),
         cmd.propertyPath.c_str(), cmd.boolValue ? 1 : 0);

    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        LOGW("CommandServer: Invalid VMI handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& vmi = it->second;
    auto* prop = vmi->propertyBoolean(cmd.propertyPath);
    if (!prop) {
        LOGW("CommandServer: Boolean property not found: %s", cmd.propertyPath.c_str());

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Boolean property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    prop->value(cmd.boolValue);

    LOGI("CommandServer: SetBooleanProperty succeeded (path=%s, value=%d)",
         cmd.propertyPath.c_str(), cmd.boolValue ? 1 : 0);

    // Emit update to subscribers (Phase D.4)
    emitPropertyUpdateIfSubscribed(cmd.handle, cmd.propertyPath, PropertyDataType::BOOLEAN);

    Message msg(MessageType::PropertySetSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

// =============================================================================
// Additional Property Types - Public API (Phase D.3)
// =============================================================================

void CommandServer::getEnumProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath)
{
    LOGI("CommandServer: Enqueuing GetEnumProperty command (requestID=%lld, vmiHandle=%lld, path=%s)",
         static_cast<long long>(requestID), static_cast<long long>(vmiHandle), propertyPath.c_str());

    Command cmd(CommandType::GetEnumProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;

    enqueueCommand(std::move(cmd));
}

void CommandServer::setEnumProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, const std::string& value)
{
    LOGI("CommandServer: Enqueuing SetEnumProperty command (requestID=%lld, vmiHandle=%lld, path=%s, value=%s)",
         static_cast<long long>(requestID), static_cast<long long>(vmiHandle), propertyPath.c_str(), value.c_str());

    Command cmd(CommandType::SetEnumProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.stringValue = value;

    enqueueCommand(std::move(cmd));
}

void CommandServer::getColorProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath)
{
    LOGI("CommandServer: Enqueuing GetColorProperty command (requestID=%lld, vmiHandle=%lld, path=%s)",
         static_cast<long long>(requestID), static_cast<long long>(vmiHandle), propertyPath.c_str());

    Command cmd(CommandType::GetColorProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;

    enqueueCommand(std::move(cmd));
}

void CommandServer::setColorProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int32_t value)
{
    LOGI("CommandServer: Enqueuing SetColorProperty command (requestID=%lld, vmiHandle=%lld, path=%s, value=0x%08X)",
         static_cast<long long>(requestID), static_cast<long long>(vmiHandle), propertyPath.c_str(), value);

    Command cmd(CommandType::SetColorProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.colorValue = value;

    enqueueCommand(std::move(cmd));
}

void CommandServer::fireTriggerProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath)
{
    LOGI("CommandServer: Enqueuing FireTriggerProperty command (requestID=%lld, vmiHandle=%lld, path=%s)",
         static_cast<long long>(requestID), static_cast<long long>(vmiHandle), propertyPath.c_str());

    Command cmd(CommandType::FireTriggerProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;

    enqueueCommand(std::move(cmd));
}

// =============================================================================
// Property Subscriptions - Public API (Phase D.4)
// =============================================================================

void CommandServer::subscribeToProperty(int64_t vmiHandle, const std::string& propertyPath, int32_t propertyType)
{
    LOGI("CommandServer: Enqueuing SubscribeToProperty command (vmiHandle=%lld, path=%s, type=%d)",
         static_cast<long long>(vmiHandle), propertyPath.c_str(), propertyType);

    Command cmd(CommandType::SubscribeToProperty, 0);  // No requestID needed
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.propertyType = propertyType;

    enqueueCommand(std::move(cmd));
}

void CommandServer::unsubscribeFromProperty(int64_t vmiHandle, const std::string& propertyPath, int32_t propertyType)
{
    LOGI("CommandServer: Enqueuing UnsubscribeFromProperty command (vmiHandle=%lld, path=%s, type=%d)",
         static_cast<long long>(vmiHandle), propertyPath.c_str(), propertyType);

    Command cmd(CommandType::UnsubscribeFromProperty, 0);  // No requestID needed
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.propertyType = propertyType;

    enqueueCommand(std::move(cmd));
}

// =============================================================================
// Additional Property Types - Handler Implementations (Phase D.3)
// =============================================================================

void CommandServer::handleGetEnumProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling GetEnumProperty command (requestID=%lld, vmiHandle=%lld, path=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.propertyPath.c_str());

    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        LOGW("CommandServer: Invalid VMI handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& vmi = it->second;
    auto* prop = vmi->propertyEnum(cmd.propertyPath);
    if (!prop) {
        LOGW("CommandServer: Enum property not found: %s", cmd.propertyPath.c_str());

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Enum property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    std::string value = prop->value();

    LOGI("CommandServer: GetEnumProperty succeeded (path=%s, value=%s)",
         cmd.propertyPath.c_str(), value.c_str());

    Message msg(MessageType::EnumPropertyValue, cmd.requestID);
    msg.stringValue = value;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleSetEnumProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling SetEnumProperty command (requestID=%lld, vmiHandle=%lld, path=%s, value=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle),
         cmd.propertyPath.c_str(), cmd.stringValue.c_str());

    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        LOGW("CommandServer: Invalid VMI handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& vmi = it->second;
    auto* prop = vmi->propertyEnum(cmd.propertyPath);
    if (!prop) {
        LOGW("CommandServer: Enum property not found: %s", cmd.propertyPath.c_str());

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Enum property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    prop->value(cmd.stringValue);

    LOGI("CommandServer: SetEnumProperty succeeded (path=%s, value=%s)",
         cmd.propertyPath.c_str(), cmd.stringValue.c_str());

    // Emit update to subscribers (Phase D.4)
    emitPropertyUpdateIfSubscribed(cmd.handle, cmd.propertyPath, PropertyDataType::ENUM);

    Message msg(MessageType::PropertySetSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleGetColorProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling GetColorProperty command (requestID=%lld, vmiHandle=%lld, path=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.propertyPath.c_str());

    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        LOGW("CommandServer: Invalid VMI handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& vmi = it->second;
    auto* prop = vmi->propertyColor(cmd.propertyPath);
    if (!prop) {
        LOGW("CommandServer: Color property not found: %s", cmd.propertyPath.c_str());

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Color property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    int value = prop->value();

    LOGI("CommandServer: GetColorProperty succeeded (path=%s, value=0x%08X)",
         cmd.propertyPath.c_str(), value);

    Message msg(MessageType::ColorPropertyValue, cmd.requestID);
    msg.colorValue = value;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleSetColorProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling SetColorProperty command (requestID=%lld, vmiHandle=%lld, path=%s, value=0x%08X)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle),
         cmd.propertyPath.c_str(), cmd.colorValue);

    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        LOGW("CommandServer: Invalid VMI handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& vmi = it->second;
    auto* prop = vmi->propertyColor(cmd.propertyPath);
    if (!prop) {
        LOGW("CommandServer: Color property not found: %s", cmd.propertyPath.c_str());

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Color property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    prop->value(cmd.colorValue);

    LOGI("CommandServer: SetColorProperty succeeded (path=%s, value=0x%08X)",
         cmd.propertyPath.c_str(), cmd.colorValue);

    // Emit update to subscribers (Phase D.4)
    emitPropertyUpdateIfSubscribed(cmd.handle, cmd.propertyPath, PropertyDataType::COLOR);

    Message msg(MessageType::PropertySetSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleFireTriggerProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling FireTriggerProperty command (requestID=%lld, vmiHandle=%lld, path=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.propertyPath.c_str());

    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        LOGW("CommandServer: Invalid VMI handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& vmi = it->second;
    auto* prop = vmi->propertyTrigger(cmd.propertyPath);
    if (!prop) {
        LOGW("CommandServer: Trigger property not found: %s", cmd.propertyPath.c_str());

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Trigger property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    prop->trigger();

    LOGI("CommandServer: FireTriggerProperty succeeded (path=%s)",
         cmd.propertyPath.c_str());

    // Emit update to subscribers (Phase D.4)
    emitPropertyUpdateIfSubscribed(cmd.handle, cmd.propertyPath, PropertyDataType::TRIGGER);

    Message msg(MessageType::TriggerFired, cmd.requestID);
    enqueueMessage(std::move(msg));
}

// =============================================================================
// Property Subscriptions - Handler Implementations (Phase D.4)
// =============================================================================

void CommandServer::handleSubscribeToProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling SubscribeToProperty command (vmiHandle=%lld, path=%s, type=%d)",
         static_cast<long long>(cmd.handle), cmd.propertyPath.c_str(), cmd.propertyType);

    // Validate the VMI handle exists
    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        LOGW("CommandServer: Invalid VMI handle for subscription: %lld", static_cast<long long>(cmd.handle));
        return;
    }

    PropertySubscription sub;
    sub.vmiHandle = cmd.handle;
    sub.propertyPath = cmd.propertyPath;
    sub.propertyType = static_cast<PropertyDataType>(cmd.propertyType);

    // Check if already subscribed (avoid duplicates)
    {
        std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
        for (const auto& existing : m_propertySubscriptions) {
            if (existing == sub) {
                LOGI("CommandServer: Already subscribed to property (vmiHandle=%lld, path=%s)",
                     static_cast<long long>(cmd.handle), cmd.propertyPath.c_str());
                return;
            }
        }
        m_propertySubscriptions.push_back(sub);
    }

    LOGI("CommandServer: Subscribed to property (vmiHandle=%lld, path=%s, type=%d)",
         static_cast<long long>(cmd.handle), cmd.propertyPath.c_str(), cmd.propertyType);
}

void CommandServer::handleUnsubscribeFromProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling UnsubscribeFromProperty command (vmiHandle=%lld, path=%s, type=%d)",
         static_cast<long long>(cmd.handle), cmd.propertyPath.c_str(), cmd.propertyType);

    PropertySubscription sub;
    sub.vmiHandle = cmd.handle;
    sub.propertyPath = cmd.propertyPath;
    sub.propertyType = static_cast<PropertyDataType>(cmd.propertyType);

    {
        std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
        auto it = std::find(m_propertySubscriptions.begin(), m_propertySubscriptions.end(), sub);
        if (it != m_propertySubscriptions.end()) {
            m_propertySubscriptions.erase(it);
            LOGI("CommandServer: Unsubscribed from property (vmiHandle=%lld, path=%s)",
                 static_cast<long long>(cmd.handle), cmd.propertyPath.c_str());
        } else {
            LOGI("CommandServer: Subscription not found (vmiHandle=%lld, path=%s)",
                 static_cast<long long>(cmd.handle), cmd.propertyPath.c_str());
        }
    }
}

void CommandServer::emitPropertyUpdateIfSubscribed(int64_t vmiHandle, const std::string& propertyPath, PropertyDataType propertyType)
{
    // Check if this property is subscribed
    bool isSubscribed = false;
    {
        std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
        for (const auto& sub : m_propertySubscriptions) {
            if (sub.vmiHandle == vmiHandle && sub.propertyPath == propertyPath && sub.propertyType == propertyType) {
                isSubscribed = true;
                break;
            }
        }
    }

    if (!isSubscribed) {
        return;
    }

    // Get the VMI
    auto it = m_viewModelInstances.find(vmiHandle);
    if (it == m_viewModelInstances.end()) {
        return;
    }

    auto& vmi = it->second;

    // Emit update message based on property type
    switch (propertyType) {
        case PropertyDataType::NUMBER: {
            auto* prop = vmi->propertyNumber(propertyPath);
            if (prop) {
                Message msg(MessageType::NumberPropertyUpdated, 0);
                msg.vmiHandle = vmiHandle;
                msg.propertyPath = propertyPath;
                msg.floatValue = prop->value();
                enqueueMessage(std::move(msg));
            }
            break;
        }
        case PropertyDataType::STRING: {
            auto* prop = vmi->propertyString(propertyPath);
            if (prop) {
                Message msg(MessageType::StringPropertyUpdated, 0);
                msg.vmiHandle = vmiHandle;
                msg.propertyPath = propertyPath;
                msg.stringValue = prop->value();
                enqueueMessage(std::move(msg));
            }
            break;
        }
        case PropertyDataType::BOOLEAN: {
            auto* prop = vmi->propertyBoolean(propertyPath);
            if (prop) {
                Message msg(MessageType::BooleanPropertyUpdated, 0);
                msg.vmiHandle = vmiHandle;
                msg.propertyPath = propertyPath;
                msg.boolValue = prop->value();
                enqueueMessage(std::move(msg));
            }
            break;
        }
        case PropertyDataType::ENUM: {
            auto* prop = vmi->propertyEnum(propertyPath);
            if (prop) {
                Message msg(MessageType::EnumPropertyUpdated, 0);
                msg.vmiHandle = vmiHandle;
                msg.propertyPath = propertyPath;
                msg.stringValue = prop->value();
                enqueueMessage(std::move(msg));
            }
            break;
        }
        case PropertyDataType::COLOR: {
            auto* prop = vmi->propertyColor(propertyPath);
            if (prop) {
                Message msg(MessageType::ColorPropertyUpdated, 0);
                msg.vmiHandle = vmiHandle;
                msg.propertyPath = propertyPath;
                msg.colorValue = prop->value();
                enqueueMessage(std::move(msg));
            }
            break;
        }
        case PropertyDataType::TRIGGER: {
            // Triggers don't have a value, just emit that it was fired
            Message msg(MessageType::TriggerPropertyFired, 0);
            msg.vmiHandle = vmiHandle;
            msg.propertyPath = propertyPath;
            enqueueMessage(std::move(msg));
            break;
        }
        default:
            LOGW("CommandServer: Unsupported property type for subscription update: %d",
                 static_cast<int>(propertyType));
            break;
    }
}

// =============================================================================
// Phase D.5: List Operations - Public API
// =============================================================================

void CommandServer::getListSize(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath)
{
    Command cmd(CommandType::GetListSize, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    enqueueCommand(std::move(cmd));
}

void CommandServer::getListItem(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int32_t index)
{
    Command cmd(CommandType::GetListItem, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.listIndex = index;
    enqueueCommand(std::move(cmd));
}

void CommandServer::addListItem(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int64_t itemHandle)
{
    Command cmd(CommandType::AddListItem, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.itemHandle = itemHandle;
    enqueueCommand(std::move(cmd));
}

void CommandServer::addListItemAt(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int32_t index, int64_t itemHandle)
{
    Command cmd(CommandType::AddListItemAt, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.listIndex = index;
    cmd.itemHandle = itemHandle;
    enqueueCommand(std::move(cmd));
}

void CommandServer::removeListItem(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int64_t itemHandle)
{
    Command cmd(CommandType::RemoveListItem, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.itemHandle = itemHandle;
    enqueueCommand(std::move(cmd));
}

void CommandServer::removeListItemAt(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int32_t index)
{
    Command cmd(CommandType::RemoveListItemAt, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.listIndex = index;
    enqueueCommand(std::move(cmd));
}

void CommandServer::swapListItems(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int32_t indexA, int32_t indexB)
{
    Command cmd(CommandType::SwapListItems, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.listIndex = indexA;
    cmd.listIndexB = indexB;
    enqueueCommand(std::move(cmd));
}

// =============================================================================
// Phase D.5: Nested VMI Operations - Public API
// =============================================================================

void CommandServer::getInstanceProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath)
{
    Command cmd(CommandType::GetInstanceProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    enqueueCommand(std::move(cmd));
}

void CommandServer::setInstanceProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int64_t nestedHandle)
{
    Command cmd(CommandType::SetInstanceProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.nestedHandle = nestedHandle;
    enqueueCommand(std::move(cmd));
}

// =============================================================================
// Phase D.5: Asset Property Operations - Public API
// =============================================================================

void CommandServer::setImageProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int64_t imageHandle)
{
    Command cmd(CommandType::SetImageProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.assetHandle = imageHandle;
    enqueueCommand(std::move(cmd));
}

void CommandServer::setArtboardProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int64_t fileHandle, int64_t artboardHandle)
{
    Command cmd(CommandType::SetArtboardProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.fileHandle = fileHandle;
    cmd.assetHandle = artboardHandle;
    enqueueCommand(std::move(cmd));
}

// =============================================================================
// Phase D.5: List Operations - Handlers
// =============================================================================

void CommandServer::handleGetListSize(const Command& cmd)
{
    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto* listProp = it->second->propertyList(cmd.propertyPath);
    if (!listProp) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "List property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    Message msg(MessageType::ListSizeResult, cmd.requestID);
    msg.intValue = static_cast<int32_t>(listProp->size());
    enqueueMessage(std::move(msg));
}

void CommandServer::handleGetListItem(const Command& cmd)
{
    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto* listProp = it->second->propertyList(cmd.propertyPath);
    if (!listProp) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "List property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    if (cmd.listIndex < 0 || static_cast<size_t>(cmd.listIndex) >= listProp->size()) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Index out of bounds: " + std::to_string(cmd.listIndex);
        enqueueMessage(std::move(msg));
        return;
    }

    auto itemVmi = listProp->instanceAt(cmd.listIndex);
    if (!itemVmi) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Failed to get item at index: " + std::to_string(cmd.listIndex);
        enqueueMessage(std::move(msg));
        return;
    }

    // Store the item in our map and return a handle
    int64_t handle = m_nextHandle.fetch_add(1);
    m_viewModelInstances[handle] = itemVmi;

    Message msg(MessageType::ListItemResult, cmd.requestID);
    msg.handle = handle;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleAddListItem(const Command& cmd)
{
    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto* listProp = it->second->propertyList(cmd.propertyPath);
    if (!listProp) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "List property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    auto itemIt = m_viewModelInstances.find(cmd.itemHandle);
    if (itemIt == m_viewModelInstances.end()) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Invalid item ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    listProp->addInstance(itemIt->second.get());

    Message msg(MessageType::ListOperationSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleAddListItemAt(const Command& cmd)
{
    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto* listProp = it->second->propertyList(cmd.propertyPath);
    if (!listProp) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "List property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    auto itemIt = m_viewModelInstances.find(cmd.itemHandle);
    if (itemIt == m_viewModelInstances.end()) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Invalid item ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    bool success = listProp->addInstanceAt(itemIt->second.get(), cmd.listIndex);
    if (!success) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Failed to add item at index: " + std::to_string(cmd.listIndex);
        enqueueMessage(std::move(msg));
        return;
    }

    Message msg(MessageType::ListOperationSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleRemoveListItem(const Command& cmd)
{
    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto* listProp = it->second->propertyList(cmd.propertyPath);
    if (!listProp) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "List property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    auto itemIt = m_viewModelInstances.find(cmd.itemHandle);
    if (itemIt == m_viewModelInstances.end()) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Invalid item ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    listProp->removeInstance(itemIt->second.get());

    Message msg(MessageType::ListOperationSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleRemoveListItemAt(const Command& cmd)
{
    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto* listProp = it->second->propertyList(cmd.propertyPath);
    if (!listProp) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "List property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    if (cmd.listIndex < 0 || static_cast<size_t>(cmd.listIndex) >= listProp->size()) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Index out of bounds: " + std::to_string(cmd.listIndex);
        enqueueMessage(std::move(msg));
        return;
    }

    listProp->removeInstanceAt(cmd.listIndex);

    Message msg(MessageType::ListOperationSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleSwapListItems(const Command& cmd)
{
    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto* listProp = it->second->propertyList(cmd.propertyPath);
    if (!listProp) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "List property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    size_t size = listProp->size();
    if (cmd.listIndex < 0 || static_cast<size_t>(cmd.listIndex) >= size ||
        cmd.listIndexB < 0 || static_cast<size_t>(cmd.listIndexB) >= size) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Index out of bounds for swap";
        enqueueMessage(std::move(msg));
        return;
    }

    listProp->swap(static_cast<uint32_t>(cmd.listIndex), static_cast<uint32_t>(cmd.listIndexB));

    Message msg(MessageType::ListOperationSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

// =============================================================================
// Phase D.5: Nested VMI Operations - Handlers
// =============================================================================

void CommandServer::handleGetInstanceProperty(const Command& cmd)
{
    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        Message msg(MessageType::InstancePropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto nestedVmi = it->second->propertyViewModel(cmd.propertyPath);
    if (!nestedVmi) {
        Message msg(MessageType::InstancePropertyError, cmd.requestID);
        msg.error = "Instance property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    // Store the nested VMI in our map and return a handle
    int64_t handle = m_nextHandle.fetch_add(1);
    m_viewModelInstances[handle] = nestedVmi;

    Message msg(MessageType::InstancePropertyResult, cmd.requestID);
    msg.handle = handle;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleSetInstanceProperty(const Command& cmd)
{
    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        Message msg(MessageType::InstancePropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto nestedIt = m_viewModelInstances.find(cmd.nestedHandle);
    if (nestedIt == m_viewModelInstances.end()) {
        Message msg(MessageType::InstancePropertyError, cmd.requestID);
        msg.error = "Invalid nested ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    bool success = it->second->replaceViewModel(cmd.propertyPath, nestedIt->second.get());
    if (!success) {
        Message msg(MessageType::InstancePropertyError, cmd.requestID);
        msg.error = "Failed to set instance property: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    Message msg(MessageType::InstancePropertySetSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

// =============================================================================
// Phase D.5: Asset Property Operations - Handlers
// =============================================================================

void CommandServer::handleSetImageProperty(const Command& cmd)
{
    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        Message msg(MessageType::AssetPropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto* imageProp = it->second->propertyImage(cmd.propertyPath);
    if (!imageProp) {
        Message msg(MessageType::AssetPropertyError, cmd.requestID);
        msg.error = "Image property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    // Note: For now we just set to nullptr since we don't have an image storage map yet
    // This is a placeholder - full implementation would look up the image from a storage map
    if (cmd.assetHandle == 0) {
        imageProp->value(nullptr);
    } else {
        // TODO: Look up image from m_images map when implemented
        Message msg(MessageType::AssetPropertyError, cmd.requestID);
        msg.error = "Image storage not yet implemented";
        enqueueMessage(std::move(msg));
        return;
    }

    Message msg(MessageType::AssetPropertySetSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleSetArtboardProperty(const Command& cmd)
{
    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        Message msg(MessageType::AssetPropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto* artboardProp = it->second->propertyArtboard(cmd.propertyPath);
    if (!artboardProp) {
        Message msg(MessageType::AssetPropertyError, cmd.requestID);
        msg.error = "Artboard property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    if (cmd.assetHandle == 0) {
        // Clear the artboard property
        artboardProp->value(nullptr);
    } else {
        // Look up file from our storage map
        auto fileIt = m_files.find(cmd.fileHandle);
        if (fileIt == m_files.end()) {
            Message msg(MessageType::AssetPropertyError, cmd.requestID);
            msg.error = "Invalid file handle";
            enqueueMessage(std::move(msg));
            return;
        }

        // Look up artboard from our storage map
        auto artboardIt = m_artboards.find(cmd.assetHandle);
        if (artboardIt == m_artboards.end()) {
            Message msg(MessageType::AssetPropertyError, cmd.requestID);
            msg.error = "Invalid artboard handle";
            enqueueMessage(std::move(msg));
            return;
        }

        // Get the artboard as a raw pointer (we need the base Artboard, not ArtboardInstance)
        rive::Artboard* artboard = artboardIt->second.get();

        // Create a BindableArtboard using the file's internal method
        auto bindableArtboard = fileIt->second->internalBindableArtboardFromArtboard(artboard);
        artboardProp->value(bindableArtboard);
    }

    Message msg(MessageType::AssetPropertySetSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

// =============================================================================
// Phase D.6: VMI Binding to State Machine - Public API
// =============================================================================

void CommandServer::bindViewModelInstance(int64_t requestID, int64_t smHandle, int64_t vmiHandle)
{
    Command cmd(CommandType::BindViewModelInstance, requestID);
    cmd.handle = smHandle;      // State machine handle
    cmd.vmiHandle = vmiHandle;  // VMI handle to bind
    enqueueCommand(std::move(cmd));
}

void CommandServer::getDefaultViewModelInstance(int64_t requestID, int64_t fileHandle, int64_t artboardHandle)
{
    Command cmd(CommandType::GetDefaultVMI, requestID);
    cmd.fileHandle = fileHandle;
    cmd.handle = artboardHandle;  // Artboard handle
    enqueueCommand(std::move(cmd));
}

// =============================================================================
// Phase D.6: VMI Binding to State Machine - Handlers
// =============================================================================

void CommandServer::handleBindViewModelInstance(const Command& cmd)
{
    // Look up state machine
    auto smIt = m_stateMachines.find(cmd.handle);
    if (smIt == m_stateMachines.end()) {
        Message msg(MessageType::VMIBindingError, cmd.requestID);
        msg.error = "Invalid state machine handle";
        enqueueMessage(std::move(msg));
        return;
    }

    // Look up VMI
    auto vmiIt = m_viewModelInstances.find(cmd.vmiHandle);
    if (vmiIt == m_viewModelInstances.end()) {
        Message msg(MessageType::VMIBindingError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    // Bind the VMI to the state machine
    // StateMachineInstance::bindViewModelInstance takes rcp<ViewModelInstance>
    // ViewModelInstanceRuntime wraps a ViewModelInstance, use instance() to get it
    smIt->second->bindViewModelInstance(vmiIt->second->instance());

    Message msg(MessageType::VMIBindingSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleGetDefaultVMI(const Command& cmd)
{
    // Look up file
    auto fileIt = m_files.find(cmd.fileHandle);
    if (fileIt == m_files.end()) {
        Message msg(MessageType::DefaultVMIError, cmd.requestID);
        msg.error = "Invalid file handle";
        enqueueMessage(std::move(msg));
        return;
    }

    // Look up artboard
    auto artboardIt = m_artboards.find(cmd.handle);
    if (artboardIt == m_artboards.end()) {
        Message msg(MessageType::DefaultVMIError, cmd.requestID);
        msg.error = "Invalid artboard handle";
        enqueueMessage(std::move(msg));
        return;
    }

    // Get default VMI from file for the artboard
    // File::createDefaultViewModelInstance returns rcp<ViewModelInstance>
    auto defaultVMI = fileIt->second->createDefaultViewModelInstance(artboardIt->second.get());

    if (!defaultVMI) {
        // No default VMI for this artboard - return 0 handle (not an error, just no default)
        Message msg(MessageType::DefaultVMIResult, cmd.requestID);
        msg.handle = 0;  // 0 means no default VMI
        enqueueMessage(std::move(msg));
        return;
    }

    // Wrap in ViewModelInstanceRuntime for storage
    // We store rcp<ViewModelInstanceRuntime>, not raw rcp<ViewModelInstance>
    auto runtimeVMI = rive::rcp<rive::ViewModelInstanceRuntime>(
        new rive::ViewModelInstanceRuntime(std::move(defaultVMI))
    );

    // Store the VMI and return its handle
    int64_t vmiHandle = m_nextHandle++;
    m_viewModelInstances[vmiHandle] = std::move(runtimeVMI);

    Message msg(MessageType::DefaultVMIResult, cmd.requestID);
    msg.handle = vmiHandle;
    enqueueMessage(std::move(msg));
}

// =============================================================================
// Phase C.2.3: Render Target Operations - Public API
// =============================================================================

void CommandServer::createRenderTarget(int64_t requestID, int32_t width, int32_t height, int32_t sampleCount)
{
    LOGI("CommandServer: Enqueuing CreateRenderTarget command (requestID=%lld, width=%d, height=%d, sampleCount=%d)",
         static_cast<long long>(requestID), width, height, sampleCount);

    Command cmd(CommandType::CreateRenderTarget, requestID);
    cmd.rtWidth = width;
    cmd.rtHeight = height;
    cmd.sampleCount = sampleCount;

    enqueueCommand(std::move(cmd));
}

void CommandServer::deleteRenderTarget(int64_t requestID, int64_t renderTargetHandle)
{
    LOGI("CommandServer: Enqueuing DeleteRenderTarget command (requestID=%lld, handle=%lld)",
         static_cast<long long>(requestID), static_cast<long long>(renderTargetHandle));

    Command cmd(CommandType::DeleteRenderTarget, requestID);
    cmd.handle = renderTargetHandle;

    enqueueCommand(std::move(cmd));
}

// =============================================================================
// Phase C.2.6: Rendering Operations - Public API
// =============================================================================

void CommandServer::draw(int64_t requestID,
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
                          float scaleFactor)
{
    LOGI("CommandServer: Enqueuing Draw command (requestID=%lld, artboard=%lld, sm=%lld)",
         static_cast<long long>(requestID),
         static_cast<long long>(artboardHandle),
         static_cast<long long>(smHandle));

    Command cmd(CommandType::Draw, requestID);
    cmd.artboardHandle = artboardHandle;
    cmd.smHandle = smHandle;
    cmd.surfacePtr = surfacePtr;
    cmd.renderTargetPtr = renderTargetPtr;
    cmd.drawKey = drawKey;
    cmd.surfaceWidth = width;
    cmd.surfaceHeight = height;
    cmd.fitMode = fitMode;
    cmd.alignmentMode = alignmentMode;
    cmd.clearColor = clearColor;
    cmd.scaleFactor = scaleFactor;

    enqueueCommand(std::move(cmd));
}

// =============================================================================
// Phase C.2.3: Render Target Operations - Handlers
// =============================================================================

void CommandServer::handleCreateRenderTarget(const Command& cmd)
{
    LOGI("CommandServer: Handling CreateRenderTarget command (requestID=%lld, %dx%d, samples=%d)",
         static_cast<long long>(cmd.requestID), cmd.rtWidth, cmd.rtHeight, cmd.sampleCount);

    // Validate dimensions
    if (cmd.rtWidth <= 0 || cmd.rtHeight <= 0) {
        LOGW("CommandServer: Invalid render target dimensions: %dx%d", cmd.rtWidth, cmd.rtHeight);

        Message msg(MessageType::RenderTargetError, cmd.requestID);
        msg.error = "Invalid render target dimensions";
        enqueueMessage(std::move(msg));
        return;
    }

    // TODO (Phase C.2.6 full rendering): Create actual rive::gpu::FramebufferRenderTargetGL
    // This requires:
    // - rive::gpu::RenderContext to be available and initialized
    // - GL context to be current on this thread
    // - Proper GPU renderer setup
    //
    // Placeholder implementation:
    // For now, we create a placeholder handle to unblock development.
    // The actual render target creation will be implemented when full GPU rendering
    // is integrated in Phase C.2.6.

    int64_t handle = m_nextHandle.fetch_add(1);

    // Store nullptr for now - will be replaced with actual RenderTargetGL in Phase C.2.6
    m_renderTargets[handle] = nullptr;

    LOGI("CommandServer: Created render target handle %lld (placeholder)", static_cast<long long>(handle));

    Message msg(MessageType::RenderTargetCreated, cmd.requestID);
    msg.handle = handle;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleDeleteRenderTarget(const Command& cmd)
{
    LOGI("CommandServer: Handling DeleteRenderTarget command (requestID=%lld, handle=%lld)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle));

    auto it = m_renderTargets.find(cmd.handle);
    if (it == m_renderTargets.end()) {
        LOGW("CommandServer: Invalid render target handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::RenderTargetError, cmd.requestID);
        msg.error = "Invalid render target handle";
        enqueueMessage(std::move(msg));
        return;
    }

    // TODO (Phase C.2.6 full rendering): Delete actual rive::gpu::RenderTargetGL
    // if (it->second != nullptr) {
    //     delete it->second;
    // }

    m_renderTargets.erase(it);

    LOGI("CommandServer: Deleted render target handle %lld", static_cast<long long>(cmd.handle));

    Message msg(MessageType::RenderTargetDeleted, cmd.requestID);
    enqueueMessage(std::move(msg));
}

// =============================================================================
// Phase C.2.6: Rendering Operations - Handler
// =============================================================================

/**
 * Convert Fit ordinal to rive::Fit enum.
 * Ordinal mapping:
 * 0=FILL, 1=CONTAIN, 2=COVER, 3=FIT_WIDTH, 4=FIT_HEIGHT, 5=NONE, 6=SCALE_DOWN, 7=LAYOUT
 */
static rive::Fit getFitFromOrdinal(int32_t ordinal)
{
    switch (ordinal)
    {
        case 0: return rive::Fit::fill;
        case 1: return rive::Fit::contain;
        case 2: return rive::Fit::cover;
        case 3: return rive::Fit::fitWidth;
        case 4: return rive::Fit::fitHeight;
        case 5: return rive::Fit::none;
        case 6: return rive::Fit::scaleDown;
        case 7: return rive::Fit::layout;
        default:
            LOGW("CommandServer: Invalid Fit ordinal %d, defaulting to CONTAIN", ordinal);
            return rive::Fit::contain;
    }
}

/**
 * Convert Alignment ordinal to rive::Alignment enum.
 * Ordinal mapping:
 * 0=TOP_LEFT, 1=TOP_CENTER, 2=TOP_RIGHT,
 * 3=CENTER_LEFT, 4=CENTER, 5=CENTER_RIGHT,
 * 6=BOTTOM_LEFT, 7=BOTTOM_CENTER, 8=BOTTOM_RIGHT
 */
static rive::Alignment getAlignmentFromOrdinal(int32_t ordinal)
{
    switch (ordinal)
    {
        case 0: return rive::Alignment::topLeft;
        case 1: return rive::Alignment::topCenter;
        case 2: return rive::Alignment::topRight;
        case 3: return rive::Alignment::centerLeft;
        case 4: return rive::Alignment::center;
        case 5: return rive::Alignment::centerRight;
        case 6: return rive::Alignment::bottomLeft;
        case 7: return rive::Alignment::bottomCenter;
        case 8: return rive::Alignment::bottomRight;
        default:
            LOGW("CommandServer: Invalid Alignment ordinal %d, defaulting to CENTER", ordinal);
            return rive::Alignment::center;
    }
}

void CommandServer::handleDraw(const Command& cmd)
{
    LOGI("CommandServer: Handling Draw command (requestID=%lld, artboard=%lld, sm=%lld, %dx%d)",
         static_cast<long long>(cmd.requestID),
         static_cast<long long>(cmd.artboardHandle),
         static_cast<long long>(cmd.smHandle),
         cmd.surfaceWidth, cmd.surfaceHeight);

    // 1. Validate artboard handle
    auto artboardIt = m_artboards.find(cmd.artboardHandle);
    if (artboardIt == m_artboards.end()) {
        LOGW("CommandServer: Invalid artboard handle: %lld", static_cast<long long>(cmd.artboardHandle));

        Message msg(MessageType::DrawError, cmd.requestID);
        msg.error = "Invalid artboard handle";
        enqueueMessage(std::move(msg));
        return;
    }

    // 2. Validate state machine handle (optional - can be 0 for static artboards)
    rive::StateMachineInstance* sm = nullptr;
    if (cmd.smHandle != 0) {
        auto smIt = m_stateMachines.find(cmd.smHandle);
        if (smIt == m_stateMachines.end()) {
            LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.smHandle));

            Message msg(MessageType::DrawError, cmd.requestID);
            msg.error = "Invalid state machine handle";
            enqueueMessage(std::move(msg));
            return;
        }
        sm = smIt->second.get();
    }

    auto& artboard = artboardIt->second;

    // 3. Check render context
    if (m_renderContext == nullptr) {
        LOGW("CommandServer: No render context available");

        Message msg(MessageType::DrawError, cmd.requestID);
        msg.error = "No render context available";
        enqueueMessage(std::move(msg));
        return;
    }

    // 4. Get RenderContext and validate Rive GPU RenderContext
    auto* renderContext = static_cast<rive_mp::RenderContext*>(m_renderContext);
    if (renderContext->riveContext == nullptr) {
        LOGW("CommandServer: Rive GPU RenderContext not initialized");

        Message msg(MessageType::DrawError, cmd.requestID);
        msg.error = "Rive GPU RenderContext not initialized";
        enqueueMessage(std::move(msg));
        return;
    }

    // 5. Get render target
    auto* renderTarget = reinterpret_cast<rive::gpu::RenderTargetGL*>(cmd.renderTargetPtr);
    if (renderTarget == nullptr) {
        LOGW("CommandServer: Invalid render target pointer");

        Message msg(MessageType::DrawError, cmd.requestID);
        msg.error = "Invalid render target pointer";
        enqueueMessage(std::move(msg));
        return;
    }

    // 6. Make EGL context current for this surface
    void* surfacePtr = reinterpret_cast<void*>(cmd.surfacePtr);
    renderContext->beginFrame(surfacePtr);

    // 7. Begin Rive GPU frame with clear
    // Extract RGBA components from 0xAARRGGBB format
    uint32_t clearColor = cmd.clearColor;
    renderContext->riveContext->beginFrame({
        .renderTargetWidth = static_cast<uint32_t>(cmd.surfaceWidth),
        .renderTargetHeight = static_cast<uint32_t>(cmd.surfaceHeight),
        .loadAction = rive::gpu::LoadAction::clear,
        .clearColor = clearColor
    });

    // 8. Create renderer and apply fit/alignment transformation
    auto renderer = rive::RiveRenderer(renderContext->riveContext.get());

    // Convert ordinals to rive enums
    rive::Fit fit = getFitFromOrdinal(cmd.fitMode);
    rive::Alignment alignment = getAlignmentFromOrdinal(cmd.alignmentMode);

    // Save renderer state before transformation
    renderer.save();

    // Apply fit & alignment to map artboard bounds to surface bounds
    rive::AABB surfaceBounds(0, 0,
                             static_cast<float>(cmd.surfaceWidth),
                             static_cast<float>(cmd.surfaceHeight));
    renderer.align(fit,
                   alignment,
                   surfaceBounds,
                   artboard->bounds(),
                   cmd.scaleFactor);

    // 9. Draw the artboard
    artboard->draw(&renderer);

    // Restore renderer state
    renderer.restore();

    // 10. Flush Rive GPU context to submit rendering commands
    renderContext->riveContext->flush({.renderTarget = renderTarget});

    // 11. Present the frame (swap buffers)
    renderContext->present(surfacePtr);

    // 12. Send success message
    LOGI("CommandServer: Draw command completed successfully (artboard=%s, %dx%d)",
         artboard->name().c_str(),
         static_cast<int>(artboard->width()),
         static_cast<int>(artboard->height()));

    Message msg(MessageType::DrawComplete, cmd.requestID);
    msg.handle = cmd.drawKey;  // Return the draw key for correlation
    enqueueMessage(std::move(msg));
}

// =============================================================================
// Phase E.3: Pointer Events - Public API
// =============================================================================

void CommandServer::pointerMove(int64_t smHandle, int8_t fit, int8_t alignment,
                                 float layoutScale, float surfaceWidth, float surfaceHeight,
                                 int32_t pointerID, float x, float y)
{
    LOGI("CommandServer: Enqueuing PointerMove (smHandle=%lld, x=%f, y=%f)",
         static_cast<long long>(smHandle), x, y);

    Command cmd(CommandType::PointerMove, 0);  // No requestID needed for fire-and-forget
    cmd.handle = smHandle;
    cmd.pointerFit = fit;
    cmd.pointerAlignment = alignment;
    cmd.layoutScale = layoutScale;
    cmd.pointerSurfaceWidth = surfaceWidth;
    cmd.pointerSurfaceHeight = surfaceHeight;
    cmd.pointerID = pointerID;
    cmd.pointerX = x;
    cmd.pointerY = y;

    enqueueCommand(std::move(cmd));
}

void CommandServer::pointerDown(int64_t smHandle, int8_t fit, int8_t alignment,
                                 float layoutScale, float surfaceWidth, float surfaceHeight,
                                 int32_t pointerID, float x, float y)
{
    LOGI("CommandServer: Enqueuing PointerDown (smHandle=%lld, x=%f, y=%f)",
         static_cast<long long>(smHandle), x, y);

    Command cmd(CommandType::PointerDown, 0);
    cmd.handle = smHandle;
    cmd.pointerFit = fit;
    cmd.pointerAlignment = alignment;
    cmd.layoutScale = layoutScale;
    cmd.pointerSurfaceWidth = surfaceWidth;
    cmd.pointerSurfaceHeight = surfaceHeight;
    cmd.pointerID = pointerID;
    cmd.pointerX = x;
    cmd.pointerY = y;

    enqueueCommand(std::move(cmd));
}

void CommandServer::pointerUp(int64_t smHandle, int8_t fit, int8_t alignment,
                               float layoutScale, float surfaceWidth, float surfaceHeight,
                               int32_t pointerID, float x, float y)
{
    LOGI("CommandServer: Enqueuing PointerUp (smHandle=%lld, x=%f, y=%f)",
         static_cast<long long>(smHandle), x, y);

    Command cmd(CommandType::PointerUp, 0);
    cmd.handle = smHandle;
    cmd.pointerFit = fit;
    cmd.pointerAlignment = alignment;
    cmd.layoutScale = layoutScale;
    cmd.pointerSurfaceWidth = surfaceWidth;
    cmd.pointerSurfaceHeight = surfaceHeight;
    cmd.pointerID = pointerID;
    cmd.pointerX = x;
    cmd.pointerY = y;

    enqueueCommand(std::move(cmd));
}

void CommandServer::pointerExit(int64_t smHandle, int8_t fit, int8_t alignment,
                                 float layoutScale, float surfaceWidth, float surfaceHeight,
                                 int32_t pointerID, float x, float y)
{
    LOGI("CommandServer: Enqueuing PointerExit (smHandle=%lld)",
         static_cast<long long>(smHandle));

    Command cmd(CommandType::PointerExit, 0);
    cmd.handle = smHandle;
    cmd.pointerFit = fit;
    cmd.pointerAlignment = alignment;
    cmd.layoutScale = layoutScale;
    cmd.pointerSurfaceWidth = surfaceWidth;
    cmd.pointerSurfaceHeight = surfaceHeight;
    cmd.pointerID = pointerID;
    cmd.pointerX = x;
    cmd.pointerY = y;

    enqueueCommand(std::move(cmd));
}

// =============================================================================
// Phase E.3: Pointer Events - Coordinate Transformation Helper
// =============================================================================

bool CommandServer::transformToArtboardCoords(int64_t smHandle, int8_t fit, int8_t alignment,
                                               float layoutScale, float surfaceWidth, float surfaceHeight,
                                               float surfaceX, float surfaceY,
                                               float& outX, float& outY)
{
    // Get the state machine to access its artboard bounds
    auto smIt = m_stateMachines.find(smHandle);
    if (smIt == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle for coordinate transform: %lld",
             static_cast<long long>(smHandle));
        return false;
    }

    // Get artboard bounds from the state machine's artboard
    auto* artboard = smIt->second->artboard();
    if (!artboard) {
        LOGW("CommandServer: State machine has no artboard for coordinate transform");
        return false;
    }

    rive::AABB artboardBounds = artboard->bounds();
    rive::AABB surfaceBounds(0, 0, surfaceWidth, surfaceHeight);

    // Get the fit and alignment
    rive::Fit riveFit = getFitFromOrdinal(static_cast<int32_t>(fit));
    rive::Alignment riveAlignment = getAlignmentFromOrdinal(static_cast<int32_t>(alignment));

    // Compute the transformation matrix using rive::computeAlignment
    rive::Mat2D viewTransform = rive::computeAlignment(
        riveFit,
        riveAlignment,
        surfaceBounds,
        artboardBounds,
        layoutScale
    );

    // Invert the transform to go from surface space to artboard space
    rive::Mat2D inverseTransform;
    if (!viewTransform.invert(&inverseTransform)) {
        LOGW("CommandServer: Failed to invert view transform for coordinate transform");
        // If inversion fails, return the original coordinates
        outX = surfaceX;
        outY = surfaceY;
        return true;
    }

    // Transform the point
    rive::Vec2D surfacePoint(surfaceX, surfaceY);
    rive::Vec2D artboardPoint = inverseTransform * surfacePoint;

    outX = artboardPoint.x;
    outY = artboardPoint.y;

    return true;
}

// =============================================================================
// Phase E.3: Pointer Events - Handlers
// =============================================================================

void CommandServer::handlePointerMove(const Command& cmd)
{
    LOGI("CommandServer: Handling PointerMove (smHandle=%lld, x=%f, y=%f)",
         static_cast<long long>(cmd.handle), cmd.pointerX, cmd.pointerY);

    auto smIt = m_stateMachines.find(cmd.handle);
    if (smIt == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));
        return;  // Fire-and-forget, no error message
    }

    // Transform coordinates from surface space to artboard space
    float artboardX, artboardY;
    if (!transformToArtboardCoords(cmd.handle, cmd.pointerFit, cmd.pointerAlignment,
                                    cmd.layoutScale, cmd.pointerSurfaceWidth, cmd.pointerSurfaceHeight,
                                    cmd.pointerX, cmd.pointerY,
                                    artboardX, artboardY)) {
        return;
    }

    // Send pointer move to state machine
    smIt->second->pointerMove(rive::Vec2D(artboardX, artboardY));

    LOGI("CommandServer: PointerMove forwarded (artboard coords: %f, %f)", artboardX, artboardY);
}

void CommandServer::handlePointerDown(const Command& cmd)
{
    LOGI("CommandServer: Handling PointerDown (smHandle=%lld, x=%f, y=%f)",
         static_cast<long long>(cmd.handle), cmd.pointerX, cmd.pointerY);

    auto smIt = m_stateMachines.find(cmd.handle);
    if (smIt == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));
        return;
    }

    // Transform coordinates
    float artboardX, artboardY;
    if (!transformToArtboardCoords(cmd.handle, cmd.pointerFit, cmd.pointerAlignment,
                                    cmd.layoutScale, cmd.pointerSurfaceWidth, cmd.pointerSurfaceHeight,
                                    cmd.pointerX, cmd.pointerY,
                                    artboardX, artboardY)) {
        return;
    }

    // Send pointer down to state machine
    smIt->second->pointerDown(rive::Vec2D(artboardX, artboardY));

    LOGI("CommandServer: PointerDown forwarded (artboard coords: %f, %f)", artboardX, artboardY);
}

void CommandServer::handlePointerUp(const Command& cmd)
{
    LOGI("CommandServer: Handling PointerUp (smHandle=%lld, x=%f, y=%f)",
         static_cast<long long>(cmd.handle), cmd.pointerX, cmd.pointerY);

    auto smIt = m_stateMachines.find(cmd.handle);
    if (smIt == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));
        return;
    }

    // Transform coordinates
    float artboardX, artboardY;
    if (!transformToArtboardCoords(cmd.handle, cmd.pointerFit, cmd.pointerAlignment,
                                    cmd.layoutScale, cmd.pointerSurfaceWidth, cmd.pointerSurfaceHeight,
                                    cmd.pointerX, cmd.pointerY,
                                    artboardX, artboardY)) {
        return;
    }

    // Send pointer up to state machine
    smIt->second->pointerUp(rive::Vec2D(artboardX, artboardY));

    LOGI("CommandServer: PointerUp forwarded (artboard coords: %f, %f)", artboardX, artboardY);
}

void CommandServer::handlePointerExit(const Command& cmd)
{
    LOGI("CommandServer: Handling PointerExit (smHandle=%lld)",
         static_cast<long long>(cmd.handle));

    auto smIt = m_stateMachines.find(cmd.handle);
    if (smIt == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));
        return;
    }

    // Send pointer exit to state machine
    // Note: pointerExit doesn't require coordinates in Rive
    smIt->second->pointerExit();

    LOGI("CommandServer: PointerExit forwarded");
}

} // namespace rive_android