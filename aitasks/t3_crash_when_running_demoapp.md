I am trying to run RiveDemo.kt from the mpapp module and I nothing is actually drawn on screen. when looking at the log file I got the following meesages. 

2026-01-19 12:14:53.825 28828-29323 rive-mp                 app.rive.mpapp                       I  CommandServer: Handling AdvanceStateMachine command (smHandle=549326796480, deltaTime=0.008509)
2026-01-19 12:14:53.825 28828-29323 rive-mp                 app.rive.mpapp                       W  CommandServer: Invalid state machine handle: 549326796480
2026-01-19 12:14:53.825 28828-29323 rive-mp                 app.rive.mpapp                       I  CommandServer: Handling Draw command (requestID=-5476376663331770480, artboard=2, sm=549326796480, 984x1332)
2026-01-19 12:14:53.825 28828-29323 rive-mp                 app.rive.mpapp                       W  CommandServer: Invalid state machine handle: 549326796480
2026-01-19 12:14:53.833 28828-28828 rive-mp                 app.rive.mpapp                       W  CommandQueue JNI: Unknown message type: 59
2026-01-19 12:14:53.833 28828-28828 rive-mp                 app.rive.mpapp                       I  CommandServer: Enqueuing AdvanceStateMachine command (smHandle=549326796480, deltaTime=0.008509)
2026-01-19 12:14:53.833 28828-28828 rive-mp                 app.rive.mpapp                       I  CommandServer: Enqueuing Draw command (requestID=-5476376663331770480, artboard=2, sm=549326796480)
2026-01-19 12:14:53.834 28828-29323 rive-mp                 app.rive.mpapp                       I  CommandServer: Handling AdvanceStateMachine command (smHandle=549326796480, deltaTime=0.008509)
2026-01-19 12:14:53.834 28828-29323 rive-mp                 app.rive.mpapp                       W  CommandServer: Invalid state machine handle: 549326796480
2026-01-19 12:14:53.834 28828-29323 rive-mp                 app.rive.mpapp                       I  CommandServer: Handling Draw command (requestID=-5476376663331770480, artboard=2, sm=549326796480, 984x1332)
2026-01-19 12:14:53.834 28828-29323 rive-mp                 app.rive.mpapp                       W  CommandServer: Invalid state machine handle: 549326796480
2026-01-19 12:14:53.842 28828-28828 rive-mp                 app.rive.mpapp                       W  CommandQueue JNI: Unknown message type: 59
2026-01-19 12:14:53.842 28828-28828 rive-mp                 app.rive.mpapp                       I  CommandServer: Enqueuing AdvanceStateMachine command (smHandle=549326796480, deltaTime=0.008509)
2026-01-19 12:14:53.842 28828-28828 rive-mp                 app.rive.mpapp                       I  CommandServer: Enqueuing Draw command (requestID=-5476376663331770480, artboard=2, sm=549326796480)
2026-01-19 12:14:53.842 28828-29323 rive-mp                 app.rive.mpapp                       I  CommandServer: Handling AdvanceStateMachine command (smHandle=549326796480, deltaTime=0.008509)
2026-01-19 12:14:53.842 28828-29323 rive-mp                 app.rive.mpapp                       W  CommandServer: Invalid state machine handle: 549326796480