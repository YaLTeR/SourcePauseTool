#include "stdafx.hpp"
#include "..\feature.hpp"

#include "spt\utils\game_detection.hpp"
#include "spt\utils\portal_utils.hpp"
#include "visualizations\imgui\imgui_interface.hpp"
#include "ent_props.hpp"

#ifdef SPT_PORTAL_UTILS

class SetSgFeature : public FeatureWrapper<SetSgFeature>
{
public:
	static const utils::PortalInfo* SetSg(const char* portal, const char** errMsg);
	static const utils::PortalInfo* SetSg(int portalIdx, const char** errMsg);
	static void ClearSg();

protected:
	virtual bool ShouldLoadFeature() override
	{
		return utils::DoesGameLookLikePortal();
	};

	virtual void LoadFeature() override;

private:
	static void ImGuiCallback();
};

static SetSgFeature spt_set_sg_feat;

constexpr int SPT_PORTAL_SELECT_FLAGS = GPF_NONE;

CON_COMMAND_F(spt_set_sg,
              "Set the SG to the given portal. Valid options are:\n" SPT_PORTAL_SELECT_DESCRIPTION
              "\n  - clear: clears SG",
              FCVAR_DONTRECORD)
{
	if (args.ArgC() != 2)
	{
		Msg("Usage: %s [blue/orange/<index>/clear]\n", spt_set_sg_command.GetName());
		return;
	}
	const char* errMsg = nullptr;
	auto sgPortal = spt_set_sg_feat.SetSg(args[1], &errMsg);
	if (sgPortal)
		Msg("Set SG on portal index %d\n", sgPortal->handle.GetEntryIndex());
	else if (errMsg)
		Warning("Failed to set SG: %s\n", errMsg);
}

const utils::PortalInfo* SetSgFeature::SetSg(const char* portal, const char** errMsg)
{
	auto serverPlayer = utils::spt_serverEntList.GetPlayer();
	if (!serverPlayer)
	{
		*errMsg = "player not found.";
		return nullptr;
	}

	static utils::CachedField<CBaseHandle, "CPortal_Player", "m_hPortalEnvironment", true> fPortalEnv;
	if (!fPortalEnv.Exists())
	{
		*errMsg = "CPortal_Player::m_hPortalEnvironment not found.";
		return nullptr;
	}

	if (!stricmp(portal, "clear"))
	{
		fPortalEnv.GetPtr(serverPlayer)->Term();
		return nullptr;
	}

	auto newSgPortal = getPortal(portal, SPT_PORTAL_SELECT_FLAGS);

	if (!newSgPortal)
	{
		*errMsg = "portal not found.";
		return nullptr;
	}

	*fPortalEnv.GetPtr(serverPlayer) = newSgPortal->handle;
	return newSgPortal;
}

const utils::PortalInfo* SetSgFeature::SetSg(int portalIdx, const char** errMsg)
{
	char buf[32];
	snprintf(buf, sizeof buf, "%d", portalIdx);
	return SetSg(buf, errMsg);
}

void SetSgFeature::ClearSg()
{
	const char* dummy;
	SetSg("clear", &dummy);
}

void SetSgFeature::LoadFeature()
{
	InitCommand(spt_set_sg);
	SptImGuiGroup::Cheats_SetSg.RegisterUserCallback(ImGuiCallback);
}

void SetSgFeature::ImGuiCallback()
{
	static SptImGui::TimedToolTip errTip;
	static SptImGui::PortalSelectionPersist persist;
	auto envPortal = utils::spt_serverEntList.GetEnvironmentPortal();
	persist.selectedPortalIdx = envPortal ? envPortal->handle.GetEntryIndex() : -1;
	SptImGui::PortalSelectionWidget(persist, SPT_PORTAL_SELECT_FLAGS);
	if (persist.userSelectedPortalByIndexLastCall)
	{
		if (spt_set_sg_feat.SetSg(persist.selectedPortalIdx, &errTip.text))
			errTip.StopShowing();
		else
			errTip.StartShowing();
	}
	errTip.Show(SPT_IMGUI_WARN_COLOR_YELLOW, 3);

	if (ImGui::Button("Clear SG"))
	{
		ClearSg();
		errTip.StopShowing();
	}
}

#endif
