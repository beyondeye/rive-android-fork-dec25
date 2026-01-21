package app.rive.mp.compose

import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.toArgb
import app.rive.mp.Artboard
import app.rive.mp.RiveFile
import app.rive.mp.StateMachine
import app.rive.mp.ViewModelInstance

/**
 * The main composable for rendering a Rive file's artboard and state machine.
 *
 * This composable manages the complete Rive rendering pipeline including:
 * - Surface creation and lifecycle management
 * - Animation loop (advance state machine + draw on each frame)
 * - Pointer input event handling
 * - ViewModelInstance binding to state machines
 * - Settled/unsettled state optimization for battery life
 *
 * The composable will advance the state machine and draw the artboard on every frame while the
 * Lifecycle is in the RESUMED state. It will also handle pointer input events to influence the
 * state machine, such as pointer down, move, and up events.
 *
 * A Rive composable can enter a settled state, where it stops advancing the state machine. It
 * will be restarted when influenced by other events, such as pointer input or view model instance
 * changes.
 *
 * ## Platform-specific implementations:
 * - **Android**: Uses TextureView + PLS Renderer with full hardware acceleration
 * - **Desktop**: Stub implementation (full support coming in Phase 4)
 *
 * @param file The [RiveFile] that contains the artboard and state machine definitions.
 * @param modifier The [Modifier] to apply to the composable.
 * @param playing Whether the state machine should advance. When true (default), the state machine
 *    will advance on each frame. When false, the advancement loop will not activate.
 * @param artboard The [Artboard] to render. If null, the default artboard will be used.
 * @param stateMachine The [StateMachine] to use. If null, the default state machine will be
 *    created from the artboard.
 * @param viewModelInstance The [ViewModelInstance] to bind to the state machine. If null, no view
 *    model instance will be bound.
 * @param fit The [Fit] to use for the artboard. Defaults to [Fit.Contain].
 * @param backgroundColor The color to clear the surface with before drawing. Defaults to
 *    transparent.
 * @param pointerInputMode Controls how pointer events are handled and consumed by Rive. See
 *    [RivePointerInputMode]. Default is [RivePointerInputMode.Consume].
 *
 * @see RiveFile For loading Rive files
 * @see Artboard For artboard management
 * @see StateMachine For state machine control
 * @see ViewModelInstance For data binding
 * @see Fit For fit mode options
 * @see RivePointerInputMode For pointer event handling modes
 */
@Composable
expect fun Rive(
    file: RiveFile,
    modifier: Modifier = Modifier,
    playing: Boolean = true,
    artboard: Artboard? = null,
    stateMachine: StateMachine? = null,
    viewModelInstance: ViewModelInstance? = null,
    fit: Fit = Fit.Contain(),
    backgroundColor: Int = Color.Transparent.toArgb(),
    pointerInputMode: RivePointerInputMode = RivePointerInputMode.Consume
)