#include "stdafx.hpp"
#ifdef _WIN32
#include "..\feature.hpp"
#include <Windows.h>

CON_COMMAND(y_spt_release_mutex, "Releases \"hl2_singleton_mutex\" to enable running multiple instances.")
{
	HANDLE handle = OpenMutexA(SYNCHRONIZE, false, "hl2_singleton_mutex");
	if (handle)
	{
		if (ReleaseMutex(handle))
			Msg("Released hl2_singleton_mutex. You can start another instance now.\n");
		else
			Warning("Failed to release hl2_singleton_mutex.\n");
		CloseHandle(handle);
	}
	else
		Warning("Failed to obtain hl2_singleton_mutex handle.\n");
}

// spt_release_mutex
class MultiInstance : public FeatureWrapper<MultiInstance>
{
public:
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;
};

static MultiInstance spt_multi_instance;

bool MultiInstance::ShouldLoadFeature()
{
	return true;
}

void MultiInstance::InitHooks() {}

void MultiInstance::LoadFeature()
{
	InitCommand(y_spt_release_mutex);
}

void MultiInstance::UnloadFeature() {}

#endif