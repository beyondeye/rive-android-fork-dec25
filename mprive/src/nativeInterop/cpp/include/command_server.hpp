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
    // Phase C+: State machine operations, etc.
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
    std::string name;            // For CreateArtboardByName
    
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
    std::map<int64_t, std::unique_ptr<rive::Artboard>> m_artboards;
    std::atomic<int64_t> m_nextHandle{1};
    
    // Phase C+: More resource maps
    // std::map<int64_t, std::unique_ptr<rive::StateMachineInstance>> m_stateMachines;
};

} // namespace rive_android

#endif // RIVE_ANDROID_COMMAND_SERVER_HPP
