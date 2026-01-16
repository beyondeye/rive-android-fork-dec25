#include "command_server.hpp"
#include "rive_log.hpp"
#include "rive/viewmodel/viewmodel.hpp"
#include "rive/viewmodel/viewmodel_property.hpp"
#include "rive/viewmodel/data_enum.hpp"
#include "utils/no_op_factory.hpp"

namespace rive_android {

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

// =============================================================================
// Phase E.2: File Introspection APIs
// =============================================================================

void CommandServer::getViewModelInstanceNames(int64_t requestID, int64_t fileHandle, const std::string& viewModelName)
{
    LOGI("CommandServer: Enqueuing GetViewModelInstanceNames command (requestID=%lld, fileHandle=%lld, vmName=%s)",
         static_cast<long long>(requestID), static_cast<long long>(fileHandle), viewModelName.c_str());
    
    Command cmd(CommandType::GetViewModelInstanceNames, requestID);
    cmd.handle = fileHandle;
    cmd.viewModelName = viewModelName;
    
    enqueueCommand(std::move(cmd));
}

void CommandServer::getViewModelProperties(int64_t requestID, int64_t fileHandle, const std::string& viewModelName)
{
    LOGI("CommandServer: Enqueuing GetViewModelProperties command (requestID=%lld, fileHandle=%lld, vmName=%s)",
         static_cast<long long>(requestID), static_cast<long long>(fileHandle), viewModelName.c_str());
    
    Command cmd(CommandType::GetViewModelProperties, requestID);
    cmd.handle = fileHandle;
    cmd.viewModelName = viewModelName;
    
    enqueueCommand(std::move(cmd));
}

void CommandServer::getEnums(int64_t requestID, int64_t fileHandle)
{
    LOGI("CommandServer: Enqueuing GetEnums command (requestID=%lld, fileHandle=%lld)",
         static_cast<long long>(requestID), static_cast<long long>(fileHandle));
    
    Command cmd(CommandType::GetEnums, requestID);
    cmd.handle = fileHandle;
    
    enqueueCommand(std::move(cmd));
}

void CommandServer::handleGetViewModelInstanceNames(const Command& cmd)
{
    LOGI("CommandServer: Handling GetViewModelInstanceNames command (requestID=%lld, fileHandle=%lld, vmName=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.viewModelName.c_str());
    
    auto it = m_files.find(cmd.handle);
    if (it == m_files.end()) {
        LOGW("CommandServer: Invalid file handle: %lld", static_cast<long long>(cmd.handle));
        
        Message msg(MessageType::QueryError, cmd.requestID);
        msg.error = "Invalid file handle";
        enqueueMessage(std::move(msg));
        return;
    }
    
    auto& file = it->second;
    
    // Find the ViewModel by name
    rive::ViewModel* viewModel = nullptr;
    for (size_t i = 0; i < file->viewModelCount(); i++) {
        auto vm = file->viewModel(i);
        if (vm && vm->name() == cmd.viewModelName) {
            viewModel = vm;
            break;
        }
    }
    
    if (viewModel == nullptr) {
        LOGW("CommandServer: ViewModel not found: %s", cmd.viewModelName.c_str());
        
        Message msg(MessageType::QueryError, cmd.requestID);
        msg.error = "ViewModel not found: " + cmd.viewModelName;
        enqueueMessage(std::move(msg));
        return;
    }
    
    std::vector<std::string> names;
    
    // Get instance names from the ViewModel
    for (size_t i = 0; i < viewModel->instanceCount(); i++) {
        auto instance = viewModel->instance(i);
        if (instance) {
            names.push_back(instance->name());
        }
    }
    
    LOGI("CommandServer: Found %zu ViewModel instance names", names.size());
    
    Message msg(MessageType::ViewModelInstanceNamesListed, cmd.requestID);
    msg.stringList = std::move(names);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleGetViewModelProperties(const Command& cmd)
{
    LOGI("CommandServer: Handling GetViewModelProperties command (requestID=%lld, fileHandle=%lld, vmName=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.viewModelName.c_str());
    
    auto it = m_files.find(cmd.handle);
    if (it == m_files.end()) {
        LOGW("CommandServer: Invalid file handle: %lld", static_cast<long long>(cmd.handle));
        
        Message msg(MessageType::QueryError, cmd.requestID);
        msg.error = "Invalid file handle";
        enqueueMessage(std::move(msg));
        return;
    }
    
    auto& file = it->second;
    
    // Find the ViewModel by name
    rive::ViewModel* viewModel = nullptr;
    for (size_t i = 0; i < file->viewModelCount(); i++) {
        auto vm = file->viewModel(i);
        if (vm && vm->name() == cmd.viewModelName) {
            viewModel = vm;
            break;
        }
    }
    
    if (viewModel == nullptr) {
        LOGW("CommandServer: ViewModel not found: %s", cmd.viewModelName.c_str());
        
        Message msg(MessageType::QueryError, cmd.requestID);
        msg.error = "ViewModel not found: " + cmd.viewModelName;
        enqueueMessage(std::move(msg));
        return;
    }
    
    // Build a string list with alternating name/type pairs
    // Format: [name1, type1, name2, type2, ...]
    // This allows Kotlin to reconstruct ViewModelProperty objects
    std::vector<std::string> propertyData;
    
    // Use properties() which returns std::vector<ViewModelProperty*>
    auto props = viewModel->properties();
    for (auto* prop : props) {
        if (prop) {
            propertyData.push_back(prop->name());
            // Use coreType() to get the property type
            propertyData.push_back(std::to_string(static_cast<int>(prop->coreType())));
        }
    }
    
    LOGI("CommandServer: Found %zu ViewModel properties", propertyData.size() / 2);
    
    Message msg(MessageType::ViewModelPropertiesListed, cmd.requestID);
    msg.stringList = std::move(propertyData);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleGetEnums(const Command& cmd)
{
    LOGI("CommandServer: Handling GetEnums command (requestID=%lld, fileHandle=%lld)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle));
    
    auto it = m_files.find(cmd.handle);
    if (it == m_files.end()) {
        LOGW("CommandServer: Invalid file handle: %lld", static_cast<long long>(cmd.handle));
        
        Message msg(MessageType::QueryError, cmd.requestID);
        msg.error = "Invalid file handle";
        enqueueMessage(std::move(msg));
        return;
    }
    
    auto& file = it->second;
    
    // Build a string list with enum data
    // Format: [enumName1, valueCount1, value1, value2, ..., enumName2, valueCount2, ...]
    // This allows Kotlin to reconstruct RiveEnum objects
    std::vector<std::string> enumData;
    
    // Use enums() which returns const std::vector<DataEnum*>&
    const auto& fileEnums = file->enums();
    for (auto* riveEnum : fileEnums) {
        if (riveEnum) {
            enumData.push_back(riveEnum->enumName());
            auto& enumValues = riveEnum->values();
            enumData.push_back(std::to_string(enumValues.size()));
            
            for (size_t j = 0; j < enumValues.size(); j++) {
                enumData.push_back(riveEnum->value(static_cast<uint32_t>(j)));
            }
        }
    }
    
    LOGI("CommandServer: Found %zu enums", fileEnums.size());
    
    Message msg(MessageType::EnumsListed, cmd.requestID);
    msg.stringList = std::move(enumData);
    enqueueMessage(std::move(msg));
}

} // namespace rive_android