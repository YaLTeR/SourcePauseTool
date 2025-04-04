#include "stdafx.hpp"
#include "signals.hpp"

Gallant::Signal0<void> AdjustAngles;
Gallant::Signal0<void> AfterFramesSignal;
Gallant::Signal1<void*> FinishRestoreSignal;
Gallant::Signal0<void> FrameSignal;
Gallant::Signal0<void> JumpSignal;
Gallant::Signal1<bool> OngroundSignal;
Gallant::Signal2<void*, bool> SetPausedSignal;
Gallant::Signal1<bool> SV_ActivateServerSignal;
Gallant::Signal1<uintptr_t> CreateMoveSignal;
Gallant::Signal1<uintptr_t> DecodeUserCmdFromBufferSignal;
Gallant::Signal0<void> VagCrashSignal;
Gallant::Signal0<void> DemoStartPlaybackSignal;
Gallant::Signal1<bool> SV_FrameSignal;
Gallant::Signal2<void*, void*> ProcessMovementPost_Signal;
Gallant::Signal2<void*, void*> ProcessMovementPre_Signal;
Gallant::Signal2<void*, CViewSetup*> RenderViewPre_Signal;
Gallant::Signal2<void*, int> SetSignonStateSignal;

// Plugin callbacks
Gallant::Signal1<bool> TickSignal;
Gallant::Signal1<char const*> LevelInitSignal;
Gallant::Signal3<edict_t*, int, int> ServerActivateSignal;
Gallant::Signal0<void> LevelShutdownSignal;
Gallant::Signal1<edict_t*> OnEdictAllocatedSignal;
Gallant::Signal2<edict_t*, char const*> ClientPutInServerSignal;
Gallant::Signal1<edict_t*> ClientActiveSignal;
Gallant::Signal1<edict_t*> ClientDisconnectSignal;
Gallant::Signal1<edict_t*> ClientSettingsChangedSignal;