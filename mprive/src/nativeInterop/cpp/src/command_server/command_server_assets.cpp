#include "command_server.hpp"
#include "rive_log.hpp"

namespace rive_android {

// =============================================================================
// Phase E.1: Asset Operations - Public API (Image)
// =============================================================================

void CommandServer::decodeImage(int64_t requestID, const std::vector<uint8_t>& bytes)
{
    LOGI("CommandServer: Enqueuing DecodeImage command (requestID=%lld, size=%zu)",
         static_cast<long long>(requestID), bytes.size());

    Command cmd(CommandType::DecodeImage, requestID);
    cmd.bytes = bytes;

    enqueueCommand(std::move(cmd));
}

void CommandServer::deleteImage(int64_t imageHandle)
{
    LOGI("CommandServer: Enqueuing DeleteImage command (handle=%lld)",
         static_cast<long long>(imageHandle));

    Command cmd(CommandType::DeleteImage, 0);
    cmd.handle = imageHandle;

    enqueueCommand(std::move(cmd));
}

void CommandServer::registerImage(const std::string& name, int64_t imageHandle)
{
    LOGI("CommandServer: Enqueuing RegisterImage command (name=%s, handle=%lld)",
         name.c_str(), static_cast<long long>(imageHandle));

    Command cmd(CommandType::RegisterImage, 0);
    cmd.name = name;
    cmd.handle = imageHandle;

    enqueueCommand(std::move(cmd));
}

void CommandServer::unregisterImage(const std::string& name)
{
    LOGI("CommandServer: Enqueuing UnregisterImage command (name=%s)", name.c_str());

    Command cmd(CommandType::UnregisterImage, 0);
    cmd.name = name;

    enqueueCommand(std::move(cmd));
}

// =============================================================================
// Phase E.1: Asset Operations - Public API (Audio)
// =============================================================================

void CommandServer::decodeAudio(int64_t requestID, const std::vector<uint8_t>& bytes)
{
    LOGI("CommandServer: Enqueuing DecodeAudio command (requestID=%lld, size=%zu)",
         static_cast<long long>(requestID), bytes.size());

    Command cmd(CommandType::DecodeAudio, requestID);
    cmd.bytes = bytes;

    enqueueCommand(std::move(cmd));
}

void CommandServer::deleteAudio(int64_t audioHandle)
{
    LOGI("CommandServer: Enqueuing DeleteAudio command (handle=%lld)",
         static_cast<long long>(audioHandle));

    Command cmd(CommandType::DeleteAudio, 0);
    cmd.handle = audioHandle;

    enqueueCommand(std::move(cmd));
}

void CommandServer::registerAudio(const std::string& name, int64_t audioHandle)
{
    LOGI("CommandServer: Enqueuing RegisterAudio command (name=%s, handle=%lld)",
         name.c_str(), static_cast<long long>(audioHandle));

    Command cmd(CommandType::RegisterAudio, 0);
    cmd.name = name;
    cmd.handle = audioHandle;

    enqueueCommand(std::move(cmd));
}

void CommandServer::unregisterAudio(const std::string& name)
{
    LOGI("CommandServer: Enqueuing UnregisterAudio command (name=%s)", name.c_str());

    Command cmd(CommandType::UnregisterAudio, 0);
    cmd.name = name;

    enqueueCommand(std::move(cmd));
}

// =============================================================================
// Phase E.1: Asset Operations - Public API (Font)
// =============================================================================

void CommandServer::decodeFont(int64_t requestID, const std::vector<uint8_t>& bytes)
{
    LOGI("CommandServer: Enqueuing DecodeFont command (requestID=%lld, size=%zu)",
         static_cast<long long>(requestID), bytes.size());

    Command cmd(CommandType::DecodeFont, requestID);
    cmd.bytes = bytes;

    enqueueCommand(std::move(cmd));
}

void CommandServer::deleteFont(int64_t fontHandle)
{
    LOGI("CommandServer: Enqueuing DeleteFont command (handle=%lld)",
         static_cast<long long>(fontHandle));

    Command cmd(CommandType::DeleteFont, 0);
    cmd.handle = fontHandle;

    enqueueCommand(std::move(cmd));
}

void CommandServer::registerFont(const std::string& name, int64_t fontHandle)
{
    LOGI("CommandServer: Enqueuing RegisterFont command (name=%s, handle=%lld)",
         name.c_str(), static_cast<long long>(fontHandle));

    Command cmd(CommandType::RegisterFont, 0);
    cmd.name = name;
    cmd.handle = fontHandle;

    enqueueCommand(std::move(cmd));
}

void CommandServer::unregisterFont(const std::string& name)
{
    LOGI("CommandServer: Enqueuing UnregisterFont command (name=%s)", name.c_str());

    Command cmd(CommandType::UnregisterFont, 0);
    cmd.name = name;

    enqueueCommand(std::move(cmd));
}

// =============================================================================
// Phase E.1: Asset Operations - Handlers (Image)
// =============================================================================

void CommandServer::handleDecodeImage(const Command& cmd)
{
    LOGI("CommandServer: Handling DecodeImage command (requestID=%lld, size=%zu)",
         static_cast<long long>(cmd.requestID), cmd.bytes.size());

    // TODO: Implement actual image decoding using Rive decoders
    // For now, return an error since full implementation requires:
    // - rive::Bitmap or rive::RenderImage creation
    // - Proper decoder integration (PNG, JPEG, WebP)
    
    Message msg(MessageType::ImageError, cmd.requestID);
    msg.error = "Image decoding not yet implemented";
    enqueueMessage(std::move(msg));
}

void CommandServer::handleDeleteImage(const Command& cmd)
{
    LOGI("CommandServer: Handling DeleteImage command (handle=%lld)",
         static_cast<long long>(cmd.handle));

    auto it = m_images.find(cmd.handle);
    if (it != m_images.end()) {
        // TODO: Properly delete the image data when implemented
        // delete reinterpret_cast<rive::RenderImage*>(it->second);
        m_images.erase(it);
        LOGI("CommandServer: Image deleted successfully (handle=%lld)",
             static_cast<long long>(cmd.handle));
    } else {
        LOGW("CommandServer: Attempted to delete non-existent image (handle=%lld)",
             static_cast<long long>(cmd.handle));
    }
}

void CommandServer::handleRegisterImage(const Command& cmd)
{
    LOGI("CommandServer: Handling RegisterImage command (name=%s, handle=%lld)",
         cmd.name.c_str(), static_cast<long long>(cmd.handle));

    // Verify the image handle exists
    auto it = m_images.find(cmd.handle);
    if (it == m_images.end()) {
        LOGW("CommandServer: Cannot register non-existent image (handle=%lld)",
             static_cast<long long>(cmd.handle));
        return;
    }

    // Register the image by name
    m_registeredImages[cmd.name] = cmd.handle;
    LOGI("CommandServer: Image registered (name=%s, handle=%lld)",
         cmd.name.c_str(), static_cast<long long>(cmd.handle));
}

void CommandServer::handleUnregisterImage(const Command& cmd)
{
    LOGI("CommandServer: Handling UnregisterImage command (name=%s)", cmd.name.c_str());

    auto it = m_registeredImages.find(cmd.name);
    if (it != m_registeredImages.end()) {
        m_registeredImages.erase(it);
        LOGI("CommandServer: Image unregistered (name=%s)", cmd.name.c_str());
    } else {
        LOGW("CommandServer: Attempted to unregister non-existent image (name=%s)",
             cmd.name.c_str());
    }
}

// =============================================================================
// Phase E.1: Asset Operations - Handlers (Audio)
// =============================================================================

void CommandServer::handleDecodeAudio(const Command& cmd)
{
    LOGI("CommandServer: Handling DecodeAudio command (requestID=%lld, size=%zu)",
         static_cast<long long>(cmd.requestID), cmd.bytes.size());

    // TODO: Implement actual audio decoding using Rive audio decoders
    // For now, return an error since full implementation requires:
    // - rive::AudioSource or similar creation
    // - Proper decoder integration (WAV, MP3, etc.)
    
    Message msg(MessageType::AudioError, cmd.requestID);
    msg.error = "Audio decoding not yet implemented";
    enqueueMessage(std::move(msg));
}

void CommandServer::handleDeleteAudio(const Command& cmd)
{
    LOGI("CommandServer: Handling DeleteAudio command (handle=%lld)",
         static_cast<long long>(cmd.handle));

    auto it = m_audioClips.find(cmd.handle);
    if (it != m_audioClips.end()) {
        // TODO: Properly delete the audio data when implemented
        m_audioClips.erase(it);
        LOGI("CommandServer: Audio deleted successfully (handle=%lld)",
             static_cast<long long>(cmd.handle));
    } else {
        LOGW("CommandServer: Attempted to delete non-existent audio (handle=%lld)",
             static_cast<long long>(cmd.handle));
    }
}

void CommandServer::handleRegisterAudio(const Command& cmd)
{
    LOGI("CommandServer: Handling RegisterAudio command (name=%s, handle=%lld)",
         cmd.name.c_str(), static_cast<long long>(cmd.handle));

    // Verify the audio handle exists
    auto it = m_audioClips.find(cmd.handle);
    if (it == m_audioClips.end()) {
        LOGW("CommandServer: Cannot register non-existent audio (handle=%lld)",
             static_cast<long long>(cmd.handle));
        return;
    }

    // Register the audio by name
    m_registeredAudio[cmd.name] = cmd.handle;
    LOGI("CommandServer: Audio registered (name=%s, handle=%lld)",
         cmd.name.c_str(), static_cast<long long>(cmd.handle));
}

void CommandServer::handleUnregisterAudio(const Command& cmd)
{
    LOGI("CommandServer: Handling UnregisterAudio command (name=%s)", cmd.name.c_str());

    auto it = m_registeredAudio.find(cmd.name);
    if (it != m_registeredAudio.end()) {
        m_registeredAudio.erase(it);
        LOGI("CommandServer: Audio unregistered (name=%s)", cmd.name.c_str());
    } else {
        LOGW("CommandServer: Attempted to unregister non-existent audio (name=%s)",
             cmd.name.c_str());
    }
}

// =============================================================================
// Phase E.1: Asset Operations - Handlers (Font)
// =============================================================================

void CommandServer::handleDecodeFont(const Command& cmd)
{
    LOGI("CommandServer: Handling DecodeFont command (requestID=%lld, size=%zu)",
         static_cast<long long>(cmd.requestID), cmd.bytes.size());

    // TODO: Implement actual font decoding using Rive font decoders
    // For now, return an error since full implementation requires:
    // - rive::Font creation
    // - Proper decoder integration (TTF, OTF)
    
    Message msg(MessageType::FontError, cmd.requestID);
    msg.error = "Font decoding not yet implemented";
    enqueueMessage(std::move(msg));
}

void CommandServer::handleDeleteFont(const Command& cmd)
{
    LOGI("CommandServer: Handling DeleteFont command (handle=%lld)",
         static_cast<long long>(cmd.handle));

    auto it = m_fonts.find(cmd.handle);
    if (it != m_fonts.end()) {
        // TODO: Properly delete the font data when implemented
        m_fonts.erase(it);
        LOGI("CommandServer: Font deleted successfully (handle=%lld)",
             static_cast<long long>(cmd.handle));
    } else {
        LOGW("CommandServer: Attempted to delete non-existent font (handle=%lld)",
             static_cast<long long>(cmd.handle));
    }
}

void CommandServer::handleRegisterFont(const Command& cmd)
{
    LOGI("CommandServer: Handling RegisterFont command (name=%s, handle=%lld)",
         cmd.name.c_str(), static_cast<long long>(cmd.handle));

    // Verify the font handle exists
    auto it = m_fonts.find(cmd.handle);
    if (it == m_fonts.end()) {
        LOGW("CommandServer: Cannot register non-existent font (handle=%lld)",
             static_cast<long long>(cmd.handle));
        return;
    }

    // Register the font by name
    m_registeredFonts[cmd.name] = cmd.handle;
    LOGI("CommandServer: Font registered (name=%s, handle=%lld)",
         cmd.name.c_str(), static_cast<long long>(cmd.handle));
}

void CommandServer::handleUnregisterFont(const Command& cmd)
{
    LOGI("CommandServer: Handling UnregisterFont command (name=%s)", cmd.name.c_str());

    auto it = m_registeredFonts.find(cmd.name);
    if (it != m_registeredFonts.end()) {
        m_registeredFonts.erase(it);
        LOGI("CommandServer: Font unregistered (name=%s)", cmd.name.c_str());
    } else {
        LOGW("CommandServer: Attempted to unregister non-existent font (name=%s)",
             cmd.name.c_str());
    }
}

} // namespace rive_android