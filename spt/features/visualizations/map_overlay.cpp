#include "stdafx.hpp"

#include "renderer\mesh_renderer.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include "spt\utils\convar.hpp"
#include "spt\utils\game_detection.hpp"
#include "spt\utils\file.hpp"
#include "imgui\imgui_interface.hpp"
#include "thirdparty\imgui\imgui_internal.h"

#include "bspfile.h"

#include <stack>

#define SC_BOX_BRUSH ShapeColor(C_OUTLINE(0, 255, 255, 20))
#define SC_COMPLEX_BRUSH ShapeColor(C_OUTLINE(255, 0, 255, 20))

// Draw the brushes from another map
class MapOverlay : public FeatureWrapper<MapOverlay>
{
public:
	bool LoadMapFile(std::string filename, bool ztest, const char** err);
	void ClearMeshes();
	Vector offset;
	std::string lastLoadedFile;
	bool createdWithZTestMaterial;

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	void OnMeshRenderSignal(MeshRendererDelegate& mr);
	static void ImGuiCallback();

	std::vector<StaticMesh> meshes;
};

static MapOverlay spt_map_overlay;

CON_COMMAND_AUTOCOMPLETEFILE(spt_draw_map_overlay, "Draw the brushes from another map.", 0, "maps", ".bsp")
{
	int argc = args.ArgC();
	if (argc != 2 && argc != 3 && argc != 5 && argc != 6)
	{
		Msg("Usage: spt_draw_map_overlay <map | 0> [x y z] [ztest=1]\n");
		return;
	}
	if (argc == 2 && strcmp(args.Arg(1), "0") == 0)
	{
		spt_map_overlay.ClearMeshes();
		return;
	};

	bool ztest = true;
	if (argc == 3 || argc == 6)
		ztest = !!atoi(args.Arg(argc - 1));

	const char* err;
	if (!spt_map_overlay.LoadMapFile(args.Arg(1), ztest, &err))
	{
		Warning("%s\n", err);
	}
	else
	{
		if (argc == 5 || argc == 6)
		{
			spt_map_overlay.offset = {
			    (float)atof(args.Arg(2)),
			    (float)atof(args.Arg(3)),
			    (float)atof(args.Arg(4)),
			};
		}
		else
		{
			spt_map_overlay.offset = {0, 0, 0};
		}
	}
}

bool MapOverlay::LoadMapFile(std::string filename, bool ztest, const char** err)
{
	if (filename == lastLoadedFile && ztest == createdWithZTestMaterial && !meshes.empty())
		return true; // no need to reload

#define READ_BYTES(f, buffer, bytes) \
	{ \
		f.read((buffer), (bytes)); \
		if (f.gcount() != (bytes)) \
		{ \
			*err = "Unexpected EOF."; \
			return false; \
		} \
	} \
	while (0)

#define SEEK_TO_BYTE(f, offset) \
	{ \
		f.seekg((offset), std::ios::beg); \
		if (!f) \
		{ \
			*err = "Unexpected EOF."; \
			return false; \
		} \
	} \
	while (0)

	std::ifstream mapFile(GetGameDir() + "\\maps\\" + filename + ".bsp", std::ios::binary);
	if (!mapFile.is_open())
	{
		*err = "Cannot open file.";
		return false;
	}

	dheader_t header;
	READ_BYTES(mapFile, (char*)&header, sizeof(dheader_t));
	if (header.ident != IDBSPHEADER)
	{
		*err = "Not a bsp file.";
		return false;
	}

	if (header.version != 20 && header.version != 19
	    && !(utils::DoesGameLookLikeDMoMM() && (header.version >> 16) != 20))
	{
		*err = "Unsupported bsp version.";
		return false;
	}

	SEEK_TO_BYTE(mapFile, header.lumps[LUMP_PLANES].fileofs);
	std::vector<dplane_t> planes(header.lumps[LUMP_PLANES].filelen / sizeof(dplane_t));
	READ_BYTES(mapFile, (char*)planes.data(), header.lumps[LUMP_PLANES].filelen);

	SEEK_TO_BYTE(mapFile, header.lumps[LUMP_NODES].fileofs);
	std::vector<dnode_t> nodes(header.lumps[LUMP_NODES].filelen / sizeof(dnode_t));
	READ_BYTES(mapFile, (char*)nodes.data(), header.lumps[LUMP_NODES].filelen);

	SEEK_TO_BYTE(mapFile, header.lumps[LUMP_LEAFS].fileofs);
	std::vector<dleaf_version_0_t> leaves_v0;
	std::vector<dleaf_t> leaves;
	if (header.version < 20)
	{
		leaves_v0.resize(header.lumps[LUMP_LEAFS].filelen / sizeof(dleaf_version_0_t));
		READ_BYTES(mapFile, (char*)leaves_v0.data(), header.lumps[LUMP_LEAFS].filelen);
	}
	else
	{
		leaves.resize(header.lumps[LUMP_LEAFS].filelen / sizeof(dleaf_t));
		READ_BYTES(mapFile, (char*)leaves.data(), header.lumps[LUMP_LEAFS].filelen);
	}

	SEEK_TO_BYTE(mapFile, header.lumps[LUMP_LEAFBRUSHES].fileofs);
	std::vector<uint16_t> leafbrushes(header.lumps[LUMP_LEAFBRUSHES].filelen / sizeof(uint16_t));
	READ_BYTES(mapFile, (char*)leafbrushes.data(), header.lumps[LUMP_LEAFBRUSHES].filelen);

	SEEK_TO_BYTE(mapFile, header.lumps[LUMP_BRUSHES].fileofs);
	std::vector<dbrush_t> brushes(header.lumps[LUMP_BRUSHES].filelen / sizeof(dbrush_t));
	READ_BYTES(mapFile, (char*)brushes.data(), header.lumps[LUMP_BRUSHES].filelen);

	SEEK_TO_BYTE(mapFile, header.lumps[LUMP_BRUSHSIDES].fileofs);
	std::vector<dbrushside_t> brushsides(header.lumps[LUMP_BRUSHSIDES].filelen / sizeof(dbrushside_t));
	READ_BYTES(mapFile, (char*)brushsides.data(), header.lumps[LUMP_BRUSHSIDES].filelen);

	// Traverse the tree
	std::set<uint16_t> mapBrushesIndex;
	std::stack<uint16_t> s;
	int curr = 0;
	while (curr >= 0 || !s.empty())
	{
		while (curr >= 0)
		{
			s.push(curr);
			curr = nodes[curr].children[0];
		}

		int leafIndex = -1 - curr;
		int firstleafbrush =
		    (header.version < 20) ? leaves_v0[leafIndex].firstleafbrush : leaves[leafIndex].firstleafbrush;
		int numleafbrushes =
		    (header.version < 20) ? leaves_v0[leafIndex].numleafbrushes : leaves[leafIndex].numleafbrushes;
		for (int i = 0; i < numleafbrushes; i++)
		{
			mapBrushesIndex.insert(leafbrushes[firstleafbrush + i]);
		}

		curr = s.top();
		s.pop();
		curr = nodes[curr].children[1];
	}

	// Build meshes
	meshes.clear();

	static std::vector<VPlane> vplanes;

	auto brushIndexIter = mapBrushesIndex.begin();
	const auto brushIndexEnd = mapBrushesIndex.end();
	while (brushIndexIter != brushIndexEnd)
	{
		meshes.push_back(spt_meshBuilder.CreateStaticMesh(
		    [&](MeshBuilderDelegate& mb)
		    {
			    for (; brushIndexIter != brushIndexEnd; brushIndexIter++)
			    {
				    dbrush_t brush = brushes[*brushIndexIter];
				    bool isBox = (brush.numsides == 6);

				    if ((brush.contents & CONTENTS_SOLID) == 0)
					    continue;

				    vplanes.clear();
				    for (int i = 0; i < brush.numsides; i++)
				    {
					    dbrushside_t brushside = brushsides[brush.firstside + i];
					    if (brushside.bevel)
						    continue;
					    dplane_t plane = planes[brushside.planenum];
					    if (isBox && plane.type > 2)
						    isBox = false;
					    vplanes.emplace_back(plane.normal, plane.dist);
				    }

				    CPolyhedron* poly = GeneratePolyhedronFromPlanes((float*)vplanes.data(),
				                                                     vplanes.size(),
				                                                     0.0001f,
				                                                     true);
				    if (!poly)
					    continue;

				    ShapeColor color = isBox ? SC_BOX_BRUSH : SC_COMPLEX_BRUSH;
				    color.zTestFaces = ztest;
				    if (!mb.AddCPolyhedron(poly, color))
				    {
					    poly->Release();
					    break;
				    }
				    poly->Release();
			    }
		    }));
	}
	lastLoadedFile = std::move(filename);
	createdWithZTestMaterial = ztest;
	*err = nullptr;
	return true;
}

void MapOverlay::ClearMeshes()
{
	meshes.clear();
	lastLoadedFile.clear();
}

void MapOverlay::OnMeshRenderSignal(MeshRendererDelegate& mr)
{
	if (std::any_of(meshes.begin(), meshes.end(), [](auto& mesh) { return !mesh.Valid(); }))
		ClearMeshes();

	for (const auto& mesh : meshes)
	{
		mr.DrawMesh(mesh,
		            [](const CallbackInfoIn& infoIn, CallbackInfoOut& infoOut)
		            {
			            PositionMatrix(spt_map_overlay.offset, infoOut.mat);
			            RenderCallbackZFightFix(infoIn, infoOut);
		            });
	}
}

void MapOverlay::ImGuiCallback()
{
	static SptImGui::AutocompletePersistData acPersist;
	static bool ztest = true;

	static double errMsgStartTime = -666;
	static const char* errMsg;

	ConCommand& cmd = spt_draw_map_overlay_command;
	const char* cmdName = WrangleLegacyCommandName(cmd.GetName(), true, nullptr);

	if (ImGui::Button("Clear"))
		spt_map_overlay.ClearMeshes();
	if (ImGui::BeginItemTooltip())
	{
		ImGui::Text("%s 0", cmdName);
		ImGui::EndTooltip();
	}

	ImGui::SameLine();
	if (ImGui::Button("Draw map"))
	{
		if (!spt_map_overlay.LoadMapFile(acPersist.textInput, ztest, &errMsg))
			errMsgStartTime = ImGui::GetTime();
	}
	if (ImGui::BeginItemTooltip())
	{
		const Vector& v = spt_map_overlay.offset;
		ImGui::Text("%s \"%s\" %g %g %g %d", cmdName, acPersist.textInput, v.x, v.y, v.z, ztest);
		ImGui::EndTooltip();
	}

	if (errMsg && ImGui::GetTime() - errMsgStartTime < 2)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, SPT_IMGUI_WARN_COLOR_YELLOW);
		ImGui::SetTooltip("%s", errMsg);
		ImGui::PopStyleColor();
	}

	ImGui::SameLine();
	SptImGui::HelpMarker("%s", cmd.GetHelpText());

	SptImGui::TextInputAutocomplete("enter map name",
	                                "##map_overlay_autocomplete",
	                                acPersist,
	                                AUTOCOMPLETION_FUNCTION(spt_draw_map_overlay),
	                                spt_draw_map_overlay_command.GetName());

	Vector& v = spt_map_overlay.offset;
	ImGui::DragFloat3("offset", v.Base(), 1.f, 0.f, 0.f, "%g", ImGuiSliderFlags_NoRoundToFormat);

	ImGui::Checkbox("ztest", &ztest);
	if (ImGui::BeginItemTooltip())
	{
		ImGui::TextUnformatted("If disabled, will draw on top of everything else.");
		ImGui::EndTooltip();
	}
}

bool MapOverlay::ShouldLoadFeature()
{
	return true;
}

void MapOverlay::InitHooks() {}

void MapOverlay::LoadFeature()
{
	if (!spt_meshRenderer.signal.Works)
		return;
	InitCommand(spt_draw_map_overlay);
	spt_meshRenderer.signal.Connect(this, &MapOverlay::OnMeshRenderSignal);
	SptImGuiGroup::Draw_MapOverlay.RegisterUserCallback(ImGuiCallback);
}

void MapOverlay::UnloadFeature() {}

#endif
