#include "stdafx.h"
#include "signals.hpp"

Gallant::Signal0<void> AdjustAngles;
Gallant::Signal0<void> AfterFramesSignal;
Gallant::Signal1<void*> FinishRestoreSignal;
Gallant::Signal0<void> FrameSignal;
Gallant::Signal0<void> JumpSignal;
Gallant::Signal1<bool> OngroundSignal;
Gallant::Signal0<void> TickSignal;
Gallant::Signal2<void*, bool> SetPausedSignal;
Gallant::Signal1<bool> SV_ActivateServerSignal;
Gallant::Signal1<uintptr_t> CreateMoveSignal;
Gallant::Signal0<void> VagCrashSignal;
Gallant::Signal0<void> DemoStartPlaybackSignal;