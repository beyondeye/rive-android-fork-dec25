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
#include "jni_refs.hpp"

namespace rive_android {

// Forward declarations
class RenderContext;

/**
 * Command types that can be sent to the CommandServer.
 * For Phase A, we only need minimal command types for thread lifecycle.
 */
enum class CommandType {
    None,
    Stop,
    // Phase B+: LoadFile, DeleteFile, CreateArtboard, etc.
};

/**
 * A command to be executed by the CommandServer.
 * For Phase A, this is a minimal structure.
 */
struct Command {
    CommandType type = CommandType::None;
    int64_t requestID = 0;
    
    Command() = default;
    explicit Command(CommandType t, int64_t reqID = 0) 
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
     * For Phase A, this is a no-op. Phase B+ will implement message polling.
     */
    void pollMessages();
    
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
    GlobalRef<jobject> m_commandQueueRef;
    
    // Render context (for Phase C+)
    void* m_renderContext;
    
    // Phase B+: Resource maps
    // std::map<int64_t, rive::rcp<rive::File>> m_files;
    // std::map<int64_t, std::unique_ptr<rive::Artboard>> m_artboards;
    // std::atomic<int64_t> m_nextHandle{1};
};

} // namespace rive_android

#endif // RIVE_ANDROID_COMMAND_SERVER_HPP
