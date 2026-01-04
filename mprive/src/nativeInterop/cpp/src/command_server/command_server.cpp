#include "command_server.hpp"
#include "rive_log.hpp"
#include "rive/viewmodel/viewmodel.hpp"
#include "rive/animation/state_machine_instance.hpp"
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

} // namespace rive_android
