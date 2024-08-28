#include <stdafx.hpp>
#include "spt/feature.hpp"
#include "spt/utils/game_detection.hpp"
#include "imgui/imgui_interface.hpp"

#define _STR(x) #x

// a gui for everything in game_detection.hpp
class GameDetectionFeatureGui : public FeatureWrapper<GameDetectionFeatureGui>
{
protected:
	virtual void LoadFeature()
	{
		SptImGui::RegisterSectionCallback(SptImGuiGroup::Dev_GameDetection, ImGuiCallback);
	}

private:
	static void TableRow(const char* text, bool val)
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(text);
		ImGui::TableNextColumn();
		ImGui::BeginDisabled(true);
		char buf[16];
		snprintf(buf, sizeof buf, "##%s", text);
		ImGui::Checkbox(buf, &val);
		ImGui::EndDisabled();
	}

	static void ImGuiCallback(bool open)
	{
		if (!open)
			return;
		using namespace utils;
		ImGui::Text("Detected game build number: %d", GetBuildNumber());
		ImGuiTableFlags tableFlags =
		    ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders | ImGuiTableFlags_NoHostExtendX;
		if (ImGui::BeginTable("game detection [table]", 2, tableFlags))
		{
			ImGui::TableSetupColumn("Game");
			ImGui::TableSetupColumn("Looks like?");
			ImGui::TableHeadersRow();
			TableRow("Portal 1", DoesGameLookLikePortal());
			TableRow("DMoMM", DoesGameLookLikeDMoMM());
			TableRow("HLS", DoesGameLookLikeDMoMM());
			TableRow("BMS (retail)", DoesGameLookLikeDMoMM());
			TableRow("BMS (latest)", DoesGameLookLikeDMoMM());
			TableRow("BMS (mod)", DoesGameLookLikeDMoMM());
			TableRow("Estranged", DoesGameLookLikeDMoMM());
			ImGui::EndTable();
		}

#ifdef SSDK2007
		ImGui::Text("#define " _STR(SSDK2007) " %d", SSDK2007);
#endif
#ifdef SSDK2013
		ImGui::Text("#define " _STR(SSDK2013) " %d", SSDK2013);
#endif
#ifdef BMS
		ImGui::Text("#define " _STR(BMS) " %d", BMS);
#endif
#ifdef OE
		ImGui::Text("#define " _STR(OE) " %d", OE);
#endif
	}
};

static GameDetectionFeatureGui spt_gameDetection_feat;
