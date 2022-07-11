#include "stdafx.h"
#ifdef OE
#include "..\feature.hpp"
#include "..\cvars.hpp"

typedef void(__fastcall* _CViewRender__OnRenderStart)(void* thisptr, int edx);

ConVar _y_spt_force_fov("_y_spt_force_fov", "0", 0, "Force FOV to some value.");

namespace patterns::client
{
	PATTERNS(
	    CViewRender__OnRenderStart,
	    "2707",
	    "83 EC 38 56 8B F1 8B 0D ?? ?? ?? ?? D9 41 28 57 D8 1D ?? ?? ?? ?? DF E0 F6 C4 05 7A 35 A1 ?? ?? ?? ?? D9 40 28 8B 0D",
	    "missinginfo1_4_7",
	    "55 8B EC 83 EC 24 89 4D E0 B9 ?? ?? ?? ?? E8 ?? ?? ?? ?? D8 1D ?? ?? ?? ?? DF E0 F6 C4 05 7A 5A");
}

// FOV related features
class FOVFeatures : public FeatureWrapper<FOVFeatures>
{
public:
protected:
	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

	virtual void PreHook() override;

private:
	_CViewRender__OnRenderStart ORIG_CViewRender__OnRenderStart = nullptr;
	static void __fastcall HOOKED_CViewRender__OnRenderStart(void* thisptr, int edx);
};

static FOVFeatures spt_fov;

void FOVFeatures::InitHooks()
{
	HOOK_FUNCTION(client, CViewRender__OnRenderStart);
}

void FOVFeatures::LoadFeature() {}

void FOVFeatures::UnloadFeature() {}

void FOVFeatures::PreHook()
{
	if (_viewmodel_fov && ORIG_CViewRender__OnRenderStart)
		InitConcommandBase(_y_spt_force_fov);
}

void __fastcall FOVFeatures::HOOKED_CViewRender__OnRenderStart(void* thisptr, int edx)
{
	spt_fov.ORIG_CViewRender__OnRenderStart(thisptr, edx);

	if (!_viewmodel_fov || !_y_spt_force_fov.GetBool())
		return;

	float* fov = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(thisptr) + 52);
	float* fovViewmodel = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(thisptr) + 56);
	*fov = _y_spt_force_fov.GetFloat();
	*fovViewmodel = _viewmodel_fov->GetFloat();
}
#endif