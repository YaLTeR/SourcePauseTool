#pragma once

#include "thirdparty/Signal.h"
#include "edict.h"

class CViewSetup;

extern Gallant::Signal0<void> AdjustAngles;
extern Gallant::Signal0<void> AfterFramesSignal;
extern Gallant::Signal1<void*> FinishRestoreSignal;
extern Gallant::Signal0<void> FrameSignal;
extern Gallant::Signal0<void> JumpSignal;
extern Gallant::Signal1<bool> OngroundSignal;
extern Gallant::Signal2<void*, bool> SetPausedSignal;
extern Gallant::Signal1<bool> SV_ActivateServerSignal;
extern Gallant::Signal1<uintptr_t> CreateMoveSignal;
extern Gallant::Signal1<uintptr_t> DecodeUserCmdFromBufferSignal;
extern Gallant::Signal0<void> VagCrashSignal;
extern Gallant::Signal0<void> DemoStartPlaybackSignal;
extern Gallant::Signal1<bool> SV_FrameSignal;
extern Gallant::Signal2<void*, void*> ProcessMovementPost_Signal;
extern Gallant::Signal2<void*, void*> ProcessMovementPre_Signal;
extern Gallant::Signal2<void*, CViewSetup*> RenderViewPre_Signal;
extern Gallant::Signal2<void*, int> SetSignonStateSignal;

// Plugin callbacks
extern Gallant::Signal1<bool> TickSignal;
extern Gallant::Signal1<char const*> LevelInitSignal;
extern Gallant::Signal3<edict_t*, int, int> ServerActivateSignal;
extern Gallant::Signal0<void> LevelShutdownSignal;
extern Gallant::Signal1<edict_t*> OnEdictAllocatedSignal;
extern Gallant::Signal2<edict_t*, char const*> ClientPutInServerSignal;
extern Gallant::Signal1<edict_t*> ClientActiveSignal;
extern Gallant::Signal1<edict_t*> ClientDisconnectSignal;
extern Gallant::Signal1<edict_t*> ClientSettingsChangedSignal;