package app.rive.mp.compose

import android.graphics.SurfaceTexture
import android.view.TextureView
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.runtime.withFrameNanos
import androidx.compose.ui.Modifier
import androidx.compose.ui.input.pointer.PointerEventType
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.Layout
import androidx.compose.ui.viewinterop.AndroidView
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.compose.LocalLifecycleOwner
import androidx.lifecycle.repeatOnLifecycle
import app.rive.mp.Artboard
import app.rive.mp.ExperimentalRiveComposeAPI
import app.rive.mp.RenderContextGL
import app.rive.mp.RiveFile
import app.rive.mp.RiveLog
import app.rive.mp.RiveSurface
import app.rive.mp.StateMachine
import app.rive.mp.ViewModelInstance
import kotlinx.coroutines.flow.filter
import kotlinx.coroutines.isActive
import kotlin.time.Duration.Companion.nanoseconds
import app.rive.mp.core.Alignment as CoreAlignment
import app.rive.mp.core.Fit as CoreFit

private const val GENERAL_TAG = "Rive/UI"
private const val STATE_MACHINE_TAG = "Rive/UI/SM"
private const val DRAW_TAG = "Rive/UI/Draw"
private const val VM_INSTANCE_TAG = "Rive/UI/VMI"

/**
 * Converts the compose [Fit] sealed class to the core [CoreFit] enum for native layer calls.
 */
private fun Fit.toCoreEnum(): CoreFit = when (this) {
    is Fit.Layout -> CoreFit.LAYOUT
    is Fit.Contain -> CoreFit.CONTAIN
    is Fit.ScaleDown -> CoreFit.SCALE_DOWN
    is Fit.Cover -> CoreFit.COVER
    is Fit.FitWidth -> CoreFit.FIT_WIDTH
    is Fit.FitHeight -> CoreFit.FIT_HEIGHT
    is Fit.Fill -> CoreFit.FILL
    is Fit.None -> CoreFit.NONE
}

/**
 * Converts the Alignment from the compose Fit sealed class to the core [CoreAlignment] enum.
 */
private fun Fit.toCoreAlignment(): CoreAlignment = when (this.alignment) {
    app.rive.mp.core.Alignment.TOP_LEFT -> CoreAlignment.TOP_LEFT
    app.rive.mp.core.Alignment.TOP_CENTER -> CoreAlignment.TOP_CENTER
    app.rive.mp.core.Alignment.TOP_RIGHT -> CoreAlignment.TOP_RIGHT
    app.rive.mp.core.Alignment.CENTER_LEFT -> CoreAlignment.CENTER_LEFT
    app.rive.mp.core.Alignment.CENTER -> CoreAlignment.CENTER
    app.rive.mp.core.Alignment.CENTER_RIGHT -> CoreAlignment.CENTER_RIGHT
    app.rive.mp.core.Alignment.BOTTOM_LEFT -> CoreAlignment.BOTTOM_LEFT
    app.rive.mp.core.Alignment.BOTTOM_CENTER -> CoreAlignment.BOTTOM_CENTER
    app.rive.mp.core.Alignment.BOTTOM_RIGHT -> CoreAlignment.BOTTOM_RIGHT
}

/**
 * Android implementation of the Rive composable using TextureView and PLS Renderer.
 *
 * This implementation:
 * - Creates a TextureView for hardware-accelerated rendering
 * - Manages the EGL surface lifecycle
 * - Runs the animation loop with proper lifecycle awareness
 * - Handles pointer input for interactive animations
 * - Supports ViewModelInstance binding for data-driven animations
 * - Optimizes for battery life with settled state detection
 */
@ExperimentalRiveComposeAPI
@Composable
actual fun Rive(
    file: RiveFile,
    modifier: Modifier,
    playing: Boolean,
    artboard: Artboard?,
    stateMachine: StateMachine?,
    viewModelInstance: ViewModelInstance?,
    fit: Fit,
    backgroundColor: Int,
    pointerInputMode: RivePointerInputMode
) {
    RiveLog.v(GENERAL_TAG) { "Rive Recomposing" }
    val lifecycleOwner = LocalLifecycleOwner.current
    val riveWorker = file.riveWorker

    // Use provided artboard or create a default one
    val artboardToUse = artboard ?: rememberArtboard(file)
    val artboardHandle = artboardToUse.artboardHandle

    // Use provided state machine or create a default one
    val stateMachineToUse = stateMachine ?: rememberStateMachine(artboardToUse)
    val stateMachineHandle = stateMachineToUse.stateMachineHandle
    var isSettled by remember(stateMachineHandle) { mutableStateOf(false) }

    var surface by remember { mutableStateOf<RiveSurface?>(null) }
    var surfaceWidth by remember { mutableIntStateOf(0) }
    var surfaceHeight by remember { mutableIntStateOf(0) }

    // Clean up for the surface
    DisposableEffect(surface) {
        val nonNullSurface = surface ?: return@DisposableEffect onDispose {}
        onDispose {
            riveWorker.destroyRiveSurface(nonNullSurface)
        }
    }

    // Bind the view model instance to the state machine
    LaunchedEffect(stateMachineHandle, viewModelInstance) {
        if (viewModelInstance == null) {
            RiveLog.d(VM_INSTANCE_TAG) { "No view model instance to bind for $stateMachineHandle" }
            return@LaunchedEffect
        }

        RiveLog.d(VM_INSTANCE_TAG) { "Binding view model instance ${viewModelInstance.instanceHandle}" }
        riveWorker.bindViewModelInstance(
            stateMachineHandle,
            viewModelInstance.instanceHandle
        )

        // Assigning a view model instance unsettles the state machine
        isSettled = false

        // Subscribe to the instance's dirty flow to unsettle when properties change
        viewModelInstance.dirtyFlow.collect {
            RiveLog.v(VM_INSTANCE_TAG) { "View model instance dirty, unsettling $stateMachineHandle" }
            isSettled = false
        }
    }

    // Listen for settle events for this state machine
    LaunchedEffect(stateMachineHandle) {
        riveWorker.settledFlow
            .filter { it.handle == stateMachineHandle.handle }
            .collect {
                RiveLog.v(STATE_MACHINE_TAG) { "State machine $stateMachineHandle settled" }
                isSettled = true
            }
    }

    // Changing the fit, alignment, layout scale factor, or clear color unsettles the state machine
    LaunchedEffect(fit, backgroundColor) {
        RiveLog.d(STATE_MACHINE_TAG) {
            "State machine $stateMachineHandle unsettled due to parameter change"
        }
        isSettled = false
    }

    // Resize artboard based on fit parameter
    LaunchedEffect(fit, surface, surfaceWidth, surfaceHeight) {
        if (surface == null) return@LaunchedEffect
        when (fit) {
            is Fit.Layout -> {
                RiveLog.d(GENERAL_TAG) { "Resizing artboard to $surfaceWidth x $surfaceHeight" }
                artboardToUse.resizeArtboard(surfaceWidth, surfaceHeight, fit.scaleFactor)
            }
            else -> {
                RiveLog.d(GENERAL_TAG) { "Resetting artboard size" }
                artboardToUse.resetArtboardSize()
            }
        }
    }

    // Drawing loop while RESUMED
    LaunchedEffect(
        lifecycleOwner,
        surface,
        artboardHandle,
        stateMachineHandle,
        viewModelInstance,
        fit,
        backgroundColor,
        playing,
    ) {
        if (surface == null) {
            RiveLog.d(DRAW_TAG) { "Surface is null, skipping drawing" }
            return@LaunchedEffect
        }
        if (!playing) {
            RiveLog.d(DRAW_TAG) {
                "Playing is false. Advancing by 0, drawing once, and skipping advancement loop."
            }

            // Advance the state machine once to exit the "Entry" state and apply initial values,
            // including any pending artboard resizes from the fit mode.
            stateMachineToUse.advance(0.nanoseconds)
            riveWorker.draw(
                artboardHandle,
                stateMachineHandle,
                surface!!,
                fit.toCoreEnum(),
                fit.toCoreAlignment(),
                backgroundColor,
                fit.scaleFactor
            )

            return@LaunchedEffect
        }
        lifecycleOwner.lifecycle.repeatOnLifecycle(Lifecycle.State.RESUMED) {
            RiveLog.d(DRAW_TAG) { "Starting drawing with $artboardHandle and $stateMachineHandle" }
            var lastFrameTime = 0.nanoseconds
            while (isActive) {
                val deltaTime = withFrameNanos { frameTimeNs ->
                    val frameTime = frameTimeNs.nanoseconds
                    (if (lastFrameTime == 0.nanoseconds) 0.nanoseconds else frameTime - lastFrameTime).also {
                        lastFrameTime = frameTime
                    }
                }

                // Skip advance and draw when settled
                if (isSettled) {
                    continue
                }

                stateMachineToUse.advance(deltaTime)
                riveWorker.draw(
                    artboardHandle,
                    stateMachineHandle,
                    surface!!,
                    fit.toCoreEnum(),
                    fit.toCoreAlignment(),
                    backgroundColor,
                    fit.scaleFactor
                )
            }
            RiveLog.d(DRAW_TAG) { "Ending drawing with $artboardHandle and $stateMachineHandle" }
        }
    }

    /**
     * A wrapper for the interior AndroidView, since it handles pointer inputs in a non-standard way
     * by passing through all touch events. This gives us a standard Composable to handle pointer
     * events. Effectively a Box, but without pulling in the dependency on the Layout lib.
     */
    @Composable
    fun SingleChildLayout(
        layoutModifier: Modifier = Modifier,
        content: @Composable () -> Unit
    ) {
        Layout(
            content = content,
            modifier = layoutModifier
        ) { measurables, constraints ->
            val placeable = measurables.single().measure(constraints)
            layout(placeable.width, placeable.height) {
                placeable.place(0, 0)
            }
        }
    }

    // Create pointer input modifier using the modern API
    val pointerModifier = Modifier.pointerInput(
        stateMachineHandle, 
        fit, 
        pointerInputMode,
        surface
    ) {
        awaitPointerEventScope {
            while (true) {
                val event = awaitPointerEvent()
                
                // Pointer events unsettle the state machine.
                isSettled = false

                val currentSurface = surface ?: continue

                event.changes.forEach { change ->
                    val pointerPosition = change.position
                    val x = pointerPosition.x
                    val y = pointerPosition.y
                    val pointerId = change.id.value.toInt()
                    
                    val coreFit = fit.toCoreEnum()
                    val coreAlignment = fit.toCoreAlignment()

                    when (event.type) {
                        PointerEventType.Move -> {
                            riveWorker.pointerMove(
                                stateMachineHandle,
                                currentSurface,
                                x,
                                y,
                                coreFit,
                                coreAlignment,
                                fit.scaleFactor,
                                pointerId
                            )
                        }
                        PointerEventType.Press -> {
                            riveWorker.pointerDown(
                                stateMachineHandle,
                                currentSurface,
                                x,
                                y,
                                coreFit,
                                coreAlignment,
                                fit.scaleFactor,
                                pointerId
                            )
                        }
                        PointerEventType.Release -> {
                            // On release, Rive expects both up + exit (logically "exiting" on the Z axis)
                            riveWorker.pointerUp(
                                stateMachineHandle,
                                currentSurface,
                                x,
                                y,
                                coreFit,
                                coreAlignment,
                                fit.scaleFactor,
                                pointerId
                            )
                            riveWorker.pointerExit(
                                stateMachineHandle,
                                currentSurface,
                                coreFit,
                                coreAlignment,
                                fit.scaleFactor,
                                pointerId
                            )
                        }
                        PointerEventType.Exit -> {
                            riveWorker.pointerExit(
                                stateMachineHandle,
                                currentSurface,
                                coreFit,
                                coreAlignment,
                                fit.scaleFactor,
                                pointerId
                            )
                        }
                    }

                    // Only consume in Consume mode. Observe/PassThrough do not consume.
                    if (pointerInputMode == RivePointerInputMode.Consume) {
                        change.consume()
                    }
                }
            }
        }
    }

    SingleChildLayout(layoutModifier = modifier.then(pointerModifier)) {
        AndroidView(
            factory = { context ->
                TextureView(context).apply {
                    isOpaque = false

                    surfaceTextureListener = object : TextureView.SurfaceTextureListener {
                        override fun onSurfaceTextureAvailable(
                            newSurfaceTexture: SurfaceTexture,
                            width: Int,
                            height: Int
                        ) {
                            RiveLog.d(GENERAL_TAG) { "Surface texture available ($width x $height)" }
                            
                            // Get the RenderContext and cast to RenderContextGL for surface creation
                            val renderContext = file.riveWorker.let { cq ->
                                // Access the RenderContext from the CommandQueue
                                // We need to create a surface using RenderContextGL
                                val field = cq.javaClass.getDeclaredField("renderContext")
                                field.isAccessible = true
                                field.get(cq) as RenderContextGL
                            }
                            
                            val drawKey = riveWorker.createDrawKey()
                            val eglSurface = renderContext.createSurface(
                                newSurfaceTexture,
                                drawKey,
                                riveWorker
                            )
                            surface = eglSurface
                            surfaceWidth = width
                            surfaceHeight = height
                        }

                        override fun onSurfaceTextureDestroyed(destroyedSurfaceTexture: SurfaceTexture): Boolean {
                            RiveLog.d(GENERAL_TAG) { "Surface texture destroyed (final release deferred to RenderContext disposal)" }
                            surface = null
                            // False here means that we are responsible for destroying the surface texture
                            // This happens in RenderContext::close(), called from RiveWorker::destroyRiveSurface
                            return false
                        }

                        override fun onSurfaceTextureSizeChanged(
                            surfaceTexture: SurfaceTexture,
                            width: Int,
                            height: Int
                        ) {
                            RiveLog.d(GENERAL_TAG) { "Surface texture size changed ($width x $height)" }
                            surfaceWidth = width
                            surfaceHeight = height
                        }

                        override fun onSurfaceTextureUpdated(surfaceTexture: SurfaceTexture) {
                            // Bitmap callback handling would go here if needed
                        }
                    }
                }
            })
    }
}