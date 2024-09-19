#include "stdafx.hpp"
#ifdef _WIN32

#include "..\feature.hpp"
#include "signals.hpp"
#include "..\visualizations\imgui\imgui_interface.hpp"
#include <Windows.h>

static bool SptReleaseMutexImpl(const char** msg)
{
	HANDLE handle = OpenMutexA(SYNCHRONIZE, false, "hl2_singleton_mutex");
	if (handle)
	{
		if (ReleaseMutex(handle))
		{
			*msg = "Released hl2_singleton_mutex. You can start another instance now.";
			return true;
		}
		else
		{
			*msg = "Failed to release hl2_singleton_mutex.";
			return false;
		}
		CloseHandle(handle);
	}
	else
	{
		*msg = "Failed to obtain hl2_singleton_mutex handle.";
		return false;
	}
}

CON_COMMAND(y_spt_release_mutex, "Releases \"hl2_singleton_mutex\" to enable running multiple instances.")
{
	const char* msg;
	if (SptReleaseMutexImpl(&msg))
		Msg("%s\n", msg);
	else
		Warning("%s\n", msg);
}

// spt_release_mutex
class MultiInstance : public FeatureWrapper<MultiInstance>
{
public:
	// I don't really understand why, but trying to release the mutex from
	// within the ImGui callback doesn't work - do it from FrameSignal instead.
	inline static const char* lastMsg = "N/A";
	inline static bool lastResult = true;
	inline static bool tryRelease = false;

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	void OnFrameSignal();
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

	bool imguiEnabled = SptImGuiGroup::QoL_MultiInstance.RegisterUserCallback(
	    []()
	    {
		    if (ImGui::Button("Release HL2 mutex"))
			    tryRelease = true;
		    ImGui::SameLine();
		    SptImGui::HelpMarker("Help text for %s:\n\n%s",
		                         WrangleLegacyCommandName(y_spt_release_mutex_command.GetName(), true, nullptr),
		                         y_spt_release_mutex_command.GetHelpText());
		    ImGui::Text("Last result:");
		    ImGui::SameLine();
		    if (lastResult)
			    ImGui::Text("%s", lastMsg);
		    else
			    ImGui::TextColored(SPT_IMGUI_WARN_COLOR_YELLOW, "%s", lastMsg);
	    });

	if (imguiEnabled)
		FrameSignal.Connect(this, &MultiInstance::OnFrameSignal);
}

void MultiInstance::OnFrameSignal()
{
	if (tryRelease)
		lastResult = SptReleaseMutexImpl(&lastMsg);
	tryRelease = false;
}

void MultiInstance::UnloadFeature() {}

#endif