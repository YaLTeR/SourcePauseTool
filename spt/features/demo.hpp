#pragma once
#include "..\feature.hpp"

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

	DECL_HOOK_CDECL(void, Stop);

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

	DECL_HOOK_THISCALL(void, StopRecording);
	DECL_HOOK_THISCALL(void, SetSignonState, int state);
	uintptr_t ORIG_Record = 0;
	void OnTick();
};

extern DemoStuff spt_demostuff;
