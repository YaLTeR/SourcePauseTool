#pragma once

#include "thirdparty/Signal.h"

class MeshRenderer;

extern Gallant::Signal0<void> AdjustAngles;
extern Gallant::Signal0<void> AfterFramesSignal;
extern Gallant::Signal1<void*> FinishRestoreSignal;
extern Gallant::Signal0<void> FrameSignal;
extern Gallant::Signal0<void> JumpSignal;
extern Gallant::Signal1<bool> OngroundSignal;
extern Gallant::Signal0<void> TickSignal;
extern Gallant::Signal2<void*, bool> SetPausedSignal;
extern Gallant::Signal1<bool> SV_ActivateServerSignal;
extern Gallant::Signal1<uintptr_t> CreateMoveSignal;
extern Gallant::Signal0<void> VagCrashSignal;
extern Gallant::Signal0<void> DemoStartPlaybackSignal;
extern Gallant::Signal1<bool> SV_FrameSignal;
extern Gallant::Signal2<void*, void*> ProcessMovementPost_Signal;
extern Gallant::Signal2<void*, void*> ProcessMovementPre_Signal;
extern Gallant::Signal2<void*, struct vrect_t*> RenderSignal;
extern Gallant::Signal1<MeshRenderer&> MeshRenderSignal;
