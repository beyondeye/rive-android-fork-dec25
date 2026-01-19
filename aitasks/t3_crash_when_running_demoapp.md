I am trying to run RiveDemo.kt from the mpapp module and I nothing is actually drawn on screen. when looking at the log file I got the following meesages. 

ommandServer: Enqueuing AdvanceStateMachine command (requestID=549326796480, smHandle=8508657, deltaTime=0.000000)
2026-01-19 08:37:22.500   755-1540  rive-mp                 app.rive.mpapp                       I  CommandServer: Handling AdvanceStateMachine command (requestID=549326796480, smHandle=8508657, deltaTime=0.000000)
2026-01-19 08:37:22.500   755-1540  rive-mp                 app.rive.mpapp                       W  CommandServer: Invalid state machine handle: 8508657
2026-01-19 08:37:22.500   755-755   rive-mp                 app.rive.mpapp                       I  CommandServer: Enqueuing Draw command (requestID=1969, artboard=2, sm=549326796480)
2026-01-19 08:37:22.500   755-1540  rive-mp                 app.rive.mpapp                       I  CommandServer: Handling Draw command (requestID=1969, artboard=2, sm=549326796480, 984x1332)
2026-01-19 08:37:22.500   755-1540  rive-mp                 app.rive.mpapp                       W  CommandServer: Invalid state machine handle: 549326796480
2026-01-19 08:37:22.508   755-755   rive-mp                 app.rive.mpapp                       W  CommandQueue JNI: Unknown message type: 59
2026-01-19 08:37:22.508   755-755   rive-mp                 app.rive.mpapp                       I  CommandServer: Enqueuing AdvanceStateMachine command (requestID=549326796480, smHandle=8508657, deltaTime=0.000000)
2026-01-19 08:37:22.508   755-1540  rive-mp                 app.rive.mpapp                       I  CommandServer: Handling AdvanceStateMachine command (requestID=549326796480, smHandle=8508657, deltaTime=0.000000)
2026-01-19 08:37:22.508   755-1540  rive-mp                 app.rive.mpapp                       W  CommandServer: Invalid state machine handle: 8508657
2026-01-19 08:37:22.508   755-755   rive-mp                 app.rive.mpapp                       I  CommandServer: Enqueuing Draw command (requestID=1970, artboard=2, sm=549326796480)
2026-01-19 08:37:22.508   755-1540  rive-mp                 app.rive.mpapp                       I  CommandServer: Handling Draw command (requestID=1970, artboard=2, sm=549326796480, 984x1332)
2026-01-19 08:37:22.508   755-1540  rive-mp                 app.rive.mpapp                       W  CommandServer: Invalid state machine handle: 549326796480
2026-01-19 08:37:22.516   755-755   rive-mp                 app.rive.mpapp                       W  CommandQueue JNI: Unknown message type: 59
2026-01-19 08:37:22.517   755-755   rive-mp                 app.rive.mpapp                       I  CommandServer: Enqueuing AdvanceStateMachine command (requestID=549326796480, smHandle=8508657, deltaTime=0.000000)
2026-01-19 08:37:22.518   755-1540  rive-mp                 app.rive.mpapp                       I  CommandServer: Handling AdvanceStateMachine command (requestID=549326796480, smHandle=8508657, deltaTime=0.000000)
2026-01-19 08:37:22.518   755-1540  rive-mp                 app.rive.mpapp                       W  CommandServer: Invalid state machine handle: 8508657
2026-01-19 08:37:22.518   755-755   rive-mp                 app.rive.mpapp                       I  CommandServer: Enqueuing Draw command (requestID=1971, artboard=2, sm=549326796480)
2026-01-19 08:37:22.518   755-1540  rive-mp                 app.rive.mpapp                       I  CommandServer: Handling Draw command (requestID=1971, artboard=2, sm=549326796480, 984x1332)
2026-01-19 08:37:22.518   755-1540  rive-mp                 app.rive.mpapp                       W  CommandServer: Invalid state machine handle: 549326796480
2026-01-19 08:37:22.525   755-755   rive-mp                 app.rive.mpapp                       W  CommandQueue JNI: Unknown message type: 59
2026-01-19 08:37:22.526   755-755   rive-mp                 app.rive.mpapp                       I  CommandServer: Enqueuing AdvanceStateMachine command (requestID=549326796480, smHandle=8508657, deltaTime=0.000000)
2026-01-19 08:37:22.526   755-1540  rive-mp                 app.rive.mpapp                       I  CommandServer: Handling AdvanceStateMachine command (requestID=549326796480, smHandle=8508657, deltaTime=0.000000)
2026-01-19 08:37:22.526   755-1540  rive-mp                 app.rive.mpapp                       W  CommandServer: Invalid state machine handle: 8508657
2026-01-19 08:37:22.527   755-755   rive-mp                 app.rive.mpapp                       I  CommandServer: Enqueuing Draw command (requestID=1972, artboard=2, sm=549326796480)
2026-01-19 08:37:22.527   755-1540  rive-mp                 app.rive.mpapp                       I  CommandServer: Handling Draw command (requestID=1972, artboard=2, sm=549326796480, 984x1332)
2026-01-19 08:37:22.527   755-1540  rive-mp                 app.rive.mpapp                       W  CommandServer: Invalid state machine handle: 549326796480
2026-01-19 08:37:22.534   755-755   rive-mp                 app.rive.mpapp                       W  CommandQueue JNI: Unknown message type: 59
2026-01-19 08:37:22.535   755-755   rive-mp                 app.rive.mpapp                       I  CommandServer: Enqueuing AdvanceStateMachine command (requestID=549326796480, smHandle=8508657, deltaTime=0.000000)
2026-01-19 08:37:22.535   755-1540  rive-mp                 app.rive.mpapp                       I  CommandServer: Handling AdvanceStateMachine command (requestID=549326796480, smHandle=8508657, deltaTime=0.000000)
2026-01-19 08:37:22.535   755-1540  rive-mp                 app.rive.mpapp                       W  CommandServer: Invalid state machine handle: 8508657
2026-01-19 08:37:22.535   755-755   rive-mp                 app.rive.mpapp                       I  CommandServer: Enqueuing Draw command (requestID=1973, artboard=2, sm=549326796480)
2026-01-19 08:37:22.535   755-1540  rive-mp                 app.rive.mpapp                       I  CommandServer: Handling Draw command (requestID=1973, artboard=2, sm=549326796480, 984x1332)
2026-01-19 08:37:22.535   755-1540  rive-mp                 app.rive.mpapp                       W  CommandServer: Invalid state machine handle: 549326796480
2026-01-19 08:37:22.542   755-755   rive-mp                 app.rive.mpapp                       W  CommandQueue JNI: Unknown message type: 59
2026-01-19 08:37:22.543   755-755   rive-mp                 app.rive.mpapp                       I  CommandServer: Enqueuing AdvanceStateMachine command (requestID=549326796480, smHandle=8508657, deltaTime=0.000000)
2026-01-19 08:37:22.543   755-1540  rive-mp                 app.rive.mpapp                       I  CommandServer: Handling AdvanceStateMachine command (requestID=549326796480, smHandle=8508657, deltaTime=0.000000)
2026-01-19 08:37:22.543   755-1540  rive-mp                 app.rive.mpapp                       W  CommandServer: Invalid state machine handle: 8508657
2026-01-19 08:37:22.543   755-755   rive-mp                 app.rive.mpapp                       I  CommandServer: Enqueuing Draw command (requestID=1974, artboard=2, sm=549326796480)
2026-01-19 08:37:22.543   755-1540  rive-mp                 app.rive.mpapp                       I  CommandServer: Handling Draw command (requestID=1974, artboard=2, sm=549326796480, 984x1332)
2026-01-19 08:37:22.543   755-1540  rive-mp                 app.rive.mpapp                       W  CommandServer: Invalid state machine handle: 549326796480
2026-01-19 08:37:22.551   755-755   rive-mp                 app.rive.mpapp                       W  CommandQueue JNI: Unknown message type: 59
2026-01-19 08:37:22.552   755-755   rive-mp                 app.rive.mpapp                       I  CommandServer: Enqueuing AdvanceStateMachine command (requestID=549326796480, smHandle=8508657, deltaTime=0.000000)
2026-01-19 08:37:22.552   755-1540  rive-mp                 app.rive.mpapp                       I  CommandServer: Handling AdvanceStateMachine command (requestID=549326796480, smHandle=8508657, deltaTime=0.000000)
2026-01-19 08:37:22.552   755-1540  rive-mp                 app.rive.mpapp                       W  CommandServer: Invalid state machine handle: 8508657
2026-01-19 08:37:22.552   755-755   rive-mp                 app.rive.mpapp                       I  CommandServer: Enqueuing Draw command (requestID=1975, artboard=2, sm=549326796480)
2026-01-19 08:37:22.552   755-1540  rive-mp                 app.rive.mpapp                       I  CommandServer: Handling Draw command (requestID=1975, artboard=2, sm=549326796480, 984x1332)
2026-01-19 08:37:22.552   755-1540  rive-mp                 app.rive.mpapp                       W  CommandServer: Invalid state machine handle: 549326796480
2026-01-19 08:37:22.559   755-755   rive-mp                 app.rive.mpapp                       W  CommandQueue JNI: Unknown message type: 59
2026-01-19 08:37:22.560   755-755   rive-mp                 app.rive.mpapp                       I  CommandServer: Enqueuing AdvanceStateMachine command (requestID=549326796480, smHandle=8508657, deltaTime=0.000000)
2026-01-19 08:37:22.560   755-755   rive-mp                 app.rive.mpapp                       I  CommandServer: Enqueuing Draw command (requestID=1976, artboard=2, sm=549326796480)
2026-01-19 08:37:22.561   755-1540  rive-mp                 app.rive.mpapp                       I  CommandServer: Handling AdvanceStateMachine command (requestID=549326796480, smHandle=8508657, deltaTime=0.000000)
2026-01-19 08:37:22.561   755-1540  rive-mp                 app.rive.mpapp                       W  CommandServer: Invalid state machine handle: 8508657
2026-01-19 08:37:22.561   755-1540  rive-mp                 app.rive.mpapp                       I  CommandServer: Handling Draw command (requestID=1976, artboard=2, sm=549326796480, 984x1332)
2026-01-19 08:37:22.561   755-1540  rive-mp                 app.rive.mpapp                       W  CommandServer: Invalid state machine handle: 549326796480
2026-01-19 08:37:22.568   755-755   rive-mp                 app.rive.mpapp                       W  CommandQueue JNI: Unknown message type: 59