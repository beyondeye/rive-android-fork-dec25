#include "command_server.hpp"
#include "rive_log.hpp"

namespace rive_android {

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

// =============================================================================
// Phase E.3: Artboard Resizing (for Fit.Layout)
// =============================================================================

void CommandServer::resizeArtboard(int64_t artboardHandle, int32_t width, int32_t height, float scaleFactor)
{
    LOGI("CommandServer: Enqueuing ResizeArtboard command (artboardHandle=%lld, %dx%d, scale=%f)",
         static_cast<long long>(artboardHandle), width, height, scaleFactor);
    
    Command cmd(CommandType::ResizeArtboard, 0);  // No requestID needed for fire-and-forget
    cmd.handle = artboardHandle;
    cmd.surfaceWidth = width;
    cmd.surfaceHeight = height;
    cmd.scaleFactor = scaleFactor;
    
    enqueueCommand(std::move(cmd));
}

void CommandServer::resetArtboardSize(int64_t artboardHandle)
{
    LOGI("CommandServer: Enqueuing ResetArtboardSize command (artboardHandle=%lld)",
         static_cast<long long>(artboardHandle));
    
    Command cmd(CommandType::ResetArtboardSize, 0);  // No requestID needed for fire-and-forget
    cmd.handle = artboardHandle;
    
    enqueueCommand(std::move(cmd));
}

void CommandServer::handleResizeArtboard(const Command& cmd)
{
    LOGI("CommandServer: Handling ResizeArtboard command (handle=%lld, %dx%d, scale=%f)",
         static_cast<long long>(cmd.handle), cmd.surfaceWidth, cmd.surfaceHeight, cmd.scaleFactor);
    
    auto it = m_artboards.find(cmd.handle);
    if (it == m_artboards.end()) {
        LOGW("CommandServer: Invalid artboard handle for resize: %lld", static_cast<long long>(cmd.handle));
        return;  // Fire-and-forget, no error message
    }
    
    auto& artboard = it->second;
    
    // Apply scale factor to dimensions
    float scaledWidth = static_cast<float>(cmd.surfaceWidth) / cmd.scaleFactor;
    float scaledHeight = static_cast<float>(cmd.surfaceHeight) / cmd.scaleFactor;
    
    // Resize the artboard to match the surface dimensions
    // This is used for Fit.Layout mode where artboard should match surface
    artboard->width(scaledWidth);
    artboard->height(scaledHeight);
    
    LOGI("CommandServer: Artboard resized to %.1fx%.1f (handle=%lld)",
         scaledWidth, scaledHeight, static_cast<long long>(cmd.handle));
}

void CommandServer::handleResetArtboardSize(const Command& cmd)
{
    LOGI("CommandServer: Handling ResetArtboardSize command (handle=%lld)",
         static_cast<long long>(cmd.handle));
    
    auto it = m_artboards.find(cmd.handle);
    if (it == m_artboards.end()) {
        LOGW("CommandServer: Invalid artboard handle for reset: %lld", static_cast<long long>(cmd.handle));
        return;  // Fire-and-forget, no error message
    }
    
    auto& artboard = it->second;
    
    // Reset to original dimensions from the Rive file
    // ArtboardInstance inherits from Artboard which stores original bounds
    artboard->resetSize();
    
    LOGI("CommandServer: Artboard size reset to original (handle=%lld, %.1fx%.1f)",
         static_cast<long long>(cmd.handle), artboard->width(), artboard->height());
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

} // namespace rive_android