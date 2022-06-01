#pragma once
#include "..\feature.hpp"

typedef int(__fastcall* DemoPlayer__Func)(void* thisptr);
typedef void(__fastcall* _StopRecording)(void* thisptr, int edx);
typedef void(__fastcall* _SetSignonState)(void* thisptr, int edx, int state);
typedef void(__cdecl* _Stop)();

// Various demo features
class DemoStuff : public FeatureWrapper<DemoStuff>
{
public:
	void Demo_StopRecording();
	int Demo_GetPlaybackTick() const;
	int Demo_GetTotalTicks() const;
	bool Demo_IsPlayingBack() const;
	bool Demo_IsPlaybackPaused() const;
	bool Demo_IsAutoRecordingAvailable() const;
	void StartAutorecord();
	void StopAutorecord();

	_Stop ORIG_Stop = nullptr;

protected:
	virtual bool ShouldLoadFeature() override;
	virtual void InitHooks() override;
	virtual void PreHook() override;
	virtual void LoadFeature() override;
	virtual void UnloadFeature() override;

private:
	int GetPlaybackTick_Offset = 0;
	int GetTotalTicks_Offset = 0;
	int IsPlayingBack_Offset = 0;
	int IsPlaybackPaused_Offset = 0;
	void** pDemoplayer = nullptr;
	int currentAutoRecordDemoNumber = 1;
	int m_nDemoNumber_Offset = 0;
	int m_bRecording_Offset = 0;
	bool isAutoRecordingDemo = false;

	_StopRecording ORIG_StopRecording = nullptr;
	_SetSignonState ORIG_SetSignonState = nullptr;
	uintptr_t ORIG_Record = 0;

	static void __fastcall HOOKED_StopRecording(void* thisptr, int edx);
	static void __fastcall HOOKED_SetSignonState(void* thisptr, int edx, int state);
	static void __cdecl HOOKED_Stop();
	void OnTick();
};

extern DemoStuff spt_demostuff;
