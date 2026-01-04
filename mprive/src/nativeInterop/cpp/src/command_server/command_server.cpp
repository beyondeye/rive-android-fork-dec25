#include "command_server.hpp"
#include "rive_log.hpp"
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
    
    std::vector<Message> messages;
    
    {
        std::lock_guard<std::mutex> lock(m_messageMutex);
        while (!m_messageQueue.empty()) {
            messages.push_back(std::move(m_messageQueue.front()));
            m_messageQueue.pop();
        }
    }
    
    // Deliver messages to Kotlin
    // For Phase B.1, we'll implement callbacks in the JNI bindings
    // For now, this is a placeholder
    for (const auto& msg : messages) {
        LOGI("CommandServer: Message polled - type=%d, requestID=%lld, handle=%lld",
             static_cast<int>(msg.type), 
             static_cast<long long>(msg.requestID),
             static_cast<long long>(msg.handle));
    }
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
    auto importResult = rive::File::import(
        rive::Span<const uint8_t>(cmd.bytes.data(), cmd.bytes.size()),
        nullptr  // No asset loader for now (Phase E)
    );
    
    if (importResult.ok()) {
        // Generate a unique handle
        int64_t handle = m_nextHandle.fetch_add(1);
        
        // Store the file
        m_files[handle] = importResult.file;
        
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

} // namespace rive_android
