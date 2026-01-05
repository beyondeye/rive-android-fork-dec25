#include "command_server.hpp"
#include "rive_log.hpp"
#include "rive/viewmodel/viewmodel.hpp"
#include "rive/animation/state_machine_instance.hpp"
#include "rive/animation/state_machine_input_instance.hpp"
#include "rive/animation/state_machine_bool.hpp"
#include "rive/animation/state_machine_number.hpp"
#include "rive/animation/state_machine_trigger.hpp"
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
    
    // Import the Rive file
    auto file = rive::File::import(
        rive::Span<const uint8_t>(cmd.bytes.data(), cmd.bytes.size()),
        nullptr  // No asset loader for now (Phase E)
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
    
    auto it = m_artboards.find(cmd.handle);
    if (it == m_artboards.end()) {
        LOGW("CommandServer: Invalid artboard handle: %lld", static_cast<long long>(cmd.handle));
        
        Message msg(MessageType::QueryError, cmd.requestID);
        msg.error = "Invalid artboard handle";
        enqueueMessage(std::move(msg));
        return;
    }
    
    std::vector<std::string> names;
    auto& artboard = it->second;
    
    for (size_t i = 0; i < artboard->stateMachineCount(); i++) {
        auto sm = artboard->stateMachine(i);
        if (sm) {
            names.push_back(sm->name());
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

} // namespace rive_android
