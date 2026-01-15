#include "command_server.hpp"
#include "rive_log.hpp"

namespace rive_android {

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

} // namespace rive_android