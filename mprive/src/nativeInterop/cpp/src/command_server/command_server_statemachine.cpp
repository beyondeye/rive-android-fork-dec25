#include "command_server.hpp"
#include "rive_log.hpp"
#include "rive/animation/state_machine_input_instance.hpp"
#include "rive/animation/state_machine_bool.hpp"
#include "rive/animation/state_machine_number.hpp"
#include "rive/animation/state_machine_trigger.hpp"

namespace rive_android {

void CommandServer::createDefaultStateMachine(int64_t requestID, int64_t artboardHandle)
{
    LOGI("CommandServer: Enqueuing CreateDefaultStateMachine command (requestID=%lld, artboardHandle=%lld)",
         static_cast<long long>(requestID), static_cast<long long>(artboardHandle));
    
    Command cmd(CommandType::CreateDefaultStateMachine, requestID);
    cmd.handle = artboardHandle;
    
    enqueueCommand(std::move(cmd));
}

void CommandServer::createStateMachineByName(int64_t requestID, int64_t artboardHandle, const std::string& name)
{
    LOGI("CommandServer: Enqueuing CreateStateMachineByName command (requestID=%lld, artboardHandle=%lld, name=%s)",
         static_cast<long long>(requestID), static_cast<long long>(artboardHandle), name.c_str());
    
    Command cmd(CommandType::CreateStateMachineByName, requestID);
    cmd.handle = artboardHandle;
    cmd.name = name;
    
    enqueueCommand(std::move(cmd));
}

void CommandServer::advanceStateMachine(int64_t requestID, int64_t smHandle, float deltaTime)
{
    LOGI("CommandServer: Enqueuing AdvanceStateMachine command (requestID=%lld, smHandle=%lld, deltaTime=%f)",
         static_cast<long long>(requestID), static_cast<long long>(smHandle), deltaTime);
    
    Command cmd(CommandType::AdvanceStateMachine, requestID);
    cmd.handle = smHandle;
    cmd.deltaTime = deltaTime;
    
    enqueueCommand(std::move(cmd));
}

void CommandServer::deleteStateMachine(int64_t requestID, int64_t smHandle)
{
    LOGI("CommandServer: Enqueuing DeleteStateMachine command (requestID=%lld, smHandle=%lld)",
         static_cast<long long>(requestID), static_cast<long long>(smHandle));
    
    Command cmd(CommandType::DeleteStateMachine, requestID);
    cmd.handle = smHandle;
    
    enqueueCommand(std::move(cmd));
}

void CommandServer::handleCreateDefaultStateMachine(const Command& cmd)
{
    LOGI("CommandServer: Handling CreateDefaultStateMachine command (requestID=%lld, artboardHandle=%lld)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle));
    
    auto it = m_artboards.find(cmd.handle);
    if (it == m_artboards.end()) {
        LOGW("CommandServer: Invalid artboard handle: %lld", static_cast<long long>(cmd.handle));
        
        Message msg(MessageType::StateMachineError, cmd.requestID);
        msg.error = "Invalid artboard handle";
        enqueueMessage(std::move(msg));
        return;
    }
    
    // Create the default state machine
    auto sm = it->second->defaultStateMachine();
    if (!sm) {
        LOGW("CommandServer: Failed to create default state machine");
        
        Message msg(MessageType::StateMachineError, cmd.requestID);
        msg.error = "Failed to create default state machine";
        enqueueMessage(std::move(msg));
        return;
    }
    
    // Generate a unique handle
    int64_t handle = m_nextHandle.fetch_add(1);
    
    // Store the state machine
    m_stateMachines[handle] = std::move(sm);
    
    LOGI("CommandServer: State machine created successfully (handle=%lld)", 
         static_cast<long long>(handle));
    
    // Send success message
    Message msg(MessageType::StateMachineCreated, cmd.requestID);
    msg.handle = handle;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleCreateStateMachineByName(const Command& cmd)
{
    LOGI("CommandServer: Handling CreateStateMachineByName command (requestID=%lld, artboardHandle=%lld, name=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.name.c_str());
    
    auto it = m_artboards.find(cmd.handle);
    if (it == m_artboards.end()) {
        LOGW("CommandServer: Invalid artboard handle: %lld", static_cast<long long>(cmd.handle));
        
        Message msg(MessageType::StateMachineError, cmd.requestID);
        msg.error = "Invalid artboard handle";
        enqueueMessage(std::move(msg));
        return;
    }
    
    // Create the state machine by name
    auto sm = it->second->stateMachineNamed(cmd.name);
    if (!sm) {
        LOGW("CommandServer: Failed to create state machine with name: %s", cmd.name.c_str());
        
        Message msg(MessageType::StateMachineError, cmd.requestID);
        msg.error = "State machine not found: " + cmd.name;
        enqueueMessage(std::move(msg));
        return;
    }
    
    // Generate a unique handle
    int64_t handle = m_nextHandle.fetch_add(1);
    
    // Store the state machine
    m_stateMachines[handle] = std::move(sm);
    
    LOGI("CommandServer: State machine created successfully (handle=%lld, name=%s)", 
         static_cast<long long>(handle), cmd.name.c_str());
    
    // Send success message
    Message msg(MessageType::StateMachineCreated, cmd.requestID);
    msg.handle = handle;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleAdvanceStateMachine(const Command& cmd)
{
    LOGI("CommandServer: Handling AdvanceStateMachine command (requestID=%lld, smHandle=%lld, deltaTime=%f)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.deltaTime);
    
    auto it = m_stateMachines.find(cmd.handle);
    if (it == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));
        
        Message msg(MessageType::StateMachineError, cmd.requestID);
        msg.error = "Invalid state machine handle";
        enqueueMessage(std::move(msg));
        return;
    }
    
    // Advance the state machine
    it->second->advance(cmd.deltaTime);
    
    // Check if the state machine has settled
    bool settled = !it->second->needsAdvance();
    
    LOGI("CommandServer: State machine advanced (handle=%lld, settled=%d)", 
         static_cast<long long>(cmd.handle), settled);
    
    // If settled, send settled message
    if (settled) {
        Message msg(MessageType::StateMachineSettled, cmd.requestID);
        msg.handle = cmd.handle;
        enqueueMessage(std::move(msg));
    }
}

void CommandServer::handleDeleteStateMachine(const Command& cmd)
{
    LOGI("CommandServer: Handling DeleteStateMachine command (requestID=%lld, handle=%lld)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle));

    auto it = m_stateMachines.find(cmd.handle);
    if (it != m_stateMachines.end()) {
        m_stateMachines.erase(it);

        LOGI("CommandServer: State machine deleted successfully (handle=%lld)",
             static_cast<long long>(cmd.handle));

        // Send success message
        Message msg(MessageType::StateMachineDeleted, cmd.requestID);
        msg.handle = cmd.handle;
        enqueueMessage(std::move(msg));
    } else {
        LOGW("CommandServer: Attempted to delete non-existent state machine (handle=%lld)",
             static_cast<long long>(cmd.handle));

        // Send error message
        Message msg(MessageType::StateMachineError, cmd.requestID);
        msg.error = "Invalid state machine handle";
        enqueueMessage(std::move(msg));
    }
}

// =============================================================================
// Phase C.4: State Machine Input Operations
// =============================================================================

void CommandServer::getInputCount(int64_t requestID, int64_t smHandle)
{
    LOGI("CommandServer: Enqueuing GetInputCount command (requestID=%lld, smHandle=%lld)",
         static_cast<long long>(requestID), static_cast<long long>(smHandle));

    Command cmd(CommandType::GetInputCount, requestID);
    cmd.handle = smHandle;

    enqueueCommand(std::move(cmd));
}

void CommandServer::getInputNames(int64_t requestID, int64_t smHandle)
{
    LOGI("CommandServer: Enqueuing GetInputNames command (requestID=%lld, smHandle=%lld)",
         static_cast<long long>(requestID), static_cast<long long>(smHandle));

    Command cmd(CommandType::GetInputNames, requestID);
    cmd.handle = smHandle;

    enqueueCommand(std::move(cmd));
}

void CommandServer::getInputInfo(int64_t requestID, int64_t smHandle, int32_t inputIndex)
{
    LOGI("CommandServer: Enqueuing GetInputInfo command (requestID=%lld, smHandle=%lld, index=%d)",
         static_cast<long long>(requestID), static_cast<long long>(smHandle), inputIndex);

    Command cmd(CommandType::GetInputInfo, requestID);
    cmd.handle = smHandle;
    cmd.inputIndex = inputIndex;

    enqueueCommand(std::move(cmd));
}

void CommandServer::getNumberInput(int64_t requestID, int64_t smHandle, const std::string& inputName)
{
    LOGI("CommandServer: Enqueuing GetNumberInput command (requestID=%lld, smHandle=%lld, name=%s)",
         static_cast<long long>(requestID), static_cast<long long>(smHandle), inputName.c_str());

    Command cmd(CommandType::GetNumberInput, requestID);
    cmd.handle = smHandle;
    cmd.inputName = inputName;

    enqueueCommand(std::move(cmd));
}

void CommandServer::setNumberInput(int64_t requestID, int64_t smHandle, const std::string& inputName, float value)
{
    LOGI("CommandServer: Enqueuing SetNumberInput command (requestID=%lld, smHandle=%lld, name=%s, value=%f)",
         static_cast<long long>(requestID), static_cast<long long>(smHandle), inputName.c_str(), value);

    Command cmd(CommandType::SetNumberInput, requestID);
    cmd.handle = smHandle;
    cmd.inputName = inputName;
    cmd.floatValue = value;

    enqueueCommand(std::move(cmd));
}

void CommandServer::getBooleanInput(int64_t requestID, int64_t smHandle, const std::string& inputName)
{
    LOGI("CommandServer: Enqueuing GetBooleanInput command (requestID=%lld, smHandle=%lld, name=%s)",
         static_cast<long long>(requestID), static_cast<long long>(smHandle), inputName.c_str());

    Command cmd(CommandType::GetBooleanInput, requestID);
    cmd.handle = smHandle;
    cmd.inputName = inputName;

    enqueueCommand(std::move(cmd));
}

void CommandServer::setBooleanInput(int64_t requestID, int64_t smHandle, const std::string& inputName, bool value)
{
    LOGI("CommandServer: Enqueuing SetBooleanInput command (requestID=%lld, smHandle=%lld, name=%s, value=%d)",
         static_cast<long long>(requestID), static_cast<long long>(smHandle), inputName.c_str(), value);

    Command cmd(CommandType::SetBooleanInput, requestID);
    cmd.handle = smHandle;
    cmd.inputName = inputName;
    cmd.boolValue = value;

    enqueueCommand(std::move(cmd));
}

void CommandServer::fireTrigger(int64_t requestID, int64_t smHandle, const std::string& inputName)
{
    LOGI("CommandServer: Enqueuing FireTrigger command (requestID=%lld, smHandle=%lld, name=%s)",
         static_cast<long long>(requestID), static_cast<long long>(smHandle), inputName.c_str());

    Command cmd(CommandType::FireTrigger, requestID);
    cmd.handle = smHandle;
    cmd.inputName = inputName;

    enqueueCommand(std::move(cmd));
}

// =============================================================================
// Input Handler Implementations
// =============================================================================

void CommandServer::handleGetInputCount(const Command& cmd)
{
    LOGI("CommandServer: Handling GetInputCount command (requestID=%lld, smHandle=%lld)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle));

    auto it = m_stateMachines.find(cmd.handle);
    if (it == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Invalid state machine handle";
        enqueueMessage(std::move(msg));
        return;
    }

    int32_t count = static_cast<int32_t>(it->second->inputCount());

    LOGI("CommandServer: Input count: %d", count);

    Message msg(MessageType::InputCountResult, cmd.requestID);
    msg.intValue = count;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleGetInputNames(const Command& cmd)
{
    LOGI("CommandServer: Handling GetInputNames command (requestID=%lld, smHandle=%lld)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle));

    auto it = m_stateMachines.find(cmd.handle);
    if (it == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Invalid state machine handle";
        enqueueMessage(std::move(msg));
        return;
    }

    std::vector<std::string> names;
    auto& sm = it->second;

    for (size_t i = 0; i < sm->inputCount(); i++) {
        auto input = sm->input(i);
        if (input) {
            names.push_back(input->name());
        }
    }

    LOGI("CommandServer: Found %zu input names", names.size());

    Message msg(MessageType::InputNamesListed, cmd.requestID);
    msg.stringList = std::move(names);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleGetInputInfo(const Command& cmd)
{
    LOGI("CommandServer: Handling GetInputInfo command (requestID=%lld, smHandle=%lld, index=%d)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.inputIndex);

    auto it = m_stateMachines.find(cmd.handle);
    if (it == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Invalid state machine handle";
        enqueueMessage(std::move(msg));
        return;
    }

    auto& sm = it->second;
    if (cmd.inputIndex < 0 || static_cast<size_t>(cmd.inputIndex) >= sm->inputCount()) {
        LOGW("CommandServer: Input index out of bounds: %d (count=%zu)", cmd.inputIndex, sm->inputCount());

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Input index out of bounds";
        enqueueMessage(std::move(msg));
        return;
    }

    auto input = sm->input(cmd.inputIndex);
    if (!input) {
        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Failed to get input at index";
        enqueueMessage(std::move(msg));
        return;
    }

    // Determine input type by checking the underlying StateMachineInput type
    InputType inputType = InputType::UNKNOWN;
    if (input->input()->is<rive::StateMachineNumber>()) {
        inputType = InputType::NUMBER;
    } else if (input->input()->is<rive::StateMachineBool>()) {
        inputType = InputType::BOOLEAN;
    } else if (input->input()->is<rive::StateMachineTrigger>()) {
        inputType = InputType::TRIGGER;
    }

    LOGI("CommandServer: Input info - name=%s, type=%d", input->name().c_str(), static_cast<int>(inputType));

    Message msg(MessageType::InputInfoResult, cmd.requestID);
    msg.inputName = input->name();
    msg.inputType = inputType;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleGetNumberInput(const Command& cmd)
{
    LOGI("CommandServer: Handling GetNumberInput command (requestID=%lld, smHandle=%lld, name=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.inputName.c_str());

    auto it = m_stateMachines.find(cmd.handle);
    if (it == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Invalid state machine handle";
        enqueueMessage(std::move(msg));
        return;
    }

    // Find the input by name
    auto& sm = it->second;
    rive::SMIInput* foundInput = nullptr;
    for (size_t i = 0; i < sm->inputCount(); i++) {
        auto input = sm->input(i);
        if (input && input->name() == cmd.inputName) {
            foundInput = input;
            break;
        }
    }

    if (!foundInput) {
        LOGW("CommandServer: Input not found: %s", cmd.inputName.c_str());

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Input not found: " + cmd.inputName;
        enqueueMessage(std::move(msg));
        return;
    }

    if (!foundInput->input()->is<rive::StateMachineNumber>()) {
        LOGW("CommandServer: Input is not a number: %s", cmd.inputName.c_str());

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Input is not a number: " + cmd.inputName;
        enqueueMessage(std::move(msg));
        return;
    }

    auto numberInput = reinterpret_cast<rive::SMINumber*>(foundInput);
    float value = numberInput->value();

    LOGI("CommandServer: Number input value: %f", value);

    Message msg(MessageType::NumberInputValue, cmd.requestID);
    msg.floatValue = value;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleSetNumberInput(const Command& cmd)
{
    LOGI("CommandServer: Handling SetNumberInput command (requestID=%lld, smHandle=%lld, name=%s, value=%f)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.inputName.c_str(), cmd.floatValue);

    auto it = m_stateMachines.find(cmd.handle);
    if (it == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Invalid state machine handle";
        enqueueMessage(std::move(msg));
        return;
    }

    // Find the input by name
    auto& sm = it->second;
    rive::SMIInput* foundInput = nullptr;
    for (size_t i = 0; i < sm->inputCount(); i++) {
        auto input = sm->input(i);
        if (input && input->name() == cmd.inputName) {
            foundInput = input;
            break;
        }
    }

    if (!foundInput) {
        LOGW("CommandServer: Input not found: %s", cmd.inputName.c_str());

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Input not found: " + cmd.inputName;
        enqueueMessage(std::move(msg));
        return;
    }

    if (!foundInput->input()->is<rive::StateMachineNumber>()) {
        LOGW("CommandServer: Input is not a number: %s", cmd.inputName.c_str());

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Input is not a number: " + cmd.inputName;
        enqueueMessage(std::move(msg));
        return;
    }

    auto numberInput = reinterpret_cast<rive::SMINumber*>(foundInput);
    numberInput->value(cmd.floatValue);

    LOGI("CommandServer: Number input set to: %f", cmd.floatValue);

    Message msg(MessageType::InputOperationSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleGetBooleanInput(const Command& cmd)
{
    LOGI("CommandServer: Handling GetBooleanInput command (requestID=%lld, smHandle=%lld, name=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.inputName.c_str());

    auto it = m_stateMachines.find(cmd.handle);
    if (it == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Invalid state machine handle";
        enqueueMessage(std::move(msg));
        return;
    }

    // Find the input by name
    auto& sm = it->second;
    rive::SMIInput* foundInput = nullptr;
    for (size_t i = 0; i < sm->inputCount(); i++) {
        auto input = sm->input(i);
        if (input && input->name() == cmd.inputName) {
            foundInput = input;
            break;
        }
    }

    if (!foundInput) {
        LOGW("CommandServer: Input not found: %s", cmd.inputName.c_str());

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Input not found: " + cmd.inputName;
        enqueueMessage(std::move(msg));
        return;
    }

    if (!foundInput->input()->is<rive::StateMachineBool>()) {
        LOGW("CommandServer: Input is not a boolean: %s", cmd.inputName.c_str());

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Input is not a boolean: " + cmd.inputName;
        enqueueMessage(std::move(msg));
        return;
    }

    auto boolInput = reinterpret_cast<rive::SMIBool*>(foundInput);
    bool value = boolInput->value();

    LOGI("CommandServer: Boolean input value: %d", value);

    Message msg(MessageType::BooleanInputValue, cmd.requestID);
    msg.boolValue = value;
    enqueueMessage(std::move(msg));
}

void CommandServer::handleSetBooleanInput(const Command& cmd)
{
    LOGI("CommandServer: Handling SetBooleanInput command (requestID=%lld, smHandle=%lld, name=%s, value=%d)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.inputName.c_str(), cmd.boolValue);

    auto it = m_stateMachines.find(cmd.handle);
    if (it == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Invalid state machine handle";
        enqueueMessage(std::move(msg));
        return;
    }

    // Find the input by name
    auto& sm = it->second;
    rive::SMIInput* foundInput = nullptr;
    for (size_t i = 0; i < sm->inputCount(); i++) {
        auto input = sm->input(i);
        if (input && input->name() == cmd.inputName) {
            foundInput = input;
            break;
        }
    }

    if (!foundInput) {
        LOGW("CommandServer: Input not found: %s", cmd.inputName.c_str());

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Input not found: " + cmd.inputName;
        enqueueMessage(std::move(msg));
        return;
    }

    if (!foundInput->input()->is<rive::StateMachineBool>()) {
        LOGW("CommandServer: Input is not a boolean: %s", cmd.inputName.c_str());

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Input is not a boolean: " + cmd.inputName;
        enqueueMessage(std::move(msg));
        return;
    }

    auto boolInput = reinterpret_cast<rive::SMIBool*>(foundInput);
    boolInput->value(cmd.boolValue);

    LOGI("CommandServer: Boolean input set to: %d", cmd.boolValue);

    Message msg(MessageType::InputOperationSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

void CommandServer::handleFireTrigger(const Command& cmd)
{
    LOGI("CommandServer: Handling FireTrigger command (requestID=%lld, smHandle=%lld, name=%s)",
         static_cast<long long>(cmd.requestID), static_cast<long long>(cmd.handle), cmd.inputName.c_str());

    auto it = m_stateMachines.find(cmd.handle);
    if (it == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Invalid state machine handle";
        enqueueMessage(std::move(msg));
        return;
    }

    // Find the input by name
    auto& sm = it->second;
    rive::SMIInput* foundInput = nullptr;
    for (size_t i = 0; i < sm->inputCount(); i++) {
        auto input = sm->input(i);
        if (input && input->name() == cmd.inputName) {
            foundInput = input;
            break;
        }
    }

    if (!foundInput) {
        LOGW("CommandServer: Input not found: %s", cmd.inputName.c_str());

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Input not found: " + cmd.inputName;
        enqueueMessage(std::move(msg));
        return;
    }

    if (!foundInput->input()->is<rive::StateMachineTrigger>()) {
        LOGW("CommandServer: Input is not a trigger: %s", cmd.inputName.c_str());

        Message msg(MessageType::InputOperationError, cmd.requestID);
        msg.error = "Input is not a trigger: " + cmd.inputName;
        enqueueMessage(std::move(msg));
        return;
    }

    auto triggerInput = reinterpret_cast<rive::SMITrigger*>(foundInput);
    triggerInput->fire();

    LOGI("CommandServer: Trigger fired: %s", cmd.inputName.c_str());

    Message msg(MessageType::InputOperationSuccess, cmd.requestID);
    enqueueMessage(std::move(msg));
}

} // namespace rive_android