I am trying to run RiveDemo.kt from the mpapp module and I get the follwoing crash (as logged by logcat)
can you help me solve the issue?

2026-01-18 22:40:50.588 16558-16558 VRI[MainAc...ty]@28d4be app.rive.mpapp                       I  call setFrameRateCategory for touch hint category=high hint, reason=touch, vri=VRI[MainActivity]@28d4be
2026-01-18 22:40:50.601 16558-16558 View                    app.rive.mpapp                       I  setRequestedFrameRate frameRate=NaN, this=androidx.compose.ui.platform.AndroidComposeView{d818a1e VFED..... ........ 0,0-1080,2340 aid=1073741824}, caller=android.view.ViewGroup.setRequestedFrameRate:9952 androidx.compose.ui.platform.Api35Impl.setRequestedFrameRate:3822 androidx.compose.ui.platform.AndroidComposeView.dispatchDraw:2114 android.view.View.draw:26345 android.view.View.updateDisplayListIfDirty:25175
2026-01-18 22:40:50.601 16558-16558 View                    app.rive.mpapp                       I  setRequestedFrameRate frameRate=NaN, this=android.view.View{f05f51e V.ED..... ........ 0,0-0,0}, caller=androidx.compose.ui.platform.Api35Impl.setRequestedFrameRate:3822 androidx.compose.ui.platform.AndroidComposeView.dispatchDraw:2115 android.view.View.draw:26345 android.view.View.updateDisplayListIfDirty:25175 android.view.ViewGroup.recreateChildDisplayList:4794
2026-01-18 22:40:50.703 16558-16558 VRI[MainAc...ty]@28d4be app.rive.mpapp                       I  ViewPostIme pointer 1
2026-01-18 22:40:50.708 16558-16659 ProfileInstaller        app.rive.mpapp                       D  Installing profile for app.rive.mpapp
2026-01-18 22:40:50.737 16558-16558 AdrenoGLES-0            app.rive.mpapp                       I  QUALCOMM build                   : c81b28e1a5, I5a61fcf667
Build Date                       : 06/20/25
OpenGL ES Shader Compiler Version: E031.45.02.26
Local Branch                     :
Remote Branch                    : refs/tags/AU_LINUX_ANDROID_LA.VENDOR.14.3.0.11.00.00.973.948
Remote Branch                    : NONE
Reconstruct Branch               : NOTHING
2026-01-18 22:40:50.737 16558-16558 AdrenoGLES-0            app.rive.mpapp                       I  Build Config                     : S P 16.1.2 AArch64
2026-01-18 22:40:50.737 16558-16558 AdrenoGLES-0            app.rive.mpapp                       I  Driver Path                      : /vendor/lib64/egl/libGLESv2_adreno.so
2026-01-18 22:40:50.737 16558-16558 AdrenoGLES-0            app.rive.mpapp                       I  Driver Version                   : 0762.39
2026-01-18 22:40:50.737 16558-16558 AdrenoGLES-0            app.rive.mpapp                       I  Process Name                     : app.rive.mpapp
2026-01-18 22:40:50.738 16558-16558 AdrenoGLES-0            app.rive.mpapp                       I  PFP: 0x01520181, ME: 0x01520023
2026-01-18 22:40:50.740 16558-16558 Adreno-AppProfiles      app.rive.mpapp                       E  QSPM AIDL service doesn't exist
2026-01-18 22:40:50.742 16558-16558 rive-mp                 app.rive.mpapp                       D  [RenderContextJNI] Creating RenderContextGL from EGL handles
2026-01-18 22:40:50.742 16558-16558 rive-mp                 app.rive.mpapp                       D  [RenderContext] Creating 1x1 PBuffer surface for EGL context
2026-01-18 22:40:50.742 16558-16558 rive-mp                 app.rive.mpapp                       D  [RenderContext] Successfully created PBuffer surface
2026-01-18 22:40:50.742 16558-16558 rive-mp                 app.rive.mpapp                       D  [RenderContextJNI] RenderContextGL created successfully
2026-01-18 22:40:50.742 16558-16558 rive-mp                 app.rive.mpapp                       I  CommandQueue JNI: Creating CommandServer
2026-01-18 22:40:50.742 16558-16558 rive-mp                 app.rive.mpapp                       I  CommandServer: Constructing
2026-01-18 22:40:50.742 16558-16558 rive-mp                 app.rive.mpapp                       I  CommandServer: Starting worker thread
2026-01-18 22:40:50.742 16558-16558 rive-mp                 app.rive.mpapp                       I  CommandQueue JNI: CommandServer created successfully
2026-01-18 22:40:50.742 16558-16660 rive-mp                 app.rive.mpapp                       I  CommandServer: Worker thread started
2026-01-18 22:40:50.781 16558-16558 View                    app.rive.mpapp                       I  setRequestedFrameRate frameRate=NaN, this=androidx.compose.ui.platform.AndroidComposeView{d818a1e VFED..... ........ 0,0-1080,2340 aid=1073741824}, caller=android.view.ViewGroup.setRequestedFrameRate:9952 androidx.compose.ui.platform.Api35Impl.setRequestedFrameRate:3822 androidx.compose.ui.platform.AndroidComposeView.dispatchDraw:2114 android.view.View.draw:26345 android.view.View.updateDisplayListIfDirty:25175
2026-01-18 22:40:50.781 16558-16558 View                    app.rive.mpapp                       I  setRequestedFrameRate frameRate=-4.0, this=android.view.View{f05f51e V.ED..... ........ 0,0-0,0}, caller=androidx.compose.ui.platform.Api35Impl.setRequestedFrameRate:3822 androidx.compose.ui.platform.AndroidComposeView.dispatchDraw:2115 android.view.View.draw:26345 android.view.View.updateDisplayListIfDirty:25175 android.view.View.draw:26072
2026-01-18 22:40:50.783 16558-16558 rive-mp                 app.rive.mpapp                       I  CommandServer: Enqueuing LoadFile command (requestID=0, size=15821)
2026-01-18 22:40:50.783 16558-16660 rive-mp                 app.rive.mpapp                       I  CommandServer: Handling LoadFile command (requestID=0, size=15821)
2026-01-18 22:40:50.791 16558-16558 View                    app.rive.mpapp                       I  setRequestedFrameRate frameRate=NaN, this=androidx.compose.ui.platform.AndroidComposeView{d818a1e VFED..... ........ 0,0-1080,2340 aid=1073741824}, caller=android.view.ViewGroup.setRequestedFrameRate:9952 androidx.compose.ui.platform.Api35Impl.setRequestedFrameRate:3822 androidx.compose.ui.platform.AndroidComposeView.dispatchDraw:2114 android.view.View.draw:26345 android.view.View.updateDisplayListIfDirty:25175
2026-01-18 22:40:50.791 16558-16558 View                    app.rive.mpapp                       I  setRequestedFrameRate frameRate=NaN, this=android.view.View{f05f51e V.ED..... ........ 0,0-0,0}, caller=androidx.compose.ui.platform.Api35Impl.setRequestedFrameRate:3822 androidx.compose.ui.platform.AndroidComposeView.dispatchDraw:2115 android.view.View.draw:26345 android.view.View.updateDisplayListIfDirty:25175 android.view.ViewGroup.recreateChildDisplayList:4794
2026-01-18 22:40:50.795 16558-16660 rive-mp                 app.rive.mpapp                       I  CommandServer: File loaded successfully (handle=1)
2026-01-18 22:40:50.808 16558-16558 View                    app.rive.mpapp                       I  setRequestedFrameRate frameRate=NaN, this=androidx.compose.ui.platform.AndroidComposeView{d818a1e VFED..... ........ 0,0-1080,2340 aid=1073741824}, caller=android.view.ViewGroup.setRequestedFrameRate:9952 androidx.compose.ui.platform.Api35Impl.setRequestedFrameRate:3822 androidx.compose.ui.platform.AndroidComposeView.dispatchDraw:2114 android.view.View.draw:26345 android.view.View.updateDisplayListIfDirty:25175
2026-01-18 22:40:50.808 16558-16558 View                    app.rive.mpapp                       I  setRequestedFrameRate frameRate=NaN, this=android.view.View{f05f51e V.ED..... ........ 0,0-0,0}, caller=androidx.compose.ui.platform.Api35Impl.setRequestedFrameRate:3822 androidx.compose.ui.platform.AndroidComposeView.dispatchDraw:2115 android.view.View.draw:26345 android.view.View.updateDisplayListIfDirty:25175 android.view.ViewGroup.recreateChildDisplayList:4794
2026-01-18 22:40:50.811 16558-16558 rive-mp                 app.rive.mpapp                       I  CommandServer: Creating default artboard synchronously (fileHandle=1)
2026-01-18 22:40:50.811 16558-16558 rive-mp                 app.rive.mpapp                       I  CommandServer: Artboard created synchronously (handle=2)
2026-01-18 22:40:50.812 16558-16558 rive-mp                 app.rive.mpapp                       I  CommandServer: Enqueuing CreateDefaultStateMachine command (requestID=0, artboardHandle=2)
2026-01-18 22:40:50.812 16558-16660 rive-mp                 app.rive.mpapp                       I  CommandServer: Handling CreateDefaultStateMachine command (requestID=0, artboardHandle=2)
2026-01-18 22:40:50.812 16558-16660 rive-mp                 app.rive.mpapp                       I  CommandServer: State machine created successfully (handle=3)
2026-01-18 22:40:50.825 16558-16558 libc                    app.rive.mpapp                       W  Access denied finding property "vendor.display.enable_optimal_refresh_rate"
2026-01-18 22:40:50.825 16558-16558 libc                    app.rive.mpapp                       W  Access denied finding property "vendor.display.enable_optimal_refresh_rate"
2026-01-18 22:40:50.825 16558-16558 libc                    app.rive.mpapp                       W  Access denied finding property "vendor.display.enable_optimal_refresh_rate"
2026-01-18 22:40:50.826 16558-16558 app.rive.mpapp          app.rive.mpapp                       E  No implementation found for long app.rive.mp.core.CommandQueueJNIBridge.cppCreateRiveRenderTarget(long, int, int) (tried Java_app_rive_mp_core_CommandQueueJNIBridge_cppCreateRiveRenderTarget and Java_app_rive_mp_core_CommandQueueJNIBridge_cppCreateRiveRenderTarget__JII) - is the library loaded, e.g. System.loadLibrary?
2026-01-18 22:40:50.826 16558-16558 AndroidRuntime          app.rive.mpapp                       D  Shutting down VM
2026-01-18 22:40:50.828 16558-16558 AndroidRuntime          app.rive.mpapp                       E  FATAL EXCEPTION: main
Process: app.rive.mpapp, PID: 16558
java.lang.UnsatisfiedLinkError: No implementation found for long app.rive.mp.core.CommandQueueJNIBridge.cppCreateRiveRenderTarget(long, int, int) (tried Java_app_rive_mp_core_CommandQueueJNIBridge_cppCreateRiveRenderTarget and Java_app_rive_mp_core_CommandQueueJNIBridge_cppCreateRiveRenderTarget__JII) - is the library loaded, e.g. System.loadLibrary?
at app.rive.mp.core.CommandQueueJNIBridge.cppCreateRiveRenderTarget(Native Method)
at app.rive.mp.CommandQueue.createRiveRenderTarget(CommandQueue.kt:233)
at app.rive.mp.RenderContextGL.createSurface-GRKicnE(RenderContext.android.kt:294)
at app.rive.mp.compose.Rive_androidKt$Rive$8$1$1$1$1.onSurfaceTextureAvailable(Rive.android.kt:378)
at android.view.TextureView.getTextureLayer(TextureView.java:494)
at android.view.TextureView.draw(TextureView.java:431)
at android.view.View.updateDisplayListIfDirty(View.java:25175)
at android.view.View.draw(View.java:26072)
at android.view.ViewGroup.drawChild(ViewGroup.java:4810)
at android.view.ViewGroup.dispatchDraw(ViewGroup.java:4564)
at android.view.View.draw(View.java:26345)
at androidx.compose.ui.platform.AndroidViewsHandler.drawView(AndroidViewsHandler.android.kt:79)
at androidx.compose.ui.platform.AndroidComposeView.drawAndroidView(AndroidComposeView.android.kt:1683)
at androidx.compose.ui.viewinterop.AndroidViewHolder$layoutNode$1$coreModifier$2.invoke(AndroidViewHolder.android.kt:395)
at androidx.compose.ui.viewinterop.AndroidViewHolder$layoutNode$1$coreModifier$2.invoke(AndroidViewHolder.android.kt:391)
at androidx.compose.ui.draw.DrawBackgroundModifier.draw(DrawModifier.kt:126)
at androidx.compose.ui.node.LayoutNodeDrawScope.drawDirect-eZhPAX0$ui(LayoutNodeDrawScope.kt:132)
at androidx.compose.ui.node.LayoutNodeDrawScope.draw-eZhPAX0$ui(LayoutNodeDrawScope.kt:119)
at androidx.compose.ui.node.NodeCoordinator.drawContainedDrawModifiers(NodeCoordinator.kt:484)
at androidx.compose.ui.node.NodeCoordinator.draw(NodeCoordinator.kt:473)
at androidx.compose.ui.node.LayoutNode.draw$ui(LayoutNode.kt:1061)
at androidx.compose.ui.node.InnerNodeCoordinator.performDraw(InnerNodeCoordinator.kt:177)
at androidx.compose.ui.node.NodeCoordinator.drawContainedDrawModifiers(NodeCoordinator.kt:481)
at androidx.compose.ui.node.NodeCoordinator.draw(NodeCoordinator.kt:473)
at androidx.compose.ui.node.LayoutModifierNodeCoordinator.performDraw(LayoutModifierNodeCoordinator.kt:274)
at androidx.compose.ui.node.NodeCoordinator.drawContainedDrawModifiers(NodeCoordinator.kt:481)
at androidx.compose.ui.node.NodeCoordinator.draw(NodeCoordinator.kt:473)
at androidx.compose.ui.node.LayoutNode.draw$ui(LayoutNode.kt:1061)
at androidx.compose.ui.node.InnerNodeCoordinator.performDraw(InnerNodeCoordinator.kt:177)
at androidx.compose.ui.node.LayoutNodeDrawScope.drawContent(LayoutNodeDrawScope.kt:74)
at androidx.compose.foundation.BackgroundNode.draw(Background.kt:167)
at androidx.compose.ui.node.LayoutNodeDrawScope.drawDirect-eZhPAX0$ui(LayoutNodeDrawScope.kt:132)
at androidx.compose.ui.node.LayoutNodeDrawScope.draw-eZhPAX0$ui(LayoutNodeDrawScope.kt:119)
at androidx.compose.ui.node.NodeCoordinator.drawContainedDrawModifiers(NodeCoordinator.kt:484)
at androidx.compose.ui.node.NodeCoordinator.draw(NodeCoordinator.kt:473)
at androidx.compose.ui.node.LayoutModifierNodeCoordinator.performDraw(LayoutModifierNodeCoordinator.kt:274)
at androidx.compose.ui.node.NodeCoordinator.drawContainedDrawModifiers(NodeCoordinator.kt:481)
at androidx.compose.ui.node.NodeCoordinator.draw(NodeCoordinator.kt:473)
at androidx.compose.ui.node.LayoutNode.draw$ui(LayoutNode.kt:1061)
at androidx.compose.ui.node.InnerNodeCoordinator.performDraw(InnerNodeCoordinator.kt:177)
at androidx.compose.ui.node.NodeCoordinator.drawContainedDrawModifiers(NodeCoordinator.kt:481)
at androidx.compose.ui.node.NodeCoordinator.draw(NodeCoordinator.kt:473)
2026-01-18 22:40:50.828 16558-16558 AndroidRuntime          app.rive.mpapp                       E  	at androidx.compose.ui.node.LayoutModifierNodeCoordinator.performDraw(LayoutModifierNodeCoordinator.kt:274)
at androidx.compose.ui.node.NodeCoordinator.drawContainedDrawModifiers(NodeCoordinator.kt:481)
at androidx.compose.ui.node.NodeCoordinator.draw(NodeCoordinator.kt:473)
at androidx.compose.ui.node.LayoutModifierNodeCoordinator.performDraw(LayoutModifierNodeCoordinator.kt:274)
at androidx.compose.ui.node.NodeCoordinator.drawContainedDrawModifiers(NodeCoordinator.kt:481)
at androidx.compose.ui.node.NodeCoordinator.draw(NodeCoordinator.kt:473)
at androidx.compose.ui.node.LayoutNode.draw$ui(LayoutNode.kt:1061)
at androidx.compose.ui.node.InnerNodeCoordinator.performDraw(InnerNodeCoordinator.kt:177)
at androidx.compose.ui.node.NodeCoordinator.drawContainedDrawModifiers(NodeCoordinator.kt:481)
at androidx.compose.ui.node.NodeCoordinator.access$drawContainedDrawModifiers(NodeCoordinator.kt:69)
at androidx.compose.ui.node.NodeCoordinator$drawBlock$drawBlockCallToDrawModifiers$1.invoke(NodeCoordinator.kt:507)
at androidx.compose.ui.node.NodeCoordinator$drawBlock$drawBlockCallToDrawModifiers$1.invoke(NodeCoordinator.kt:506)
at androidx.compose.runtime.snapshots.SnapshotStateObserver.observeReads(SnapshotStateObserver.kt:728)
at androidx.compose.ui.node.NodeCoordinator$drawBlock$1.invoke(NodeCoordinator.kt:1660)
at androidx.compose.ui.node.NodeCoordinator$drawBlock$1.invoke(NodeCoordinator.kt:509)
at androidx.compose.ui.platform.GraphicsLayerOwnerLayer$recordLambda$1.invoke(GraphicsLayerOwnerLayer.android.kt:276)
at androidx.compose.ui.platform.GraphicsLayerOwnerLayer$recordLambda$1.invoke(GraphicsLayerOwnerLayer.android.kt:274)
at androidx.compose.ui.graphics.layer.GraphicsLayer.drawWithChildTracking(AndroidGraphicsLayer.android.kt:438)
at androidx.compose.ui.graphics.layer.GraphicsLayer.access$drawWithChildTracking(AndroidGraphicsLayer.android.kt:55)
at androidx.compose.ui.graphics.layer.GraphicsLayer$clipDrawBlock$1.invoke(AndroidGraphicsLayer.android.kt:68)
at androidx.compose.ui.graphics.layer.GraphicsLayer$clipDrawBlock$1.invoke(AndroidGraphicsLayer.android.kt:63)
at androidx.compose.ui.graphics.layer.GraphicsLayerV29.record(GraphicsLayerV29.android.kt:251)
at androidx.compose.ui.graphics.layer.GraphicsLayer.recordInternal(AndroidGraphicsLayer.android.kt:431)
at androidx.compose.ui.graphics.layer.GraphicsLayer.record-mL-hObY(AndroidGraphicsLayer.android.kt:427)
at androidx.compose.ui.platform.GraphicsLayerOwnerLayer.updateDisplayList(GraphicsLayerOwnerLayer.android.kt:269)
at androidx.compose.ui.platform.AndroidComposeView.dispatchDraw(AndroidComposeView.android.kt:2081)
at android.view.View.draw(View.java:26345)
at android.view.View.updateDisplayListIfDirty(View.java:25175)
at android.view.View.draw(View.java:26072)
at android.view.ViewGroup.drawChild(ViewGroup.java:4810)
at android.view.ViewGroup.dispatchDraw(ViewGroup.java:4564)
at android.view.View.updateDisplayListIfDirty(View.java:25161)
at android.view.View.draw(View.java:26072)
at android.view.ViewGroup.drawChild(ViewGroup.java:4810)
at android.view.ViewGroup.dispatchDraw(ViewGroup.java:4564)
at android.view.View.updateDisplayListIfDirty(View.java:25161)
at android.view.View.draw(View.java:26072)
at android.view.ViewGroup.drawChild(ViewGroup.java:4810)
at android.view.ViewGroup.dispatchDraw(ViewGroup.java:4564)
at android.view.View.updateDisplayListIfDirty(View.java:25161)
at android.view.View.draw(View.java:26072)
at android.view.ViewGroup.drawChild(ViewGroup.java:4810)
at android.view.ViewGroup.dispatchDraw(ViewGroup.java:4564)
at com.android.internal.policy.DecorView.dispatchDraw(DecorView.java:1211)
at android.view.View.draw(View.java:26345)
at com.android.internal.policy.DecorView.draw(DecorView.java:1193)
at android.view.View.updateDisplayListIfDirty(View.java:25175)
at android.view.ThreadedRenderer.updateViewTreeDisplayList(ThreadedRenderer.java:694)
at android.view.ThreadedRenderer.updateRootDisplayList(ThreadedRenderer.java:700)
2026-01-18 22:40:50.828 16558-16558 AndroidRuntime          app.rive.mpapp                       E  	at android.view.ThreadedRenderer.draw(ThreadedRenderer.java:798)
at android.view.ViewRootImpl.draw(ViewRootImpl.java:7016)
at android.view.ViewRootImpl.performDraw(ViewRootImpl.java:6632)
at android.view.ViewRootImpl.performTraversals(ViewRootImpl.java:5531)
at android.view.ViewRootImpl.doTraversal(ViewRootImpl.java:3924)
at android.view.ViewRootImpl$TraversalRunnable.run(ViewRootImpl.java:12903)
at android.view.Choreographer$CallbackRecord.run(Choreographer.java:1901)
at android.view.Choreographer$CallbackRecord.run(Choreographer.java:1910)
at android.view.Choreographer.doCallbacks(Choreographer.java:1367)
at android.view.Choreographer.doFrame(Choreographer.java:1292)
at android.view.Choreographer$FrameDisplayEventReceiver.run(Choreographer.java:1870)
at android.os.Handler.handleCallback(Handler.java:995)
at android.os.Handler.dispatchMessage(Handler.java:103)
at android.os.Looper.loopOnce(Looper.java:273)
at android.os.Looper.loop(Looper.java:363)
at android.app.ActivityThread.main(ActivityThread.java:10060)
at java.lang.reflect.Method.invoke(Native Method)
at com.android.internal.os.RuntimeInit$MethodAndArgsCaller.run(RuntimeInit.java:632)
at com.android.internal.os.ZygoteInit.main(ZygoteInit.java:975)
2026-01-18 22:40:50.834 16558-16558 Process                 app.rive.mpapp                       I  Sending signal. PID: 16558 SIG: 9
---------------------------- PROCESS ENDED (16558) for package app.rive.mpapp ----------------------------