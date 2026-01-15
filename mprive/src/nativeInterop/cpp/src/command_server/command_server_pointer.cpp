#include "command_server.hpp"
#include "rive_log.hpp"
#include "rive/math/aabb.hpp"
#include "rive/math/mat2d.hpp"

namespace rive_android {

// =============================================================================
// Phase E.3: Pointer Events - Helper Functions (File-local)
// =============================================================================

/**
 * Convert Fit ordinal to rive::Fit enum.
 */
static rive::Fit getFitFromOrdinalPointer(int32_t ordinal)
{
    switch (ordinal)
    {
        case 0: return rive::Fit::fill;
        case 1: return rive::Fit::contain;
        case 2: return rive::Fit::cover;
        case 3: return rive::Fit::fitWidth;
        case 4: return rive::Fit::fitHeight;
        case 5: return rive::Fit::none;
        case 6: return rive::Fit::scaleDown;
        case 7: return rive::Fit::layout;
        default:
            return rive::Fit::contain;
    }
}

/**
 * Convert Alignment ordinal to rive::Alignment enum.
 */
static rive::Alignment getAlignmentFromOrdinalPointer(int32_t ordinal)
{
    switch (ordinal)
    {
        case 0: return rive::Alignment::topLeft;
        case 1: return rive::Alignment::topCenter;
        case 2: return rive::Alignment::topRight;
        case 3: return rive::Alignment::centerLeft;
        case 4: return rive::Alignment::center;
        case 5: return rive::Alignment::centerRight;
        case 6: return rive::Alignment::bottomLeft;
        case 7: return rive::Alignment::bottomCenter;
        case 8: return rive::Alignment::bottomRight;
        default:
            return rive::Alignment::center;
    }
}

// =============================================================================
// Phase E.3: Pointer Events - Public API
// =============================================================================

void CommandServer::pointerMove(int64_t smHandle, int8_t fit, int8_t alignment,
                                 float layoutScale, float surfaceWidth, float surfaceHeight,
                                 int32_t pointerID, float x, float y)
{
    LOGI("CommandServer: Enqueuing PointerMove (smHandle=%lld, x=%f, y=%f)",
         static_cast<long long>(smHandle), x, y);

    Command cmd(CommandType::PointerMove, 0);  // No requestID needed for fire-and-forget
    cmd.handle = smHandle;
    cmd.pointerFit = fit;
    cmd.pointerAlignment = alignment;
    cmd.layoutScale = layoutScale;
    cmd.pointerSurfaceWidth = surfaceWidth;
    cmd.pointerSurfaceHeight = surfaceHeight;
    cmd.pointerID = pointerID;
    cmd.pointerX = x;
    cmd.pointerY = y;

    enqueueCommand(std::move(cmd));
}

void CommandServer::pointerDown(int64_t smHandle, int8_t fit, int8_t alignment,
                                 float layoutScale, float surfaceWidth, float surfaceHeight,
                                 int32_t pointerID, float x, float y)
{
    LOGI("CommandServer: Enqueuing PointerDown (smHandle=%lld, x=%f, y=%f)",
         static_cast<long long>(smHandle), x, y);

    Command cmd(CommandType::PointerDown, 0);
    cmd.handle = smHandle;
    cmd.pointerFit = fit;
    cmd.pointerAlignment = alignment;
    cmd.layoutScale = layoutScale;
    cmd.pointerSurfaceWidth = surfaceWidth;
    cmd.pointerSurfaceHeight = surfaceHeight;
    cmd.pointerID = pointerID;
    cmd.pointerX = x;
    cmd.pointerY = y;

    enqueueCommand(std::move(cmd));
}

void CommandServer::pointerUp(int64_t smHandle, int8_t fit, int8_t alignment,
                               float layoutScale, float surfaceWidth, float surfaceHeight,
                               int32_t pointerID, float x, float y)
{
    LOGI("CommandServer: Enqueuing PointerUp (smHandle=%lld, x=%f, y=%f)",
         static_cast<long long>(smHandle), x, y);

    Command cmd(CommandType::PointerUp, 0);
    cmd.handle = smHandle;
    cmd.pointerFit = fit;
    cmd.pointerAlignment = alignment;
    cmd.layoutScale = layoutScale;
    cmd.pointerSurfaceWidth = surfaceWidth;
    cmd.pointerSurfaceHeight = surfaceHeight;
    cmd.pointerID = pointerID;
    cmd.pointerX = x;
    cmd.pointerY = y;

    enqueueCommand(std::move(cmd));
}

void CommandServer::pointerExit(int64_t smHandle, int8_t fit, int8_t alignment,
                                 float layoutScale, float surfaceWidth, float surfaceHeight,
                                 int32_t pointerID, float x, float y)
{
    LOGI("CommandServer: Enqueuing PointerExit (smHandle=%lld)",
         static_cast<long long>(smHandle));

    Command cmd(CommandType::PointerExit, 0);
    cmd.handle = smHandle;
    cmd.pointerFit = fit;
    cmd.pointerAlignment = alignment;
    cmd.layoutScale = layoutScale;
    cmd.pointerSurfaceWidth = surfaceWidth;
    cmd.pointerSurfaceHeight = surfaceHeight;
    cmd.pointerID = pointerID;
    cmd.pointerX = x;
    cmd.pointerY = y;

    enqueueCommand(std::move(cmd));
}

// =============================================================================
// Phase E.3: Pointer Events - Coordinate Transformation Helper
// =============================================================================

bool CommandServer::transformToArtboardCoords(int64_t smHandle, int8_t fit, int8_t alignment,
                                               float layoutScale, float surfaceWidth, float surfaceHeight,
                                               float surfaceX, float surfaceY,
                                               float& outX, float& outY)
{
    // Get the state machine to access its artboard bounds
    auto smIt = m_stateMachines.find(smHandle);
    if (smIt == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle for coordinate transform: %lld",
             static_cast<long long>(smHandle));
        return false;
    }

    // Get artboard bounds from the state machine's artboard
    auto* artboard = smIt->second->artboard();
    if (!artboard) {
        LOGW("CommandServer: State machine has no artboard for coordinate transform");
        return false;
    }

    rive::AABB artboardBounds = artboard->bounds();
    rive::AABB surfaceBounds(0, 0, surfaceWidth, surfaceHeight);

    // Get the fit and alignment
    rive::Fit riveFit = getFitFromOrdinalPointer(static_cast<int32_t>(fit));
    rive::Alignment riveAlignment = getAlignmentFromOrdinalPointer(static_cast<int32_t>(alignment));

    // Compute the transformation matrix using rive::computeAlignment
    rive::Mat2D viewTransform = rive::computeAlignment(
        riveFit,
        riveAlignment,
        surfaceBounds,
        artboardBounds,
        layoutScale
    );

    // Invert the transform to go from surface space to artboard space
    rive::Mat2D inverseTransform;
    if (!viewTransform.invert(&inverseTransform)) {
        LOGW("CommandServer: Failed to invert view transform for coordinate transform");
        // If inversion fails, return the original coordinates
        outX = surfaceX;
        outY = surfaceY;
        return true;
    }

    // Transform the point
    rive::Vec2D surfacePoint(surfaceX, surfaceY);
    rive::Vec2D artboardPoint = inverseTransform * surfacePoint;

    outX = artboardPoint.x;
    outY = artboardPoint.y;

    return true;
}

// =============================================================================
// Phase E.3: Pointer Events - Handlers
// =============================================================================

void CommandServer::handlePointerMove(const Command& cmd)
{
    LOGI("CommandServer: Handling PointerMove (smHandle=%lld, x=%f, y=%f)",
         static_cast<long long>(cmd.handle), cmd.pointerX, cmd.pointerY);

    auto smIt = m_stateMachines.find(cmd.handle);
    if (smIt == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));
        return;  // Fire-and-forget, no error message
    }

    // Transform coordinates from surface space to artboard space
    float artboardX, artboardY;
    if (!transformToArtboardCoords(cmd.handle, cmd.pointerFit, cmd.pointerAlignment,
                                    cmd.layoutScale, cmd.pointerSurfaceWidth, cmd.pointerSurfaceHeight,
                                    cmd.pointerX, cmd.pointerY,
                                    artboardX, artboardY)) {
        return;
    }

    // Send pointer move to state machine
    smIt->second->pointerMove(rive::Vec2D(artboardX, artboardY));

    LOGI("CommandServer: PointerMove forwarded (artboard coords: %f, %f)", artboardX, artboardY);
}

void CommandServer::handlePointerDown(const Command& cmd)
{
    LOGI("CommandServer: Handling PointerDown (smHandle=%lld, x=%f, y=%f)",
         static_cast<long long>(cmd.handle), cmd.pointerX, cmd.pointerY);

    auto smIt = m_stateMachines.find(cmd.handle);
    if (smIt == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));
        return;
    }

    // Transform coordinates
    float artboardX, artboardY;
    if (!transformToArtboardCoords(cmd.handle, cmd.pointerFit, cmd.pointerAlignment,
                                    cmd.layoutScale, cmd.pointerSurfaceWidth, cmd.pointerSurfaceHeight,
                                    cmd.pointerX, cmd.pointerY,
                                    artboardX, artboardY)) {
        return;
    }

    // Send pointer down to state machine
    smIt->second->pointerDown(rive::Vec2D(artboardX, artboardY));

    LOGI("CommandServer: PointerDown forwarded (artboard coords: %f, %f)", artboardX, artboardY);
}

void CommandServer::handlePointerUp(const Command& cmd)
{
    LOGI("CommandServer: Handling PointerUp (smHandle=%lld, x=%f, y=%f)",
         static_cast<long long>(cmd.handle), cmd.pointerX, cmd.pointerY);

    auto smIt = m_stateMachines.find(cmd.handle);
    if (smIt == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));
        return;
    }

    // Transform coordinates
    float artboardX, artboardY;
    if (!transformToArtboardCoords(cmd.handle, cmd.pointerFit, cmd.pointerAlignment,
                                    cmd.layoutScale, cmd.pointerSurfaceWidth, cmd.pointerSurfaceHeight,
                                    cmd.pointerX, cmd.pointerY,
                                    artboardX, artboardY)) {
        return;
    }

    // Send pointer up to state machine
    smIt->second->pointerUp(rive::Vec2D(artboardX, artboardY));

    LOGI("CommandServer: PointerUp forwarded (artboard coords: %f, %f)", artboardX, artboardY);
}

void CommandServer::handlePointerExit(const Command& cmd)
{
    LOGI("CommandServer: Handling PointerExit (smHandle=%lld)",
         static_cast<long long>(cmd.handle));

    auto smIt = m_stateMachines.find(cmd.handle);
    if (smIt == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", static_cast<long long>(cmd.handle));
        return;
    }

    // Transform coordinates from surface space to artboard space
    // pointerExit in Rive API requires a position parameter
    float artboardX, artboardY;
    if (!transformToArtboardCoords(cmd.handle, cmd.pointerFit, cmd.pointerAlignment,
                                    cmd.layoutScale, cmd.pointerSurfaceWidth, cmd.pointerSurfaceHeight,
                                    cmd.pointerX, cmd.pointerY,
                                    artboardX, artboardY)) {
        return;
    }

    // Send pointer exit to state machine with position
    smIt->second->pointerExit(rive::Vec2D(artboardX, artboardY));

    LOGI("CommandServer: PointerExit forwarded (artboard coords: %f, %f)", artboardX, artboardY);
}

} // namespace rive_android