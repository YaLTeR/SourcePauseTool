#include "stdafx.h"
#include "signals.hpp"

Gallant::Signal0<void> AdjustAngles;
Gallant::Signal0<void> AfterFramesSignal;
Gallant::Signal2<void*, int> FinishRestoreSignal;
Gallant::Signal0<void> FrameSignal;
Gallant::Signal0<void> JumpSignal;
Gallant::Signal1<bool> OngroundSignal;
Gallant::Signal0<void> TickSignal;
Gallant::Signal3<void*, int, bool> SetPausedSignal;
Gallant::Signal1<bool> SV_ActivateServerSignal;