#include "command_server.hpp"
#include "rive_log.hpp"
#include <cassert>

namespace rive_android {

CommandServer::CommandServer(JNIEnv* env, jobject commandQueue, void* renderContext)
    : m_commandQueueRef(env, commandQueue)
    , m_renderContext(renderContext)
{
    RIVE_LOG_INFO("CommandServer: Constructing");
    start();
}

CommandServer::~CommandServer()
{
    RIVE_LOG_INFO("CommandServer: Destructing");
    stop();
}

void CommandServer::start()
{
    RIVE_LOG_INFO("CommandServer: Starting worker thread");
    m_running.store(true);
    m_thread = std::thread(&CommandServer::commandLoop, this);
}

void CommandServer::stop()
{
    RIVE_LOG_INFO("CommandServer: Stopping worker thread");
    
    // Signal the thread to stop
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_running.store(false);
    }
    m_cv.notify_all();
    
    // Wait for the thread to finish
    if (m_thread.joinable()) {
        RIVE_LOG_INFO("CommandServer: Waiting for worker thread to finish");
        m_thread.join();
        RIVE_LOG_INFO("CommandServer: Worker thread finished");
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
    RIVE_LOG_INFO("CommandServer: Worker thread started");
    
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
                RIVE_LOG_INFO("CommandServer: Worker thread stopping");
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
    
    RIVE_LOG_INFO("CommandServer: Worker thread stopped");
}

void CommandServer::executeCommand(const Command& cmd)
{
    switch (cmd.type) {
        case CommandType::None:
            // Do nothing
            break;
            
        case CommandType::Stop:
            RIVE_LOG_INFO("CommandServer: Received Stop command");
            // The stop logic is handled in commandLoop
            break;
            
        // Phase B+: Add more command types here
        // case CommandType::LoadFile:
        //     handleLoadFile(cmd);
        //     break;
        
        default:
            RIVE_LOG_WARN("CommandServer: Unknown command type: %d", 
                         static_cast<int>(cmd.type));
            break;
    }
}

void CommandServer::pollMessages()
{
    // Phase A: No-op
    // Phase B+: Poll messages from the command server and send them to Kotlin
    // This will involve calling Java methods via JNI to deliver callbacks
}

} // namespace rive_android
