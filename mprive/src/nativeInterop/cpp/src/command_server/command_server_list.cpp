#include "command_server.hpp"
#include "rive_log.hpp"

namespace rive_android {

// =============================================================================
// Phase D.5: List Operations - Public API
// =============================================================================

void CommandServer::getListSize(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath)
{
    Command cmd(CommandType::GetListSize, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    enqueueCommand(std::move(cmd));
}

void CommandServer::getListItem(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int32_t index)
{
    Command cmd(CommandType::GetListItem, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.listIndex = index;
    enqueueCommand(std::move(cmd));
}

void CommandServer::addListItem(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int64_t itemHandle)
{
    Command cmd(CommandType::AddListItem, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.itemHandle = itemHandle;
    enqueueCommand(std::move(cmd));
}

void CommandServer::addListItemAt(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int32_t index, int64_t itemHandle)
{
    Command cmd(CommandType::AddListItemAt, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.listIndex = index;
    cmd.itemHandle = itemHandle;
    enqueueCommand(std::move(cmd));
}

void CommandServer::removeListItem(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int64_t itemHandle)
{
    Command cmd(CommandType::RemoveListItem, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.itemHandle = itemHandle;
    enqueueCommand(std::move(cmd));
}

void CommandServer::removeListItemAt(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int32_t index)
{
    Command cmd(CommandType::RemoveListItemAt, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.listIndex = index;
    enqueueCommand(std::move(cmd));
}

void CommandServer::swapListItems(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int32_t indexA, int32_t indexB)
{
    Command cmd(CommandType::SwapListItems, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.listIndex = indexA;
    cmd.listIndexB = indexB;
    enqueueCommand(std::move(cmd));
}

// =============================================================================
// Phase D.5: Nested VMI Operations - Public API
// =============================================================================

void CommandServer::getInstanceProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath)
{
    Command cmd(CommandType::GetInstanceProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    enqueueCommand(std::move(cmd));
}

void CommandServer::setInstanceProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int64_t nestedHandle)
{
    Command cmd(CommandType::SetInstanceProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.nestedHandle = nestedHandle;
    enqueueCommand(std::move(cmd));
}

// =============================================================================
// Phase D.5: Asset Property Operations - Public API
// =============================================================================

void CommandServer::setImageProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int64_t imageHandle)
{
    Command cmd(CommandType::SetImageProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.assetHandle = imageHandle;
    enqueueCommand(std::move(cmd));
}

void CommandServer::setArtboardProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int64_t fileHandle, int64_t artboardHandle)
{
    Command cmd(CommandType::SetArtboardProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.fileHandle = fileHandle;
    cmd.assetHandle = artboardHandle;
    enqueueCommand(std::move(cmd));
}

// =============================================================================
// Phase D.5: List Operations - Handlers
// =============================================================================

void CommandServer::handleGetListSize(const Command& cmd)
{
    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto* listProp = it->second->propertyList(cmd.propertyPath);
    if (!listProp) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "List property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    Message msg(MessageType::ListSizeResult, cmd.requestID);
    msg.intValue = static_cast<int32_t>(listProp->size());
    enqueueMessage(std::move(msg));
}

void CommandServer::handleGetListItem(const Command& cmd)
{
    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto* listProp = it->second->propertyList(cmd.propertyPath);
    if (!listProp) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "List property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    if (cmd.listIndex < 0 || static_cast<size_t>(cmd.listIndex) >= listProp->size()) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Index out of bounds: " + std::to_string(cmd.listIndex);
        enqueueMessage(std::move(msg));
        return;
    }

    auto itemVmi = listProp->instanceAt(cmd.listIndex);
    if (!itemVmi) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Failed to get item at index: " + std::to_string(cmd.listIndex);
        enqueueMessage(std::move(msg));
        return;
    }

    // Store the item in our map and return a handle
    int64_t handle = m_nextHandle.fetch_add(1);
    m_viewModelInstances[handle] = itemVmi;

    Message msg(MessageType::ListItemResult, cmd.requestID);
    msg.handle = handle;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleAddListItem(const Command& cmd)
{
    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto* listProp = it->second->propertyList(cmd.propertyPath);
    if (!listProp) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "List property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    auto itemIt = m_viewModelInstances.find(cmd.itemHandle);
    if (itemIt == m_viewModelInstances.end()) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Invalid item ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    listProp->addInstance(itemIt->second.get());

    Message msg(MessageType::ListOperationSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleAddListItemAt(const Command& cmd)
{
    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto* listProp = it->second->propertyList(cmd.propertyPath);
    if (!listProp) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "List property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    auto itemIt = m_viewModelInstances.find(cmd.itemHandle);
    if (itemIt == m_viewModelInstances.end()) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Invalid item ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    bool success = listProp->addInstanceAt(itemIt->second.get(), cmd.listIndex);
    if (!success) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Failed to add item at index: " + std::to_string(cmd.listIndex);
        enqueueMessage(std::move(msg));
        return;
    }

    Message msg(MessageType::ListOperationSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleRemoveListItem(const Command& cmd)
{
    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto* listProp = it->second->propertyList(cmd.propertyPath);
    if (!listProp) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "List property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    auto itemIt = m_viewModelInstances.find(cmd.itemHandle);
    if (itemIt == m_viewModelInstances.end()) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Invalid item ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    listProp->removeInstance(itemIt->second.get());

    Message msg(MessageType::ListOperationSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleRemoveListItemAt(const Command& cmd)
{
    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto* listProp = it->second->propertyList(cmd.propertyPath);
    if (!listProp) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "List property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    if (cmd.listIndex < 0 || static_cast<size_t>(cmd.listIndex) >= listProp->size()) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Index out of bounds: " + std::to_string(cmd.listIndex);
        enqueueMessage(std::move(msg));
        return;
    }

    listProp->removeInstanceAt(cmd.listIndex);

    Message msg(MessageType::ListOperationSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleSwapListItems(const Command& cmd)
{
    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto* listProp = it->second->propertyList(cmd.propertyPath);
    if (!listProp) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "List property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    size_t size = listProp->size();
    if (cmd.listIndex < 0 || static_cast<size_t>(cmd.listIndex) >= size ||
        cmd.listIndexB < 0 || static_cast<size_t>(cmd.listIndexB) >= size) {
        Message msg(MessageType::ListOperationError, cmd.requestID);
        msg.error = "Index out of bounds for swap";
        enqueueMessage(std::move(msg));
        return;
    }

    listProp->swap(static_cast<uint32_t>(cmd.listIndex), static_cast<uint32_t>(cmd.listIndexB));

    Message msg(MessageType::ListOperationSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

// =============================================================================
// Phase D.5: Nested VMI Operations - Handlers
// =============================================================================

void CommandServer::handleGetInstanceProperty(const Command& cmd)
{
    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        Message msg(MessageType::InstancePropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto nestedVmi = it->second->propertyViewModel(cmd.propertyPath);
    if (!nestedVmi) {
        Message msg(MessageType::InstancePropertyError, cmd.requestID);
        msg.error = "Instance property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    // Store the nested VMI in our map and return a handle
    int64_t handle = m_nextHandle.fetch_add(1);
    m_viewModelInstances[handle] = nestedVmi;

    Message msg(MessageType::InstancePropertyResult, cmd.requestID);
    msg.handle = handle;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleSetInstanceProperty(const Command& cmd)
{
    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        Message msg(MessageType::InstancePropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto nestedIt = m_viewModelInstances.find(cmd.nestedHandle);
    if (nestedIt == m_viewModelInstances.end()) {
        Message msg(MessageType::InstancePropertyError, cmd.requestID);
        msg.error = "Invalid nested ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    bool success = it->second->replaceViewModel(cmd.propertyPath, nestedIt->second.get());
    if (!success) {
        Message msg(MessageType::InstancePropertyError, cmd.requestID);
        msg.error = "Failed to set instance property: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    Message msg(MessageType::InstancePropertySetSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

// =============================================================================
// Phase D.5: Asset Property Operations - Handlers
// =============================================================================

void CommandServer::handleSetImageProperty(const Command& cmd)
{
    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        Message msg(MessageType::AssetPropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto* imageProp = it->second->propertyImage(cmd.propertyPath);
    if (!imageProp) {
        Message msg(MessageType::AssetPropertyError, cmd.requestID);
        msg.error = "Image property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    // Note: For now we just set to nullptr since we don't have an image storage map yet
    // This is a placeholder - full implementation would look up the image from a storage map
    if (cmd.assetHandle == 0) {
        imageProp->value(nullptr);
    } else {
        // TODO: Look up image from m_images map when implemented
        Message msg(MessageType::AssetPropertyError, cmd.requestID);
        msg.error = "Image storage not yet implemented";
        enqueueMessage(std::move(msg));
        return;
    }

    Message msg(MessageType::AssetPropertySetSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleSetArtboardProperty(const Command& cmd)
{
    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        Message msg(MessageType::AssetPropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto* artboardProp = it->second->propertyArtboard(cmd.propertyPath);
    if (!artboardProp) {
        Message msg(MessageType::AssetPropertyError, cmd.requestID);
        msg.error = "Artboard property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    if (cmd.assetHandle == 0) {
        // Clear the artboard property
        artboardProp->value(nullptr);
    } else {
        // Look up file from our storage map
        auto fileIt = m_files.find(cmd.fileHandle);
        if (fileIt == m_files.end()) {
            Message msg(MessageType::AssetPropertyError, cmd.requestID);
            msg.error = "Invalid file handle";
            enqueueMessage(std::move(msg));
            return;
        }

        // Look up artboard from our storage map
        auto artboardIt = m_artboards.find(cmd.assetHandle);
        if (artboardIt == m_artboards.end()) {
            Message msg(MessageType::AssetPropertyError, cmd.requestID);
            msg.error = "Invalid artboard handle";
            enqueueMessage(std::move(msg));
            return;
        }

        // Get the artboard as a raw pointer (we need the base Artboard, not ArtboardInstance)
        rive::Artboard* artboard = artboardIt->second.get();

        // Create a BindableArtboard using the file's internal method
        auto bindableArtboard = fileIt->second->internalBindableArtboardFromArtboard(artboard);
        artboardProp->value(bindableArtboard);
    }

    Message msg(MessageType::AssetPropertySetSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

} // namespace rive_android