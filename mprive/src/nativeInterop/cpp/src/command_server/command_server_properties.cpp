#include "command_server.hpp"
#include "rive_log.hpp"
#include <algorithm>

namespace rive_android {

// =============================================================================
// Property Operations - Public API (Phase D.2)
// =============================================================================

void CommandServer::getNumberProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath)
{
    LOGI("CommandServer: Enqueuing GetNumberProperty command (requestID=%lld, vmiHandle=%lld, path=%s)",
         static_cast<long long>(requestID), static_cast<long long>(vmiHandle), propertyPath.c_str());

    Command cmd(CommandType::GetNumberProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;

    enqueueCommand(std::move(cmd));
}

void CommandServer::setNumberProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, float value)
{
    LOGI("CommandServer: Enqueuing SetNumberProperty command (requestID=%lld, vmiHandle=%lld, path=%s, value=%f)",
         static_cast<long long>(requestID), static_cast<long long>(vmiHandle), propertyPath.c_str(), value);

    Command cmd(CommandType::SetNumberProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.floatValue = value;

    enqueueCommand(std::move(cmd));
}

void CommandServer::getStringProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath)
{
    LOGI("CommandServer: Enqueuing GetStringProperty command (requestID=%lld, vmiHandle=%lld, path=%s)",
         static_cast<long long>(requestID), static_cast<long long>(vmiHandle), propertyPath.c_str());

    Command cmd(CommandType::GetStringProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;

    enqueueCommand(std::move(cmd));
}

void CommandServer::setStringProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, const std::string& value)
{
    LOGI("CommandServer: Enqueuing SetStringProperty command (requestID=%lld, vmiHandle=%lld, path=%s, value=%s)",
         static_cast<long long>(requestID), static_cast<long long>(vmiHandle), propertyPath.c_str(), value.c_str());

    Command cmd(CommandType::SetStringProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.stringValue = value;

    enqueueCommand(std::move(cmd));
}

void CommandServer::getBooleanProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath)
{
    LOGI("CommandServer: Enqueuing GetBooleanProperty command (requestID=%lld, vmiHandle=%lld, path=%s)",
         static_cast<long long>(requestID), static_cast<long long>(vmiHandle), propertyPath.c_str());

    Command cmd(CommandType::GetBooleanProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;

    enqueueCommand(std::move(cmd));
}

void CommandServer::setBooleanProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, bool value)
{
    LOGI("CommandServer: Enqueuing SetBooleanProperty command (requestID=%lld, vmiHandle=%lld, path=%s, value=%d)",
         static_cast<long long>(requestID), static_cast<long long>(vmiHandle), propertyPath.c_str(), value ? 1 : 0);

    Command cmd(CommandType::SetBooleanProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.boolValue = value;

    enqueueCommand(std::move(cmd));
}

// =============================================================================
// Property Operations - Handler Implementations (Phase D.2)
// =============================================================================

void CommandServer::handleGetNumberProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling GetNumberProperty command (requestID=%lld, vmiHandle=%lld, path=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.propertyPath.c_str());

    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        LOGW("CommandServer: Invalid VMI handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& vmi = it->second;
    auto* prop = vmi->propertyNumber(cmd.propertyPath);
    if (!prop) {
        LOGW("CommandServer: Number property not found: %s", cmd.propertyPath.c_str());

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Number property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    float value = prop->value();

    LOGI("CommandServer: GetNumberProperty succeeded (path=%s, value=%f)",
         cmd.propertyPath.c_str(), value);

    Message msg(MessageType::NumberPropertyValue, cmd.requestID);
    msg.floatValue = value;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleSetNumberProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling SetNumberProperty command (requestID=%lld, vmiHandle=%lld, path=%s, value=%f)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle),
         cmd.propertyPath.c_str(), cmd.floatValue);

    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        LOGW("CommandServer: Invalid VMI handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& vmi = it->second;
    auto* prop = vmi->propertyNumber(cmd.propertyPath);
    if (!prop) {
        LOGW("CommandServer: Number property not found: %s", cmd.propertyPath.c_str());

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Number property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    prop->value(cmd.floatValue);

    LOGI("CommandServer: SetNumberProperty succeeded (path=%s, value=%f)",
         cmd.propertyPath.c_str(), cmd.floatValue);

    // Emit update to subscribers (Phase D.4)
    emitPropertyUpdateIfSubscribed(cmd.handle, cmd.propertyPath, PropertyDataType::NUMBER);

    Message msg(MessageType::PropertySetSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleGetStringProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling GetStringProperty command (requestID=%lld, vmiHandle=%lld, path=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.propertyPath.c_str());

    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        LOGW("CommandServer: Invalid VMI handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& vmi = it->second;
    auto* prop = vmi->propertyString(cmd.propertyPath);
    if (!prop) {
        LOGW("CommandServer: String property not found: %s", cmd.propertyPath.c_str());

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "String property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    const std::string& value = prop->value();

    LOGI("CommandServer: GetStringProperty succeeded (path=%s, value=%s)",
         cmd.propertyPath.c_str(), value.c_str());

    Message msg(MessageType::StringPropertyValue, cmd.requestID);
    msg.stringValue = value;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleSetStringProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling SetStringProperty command (requestID=%lld, vmiHandle=%lld, path=%s, value=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle),
         cmd.propertyPath.c_str(), cmd.stringValue.c_str());

    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        LOGW("CommandServer: Invalid VMI handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& vmi = it->second;
    auto* prop = vmi->propertyString(cmd.propertyPath);
    if (!prop) {
        LOGW("CommandServer: String property not found: %s", cmd.propertyPath.c_str());

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "String property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    prop->value(cmd.stringValue);

    LOGI("CommandServer: SetStringProperty succeeded (path=%s, value=%s)",
         cmd.propertyPath.c_str(), cmd.stringValue.c_str());

    // Emit update to subscribers (Phase D.4)
    emitPropertyUpdateIfSubscribed(cmd.handle, cmd.propertyPath, PropertyDataType::STRING);

    Message msg(MessageType::PropertySetSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleGetBooleanProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling GetBooleanProperty command (requestID=%lld, vmiHandle=%lld, path=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.propertyPath.c_str());

    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        LOGW("CommandServer: Invalid VMI handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& vmi = it->second;
    auto* prop = vmi->propertyBoolean(cmd.propertyPath);
    if (!prop) {
        LOGW("CommandServer: Boolean property not found: %s", cmd.propertyPath.c_str());

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Boolean property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    bool value = prop->value();

    LOGI("CommandServer: GetBooleanProperty succeeded (path=%s, value=%d)",
         cmd.propertyPath.c_str(), value ? 1 : 0);

    Message msg(MessageType::BooleanPropertyValue, cmd.requestID);
    msg.boolValue = value;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleSetBooleanProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling SetBooleanProperty command (requestID=%lld, vmiHandle=%lld, path=%s, value=%d)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle),
         cmd.propertyPath.c_str(), cmd.boolValue ? 1 : 0);

    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        LOGW("CommandServer: Invalid VMI handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& vmi = it->second;
    auto* prop = vmi->propertyBoolean(cmd.propertyPath);
    if (!prop) {
        LOGW("CommandServer: Boolean property not found: %s", cmd.propertyPath.c_str());

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Boolean property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    prop->value(cmd.boolValue);

    LOGI("CommandServer: SetBooleanProperty succeeded (path=%s, value=%d)",
         cmd.propertyPath.c_str(), cmd.boolValue ? 1 : 0);

    // Emit update to subscribers (Phase D.4)
    emitPropertyUpdateIfSubscribed(cmd.handle, cmd.propertyPath, PropertyDataType::BOOLEAN);

    Message msg(MessageType::PropertySetSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

// =============================================================================
// Additional Property Types - Public API (Phase D.3)
// =============================================================================

void CommandServer::getEnumProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath)
{
    LOGI("CommandServer: Enqueuing GetEnumProperty command (requestID=%lld, vmiHandle=%lld, path=%s)",
         static_cast<long long>(requestID), static_cast<long long>(vmiHandle), propertyPath.c_str());

    Command cmd(CommandType::GetEnumProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;

    enqueueCommand(std::move(cmd));
}

void CommandServer::setEnumProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, const std::string& value)
{
    LOGI("CommandServer: Enqueuing SetEnumProperty command (requestID=%lld, vmiHandle=%lld, path=%s, value=%s)",
         static_cast<long long>(requestID), static_cast<long long>(vmiHandle), propertyPath.c_str(), value.c_str());

    Command cmd(CommandType::SetEnumProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.stringValue = value;

    enqueueCommand(std::move(cmd));
}

void CommandServer::getColorProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath)
{
    LOGI("CommandServer: Enqueuing GetColorProperty command (requestID=%lld, vmiHandle=%lld, path=%s)",
         static_cast<long long>(requestID), static_cast<long long>(vmiHandle), propertyPath.c_str());

    Command cmd(CommandType::GetColorProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;

    enqueueCommand(std::move(cmd));
}

void CommandServer::setColorProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath, int32_t value)
{
    LOGI("CommandServer: Enqueuing SetColorProperty command (requestID=%lld, vmiHandle=%lld, path=%s, value=0x%08X)",
         static_cast<long long>(requestID), static_cast<long long>(vmiHandle), propertyPath.c_str(), value);

    Command cmd(CommandType::SetColorProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.colorValue = value;

    enqueueCommand(std::move(cmd));
}

void CommandServer::fireTriggerProperty(int64_t requestID, int64_t vmiHandle, const std::string& propertyPath)
{
    LOGI("CommandServer: Enqueuing FireTriggerProperty command (requestID=%lld, vmiHandle=%lld, path=%s)",
         static_cast<long long>(requestID), static_cast<long long>(vmiHandle), propertyPath.c_str());

    Command cmd(CommandType::FireTriggerProperty, requestID);
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;

    enqueueCommand(std::move(cmd));
}

// =============================================================================
// Property Subscriptions - Public API (Phase D.4)
// =============================================================================

void CommandServer::subscribeToProperty(int64_t vmiHandle, const std::string& propertyPath, int32_t propertyType)
{
    LOGI("CommandServer: Enqueuing SubscribeToProperty command (vmiHandle=%lld, path=%s, type=%d)",
         static_cast<long long>(vmiHandle), propertyPath.c_str(), propertyType);

    Command cmd(CommandType::SubscribeToProperty, 0);  // No requestID needed
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.propertyType = propertyType;

    enqueueCommand(std::move(cmd));
}

void CommandServer::unsubscribeFromProperty(int64_t vmiHandle, const std::string& propertyPath, int32_t propertyType)
{
    LOGI("CommandServer: Enqueuing UnsubscribeFromProperty command (vmiHandle=%lld, path=%s, type=%d)",
         static_cast<long long>(vmiHandle), propertyPath.c_str(), propertyType);

    Command cmd(CommandType::UnsubscribeFromProperty, 0);  // No requestID needed
    cmd.handle = vmiHandle;
    cmd.propertyPath = propertyPath;
    cmd.propertyType = propertyType;

    enqueueCommand(std::move(cmd));
}

// =============================================================================
// Additional Property Types - Handler Implementations (Phase D.3)
// =============================================================================

void CommandServer::handleGetEnumProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling GetEnumProperty command (requestID=%lld, vmiHandle=%lld, path=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.propertyPath.c_str());

    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        LOGW("CommandServer: Invalid VMI handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& vmi = it->second;
    auto* prop = vmi->propertyEnum(cmd.propertyPath);
    if (!prop) {
        LOGW("CommandServer: Enum property not found: %s", cmd.propertyPath.c_str());

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Enum property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    std::string value = prop->value();

    LOGI("CommandServer: GetEnumProperty succeeded (path=%s, value=%s)",
         cmd.propertyPath.c_str(), value.c_str());

    Message msg(MessageType::EnumPropertyValue, cmd.requestID);
    msg.stringValue = value;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleSetEnumProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling SetEnumProperty command (requestID=%lld, vmiHandle=%lld, path=%s, value=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle),
         cmd.propertyPath.c_str(), cmd.stringValue.c_str());

    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        LOGW("CommandServer: Invalid VMI handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& vmi = it->second;
    auto* prop = vmi->propertyEnum(cmd.propertyPath);
    if (!prop) {
        LOGW("CommandServer: Enum property not found: %s", cmd.propertyPath.c_str());

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Enum property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    prop->value(cmd.stringValue);

    LOGI("CommandServer: SetEnumProperty succeeded (path=%s, value=%s)",
         cmd.propertyPath.c_str(), cmd.stringValue.c_str());

    // Emit update to subscribers (Phase D.4)
    emitPropertyUpdateIfSubscribed(cmd.handle, cmd.propertyPath, PropertyDataType::ENUM);

    Message msg(MessageType::PropertySetSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleGetColorProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling GetColorProperty command (requestID=%lld, vmiHandle=%lld, path=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.propertyPath.c_str());

    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        LOGW("CommandServer: Invalid VMI handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& vmi = it->second;
    auto* prop = vmi->propertyColor(cmd.propertyPath);
    if (!prop) {
        LOGW("CommandServer: Color property not found: %s", cmd.propertyPath.c_str());

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Color property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    int value = prop->value();

    LOGI("CommandServer: GetColorProperty succeeded (path=%s, value=0x%08X)",
         cmd.propertyPath.c_str(), value);

    Message msg(MessageType::ColorPropertyValue, cmd.requestID);
    msg.colorValue = value;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleSetColorProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling SetColorProperty command (requestID=%lld, vmiHandle=%lld, path=%s, value=0x%08X)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle),
         cmd.propertyPath.c_str(), cmd.colorValue);

    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        LOGW("CommandServer: Invalid VMI handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& vmi = it->second;
    auto* prop = vmi->propertyColor(cmd.propertyPath);
    if (!prop) {
        LOGW("CommandServer: Color property not found: %s", cmd.propertyPath.c_str());

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Color property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    prop->value(cmd.colorValue);

    LOGI("CommandServer: SetColorProperty succeeded (path=%s, value=0x%08X)",
         cmd.propertyPath.c_str(), cmd.colorValue);

    // Emit update to subscribers (Phase D.4)
    emitPropertyUpdateIfSubscribed(cmd.handle, cmd.propertyPath, PropertyDataType::COLOR);

    Message msg(MessageType::PropertySetSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleFireTriggerProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling FireTriggerProperty command (requestID=%lld, vmiHandle=%lld, path=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.propertyPath.c_str());

    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        LOGW("CommandServer: Invalid VMI handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Invalid ViewModelInstance handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& vmi = it->second;
    auto* prop = vmi->propertyTrigger(cmd.propertyPath);
    if (!prop) {
        LOGW("CommandServer: Trigger property not found: %s", cmd.propertyPath.c_str());

        Message msg(MessageType::PropertyError, cmd.requestID);
        msg.error = "Trigger property not found: " + cmd.propertyPath;
        enqueueMessage(std::move(msg));
        return;
    }

    prop->trigger();

    LOGI("CommandServer: FireTriggerProperty succeeded (path=%s)",
         cmd.propertyPath.c_str());

    // Emit update to subscribers (Phase D.4)
    emitPropertyUpdateIfSubscribed(cmd.handle, cmd.propertyPath, PropertyDataType::TRIGGER);

    Message msg(MessageType::TriggerFired, cmd.requestID);
    enqueueMessage(std::move(msg));
}

// =============================================================================
// Property Subscriptions - Handler Implementations (Phase D.4)
// =============================================================================

void CommandServer::handleSubscribeToProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling SubscribeToProperty command (vmiHandle=%lld, path=%s, type=%d)",
         static_cast<long long>(cmd.handle), cmd.propertyPath.c_str(), cmd.propertyType);

    // Validate the VMI handle exists
    auto it = m_viewModelInstances.find(cmd.handle);
    if (it == m_viewModelInstances.end()) {
        LOGW("CommandServer: Invalid VMI handle for subscription: %lld", static_cast<long long>(cmd.handle));
        return;
    }

    PropertySubscription sub;
    sub.vmiHandle = cmd.handle;
    sub.propertyPath = cmd.propertyPath;
    sub.propertyType = static_cast<PropertyDataType>(cmd.propertyType);

    // Check if already subscribed (avoid duplicates)
    {
        std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
        for (const auto& existing : m_propertySubscriptions) {
            if (existing == sub) {
                LOGI("CommandServer: Already subscribed to property (vmiHandle=%lld, path=%s)",
                     static_cast<long long>(cmd.handle), cmd.propertyPath.c_str());
                return;
            }
        }
        m_propertySubscriptions.push_back(sub);
    }

    LOGI("CommandServer: Subscribed to property (vmiHandle=%lld, path=%s, type=%d)",
         static_cast<long long>(cmd.handle), cmd.propertyPath.c_str(), cmd.propertyType);
}

void CommandServer::handleUnsubscribeFromProperty(const Command& cmd)
{
    LOGI("CommandServer: Handling UnsubscribeFromProperty command (vmiHandle=%lld, path=%s, type=%d)",
         static_cast<long long>(cmd.handle), cmd.propertyPath.c_str(), cmd.propertyType);

    PropertySubscription sub;
    sub.vmiHandle = cmd.handle;
    sub.propertyPath = cmd.propertyPath;
    sub.propertyType = static_cast<PropertyDataType>(cmd.propertyType);

    {
        std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
        auto it = std::find(m_propertySubscriptions.begin(), m_propertySubscriptions.end(), sub);
        if (it != m_propertySubscriptions.end()) {
            m_propertySubscriptions.erase(it);
            LOGI("CommandServer: Unsubscribed from property (vmiHandle=%lld, path=%s)",
                 static_cast<long long>(cmd.handle), cmd.propertyPath.c_str());
        } else {
            LOGI("CommandServer: Subscription not found (vmiHandle=%lld, path=%s)",
                 static_cast<long long>(cmd.handle), cmd.propertyPath.c_str());
        }
    }
}

void CommandServer::emitPropertyUpdateIfSubscribed(int64_t vmiHandle, const std::string& propertyPath, PropertyDataType propertyType)
{
    // Check if this property is subscribed
    bool isSubscribed = false;
    {
        std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
        for (const auto& sub : m_propertySubscriptions) {
            if (sub.vmiHandle == vmiHandle && sub.propertyPath == propertyPath && sub.propertyType == propertyType) {
                isSubscribed = true;
                break;
            }
        }
    }

    if (!isSubscribed) {
        return;
    }

    // Get the VMI
    auto it = m_viewModelInstances.find(vmiHandle);
    if (it == m_viewModelInstances.end()) {
        return;
    }

    auto& vmi = it->second;

    // Emit update message based on property type
    switch (propertyType) {
        case PropertyDataType::NUMBER: {
            auto* prop = vmi->propertyNumber(propertyPath);
            if (prop) {
                Message msg(MessageType::NumberPropertyUpdated, 0);
                msg.vmiHandle = vmiHandle;
                msg.propertyPath = propertyPath;
                msg.floatValue = prop->value();
                enqueueMessage(std::move(msg));
            }
            break;
        }
        case PropertyDataType::STRING: {
            auto* prop = vmi->propertyString(propertyPath);
            if (prop) {
                Message msg(MessageType::StringPropertyUpdated, 0);
                msg.vmiHandle = vmiHandle;
                msg.propertyPath = propertyPath;
                msg.stringValue = prop->value();
                enqueueMessage(std::move(msg));
            }
            break;
        }
        case PropertyDataType::BOOLEAN: {
            auto* prop = vmi->propertyBoolean(propertyPath);
            if (prop) {
                Message msg(MessageType::BooleanPropertyUpdated, 0);
                msg.vmiHandle = vmiHandle;
                msg.propertyPath = propertyPath;
                msg.boolValue = prop->value();
                enqueueMessage(std::move(msg));
            }
            break;
        }
        case PropertyDataType::ENUM: {
            auto* prop = vmi->propertyEnum(propertyPath);
            if (prop) {
                Message msg(MessageType::EnumPropertyUpdated, 0);
                msg.vmiHandle = vmiHandle;
                msg.propertyPath = propertyPath;
                msg.stringValue = prop->value();
                enqueueMessage(std::move(msg));
            }
            break;
        }
        case PropertyDataType::COLOR: {
            auto* prop = vmi->propertyColor(propertyPath);
            if (prop) {
                Message msg(MessageType::ColorPropertyUpdated, 0);
                msg.vmiHandle = vmiHandle;
                msg.propertyPath = propertyPath;
                msg.colorValue = prop->value();
                enqueueMessage(std::move(msg));
            }
            break;
        }
        case PropertyDataType::TRIGGER: {
            // Triggers don't have a value, just emit that it was fired
            Message msg(MessageType::TriggerPropertyFired, 0);
            msg.vmiHandle = vmiHandle;
            msg.propertyPath = propertyPath;
            enqueueMessage(std::move(msg));
            break;
        }
        default:
            LOGW("CommandServer: Unsupported property type for subscription update: %d",
                 static_cast<int>(propertyType));
            break;
    }
}

} // namespace rive_android