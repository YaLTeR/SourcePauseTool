#pragma once

#include "thirdparty/Signal.h"

extern Gallant::Signal0<void> AdjustAngles;
extern Gallant::Signal0<void> AfterFramesSignal;
extern Gallant::Signal2<void*, int> FinishRestoreSignal;
extern Gallant::Signal0<void> FrameSignal;
extern Gallant::Signal0<void> JumpSignal;
extern Gallant::Signal1<bool> OngroundSignal;
extern Gallant::Signal0<void> TickSignal;
extern Gallant::Signal3<void*, int, bool> SetPausedSignal;
extern Gallant::Signal1<bool> SV_ActivateServerSignal;