/**
 * CommandServer Animation Operations
 * 
 * Implementation of linear animation management for mprive.
 * Linear animations are timeline-based animations that play independently of state machines.
 * 
 * This provides rive-android parity where BOTH linear animations AND state machines
 * can be played simultaneously.
 */

#include "command_server.hpp"
#include "rive_log.hpp"

namespace rive_android {

int64_t CommandServer::createDefaultAnimationSync(int64_t artboardHandle)
{
    std::lock_guard<std::mutex> lock(m_resourceMutex);
    
    auto it = m_artboards.find(artboardHandle);
    if (it == m_artboards.end()) {
        LOGW("CommandServer: createDefaultAnimation - Invalid artboard handle: %lld", (long long)artboardHandle);
        return 0;
    }
    
    auto* artboard = it->second->artboard();
    if (!artboard) {
        LOGW("CommandServer: createDefaultAnimation - Artboard has no instance");
        return 0;
    }
    
    if (artboard->animationCount() == 0) {
        LOGW("CommandServer: createDefaultAnimation - Artboard has no animations");
        return 0;
    }
    
    // Create instance from first animation
    auto animation = artboard->animationAt(0);
    if (!animation) {
        LOGW("CommandServer: createDefaultAnimation - Failed to create animation at index 0");
        return 0;
    }
    
    int64_t handle = m_nextHandle.fetch_add(1);
    m_animations[handle] = std::move(animation);
    
    LOGI("CommandServer: Animation created (handle=%lld, name=%s)", 
         (long long)handle, 
         m_animations[handle]->name().c_str());
    return handle;
}

int64_t CommandServer::createAnimationByNameSync(int64_t artboardHandle, const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_resourceMutex);
    
    auto it = m_artboards.find(artboardHandle);
    if (it == m_artboards.end()) {
        LOGW("CommandServer: createAnimationByName - Invalid artboard handle: %lld", (long long)artboardHandle);
        return 0;
    }
    
    auto* artboard = it->second->artboard();
    if (!artboard) {
        LOGW("CommandServer: createAnimationByName - Artboard has no instance");
        return 0;
    }
    
    auto animation = artboard->animationNamed(name);
    if (!animation) {
        LOGW("CommandServer: createAnimationByName - Animation not found: %s", name.c_str());
        return 0;
    }
    
    int64_t handle = m_nextHandle.fetch_add(1);
    m_animations[handle] = std::move(animation);
    
    LOGI("CommandServer: Animation '%s' created (handle=%lld)", name.c_str(), (long long)handle);
    return handle;
}

bool CommandServer::advanceAndApplyAnimation(int64_t animHandle, int64_t artboardHandle, float deltaTime, bool advanceArtboard)
{
    std::lock_guard<std::mutex> lock(m_resourceMutex);
    
    auto animIt = m_animations.find(animHandle);
    if (animIt == m_animations.end()) {
        LOGW("CommandServer: advanceAndApplyAnimation - Invalid animation handle: %lld", (long long)animHandle);
        return false;
    }
    
    auto abIt = m_artboards.find(artboardHandle);
    if (abIt == m_artboards.end()) {
        LOGW("CommandServer: advanceAndApplyAnimation - Invalid artboard handle: %lld", (long long)artboardHandle);
        return false;
    }
    
    auto& anim = animIt->second;
    auto* artboard = abIt->second->artboard();
    
    // Advance the animation (ignore the return value indicating if the animation looped)
    anim->advance(deltaTime);
    
    // Apply to artboard (the animation instance already knows its artboard)
    // The apply() method takes only a mix value (default 1.0f)
    anim->apply();
    
    // Advance the artboard if requested.
    // This matches the reference rive-android implementation where:
    // - If a state machine is active, IT handles artboard advancement via advanceAndApply()
    //   so advanceArtboard should be false
    // - If only linear animations are active (no state machine), advanceArtboard should be true
    //   to process artboard-level updates (audio, events, nested artboards, etc.)
    if (advanceArtboard) {
        artboard->advance(deltaTime);
    }
    
    // Return whether animation is still playing (for oneShot detection)
    // loopValue: 0=oneShot, 1=loop, 2=pingPong
    bool didLoop = anim->didLoop();
    int loopMode = anim->loopValue();
    
    // For oneShot (loopValue=0), stop when the animation reaches the end
    if (loopMode == 0 && didLoop) {
        return false; // Animation completed
    }
    
    return true; // Animation still playing
}

void CommandServer::deleteAnimation(int64_t animHandle)
{
    std::lock_guard<std::mutex> lock(m_resourceMutex);
    
    auto it = m_animations.find(animHandle);
    if (it != m_animations.end()) {
        LOGI("CommandServer: Animation deleted (handle=%lld)", (long long)animHandle);
        m_animations.erase(it);
    }
}

void CommandServer::setAnimationTime(int64_t animHandle, float time)
{
    std::lock_guard<std::mutex> lock(m_resourceMutex);
    
    auto it = m_animations.find(animHandle);
    if (it != m_animations.end()) {
        it->second->time(time);
    }
}

void CommandServer::setAnimationLoop(int64_t animHandle, int32_t loopMode)
{
    std::lock_guard<std::mutex> lock(m_resourceMutex);
    
    auto it = m_animations.find(animHandle);
    if (it != m_animations.end()) {
        it->second->loopValue(loopMode);
    }
}

void CommandServer::setAnimationDirection(int64_t animHandle, int32_t direction)
{
    std::lock_guard<std::mutex> lock(m_resourceMutex);
    
    auto it = m_animations.find(animHandle);
    if (it != m_animations.end()) {
        it->second->direction(direction);
    }
}

} // namespace rive_android