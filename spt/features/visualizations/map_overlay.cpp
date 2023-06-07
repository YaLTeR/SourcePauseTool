#include "stdafx.hpp"

#include "renderer\mesh_renderer.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include "spt\utils\convar.hpp"
#include "spt\utils\game_detection.hpp"
#include "spt\utils\file.hpp"

#include "bspfile.h"

#include <stack>

#define SC_BOX_BRUSH ShapeColor(C_OUTLINE(0, 255, 255, 20))
#define SC_COMPLEX_BRUSH ShapeColor(C_OUTLINE(255, 0, 255, 20))

// Draw the brushes from another map
class MapOverlay : public FeatureWrapper<MapOverlay>
{
public:
	void LoadMapFile(std::string filename, Vector offsets, bool ztest);
	void ClearMeshes();

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	void OnMeshRenderSignal(MeshRendererDelegate& mr);

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
	}

	Vector offsets(0, 0, 0);
	if (argc == 5 || argc == 6)
	{
		offsets.x = atof(args.Arg(2));
		offsets.y = atof(args.Arg(3));
		offsets.z = atof(args.Arg(4));
	}

	bool ztest = true;
	if (argc == 3 || argc == 6)
		ztest = !!atoi(args.Arg(argc - 1));

	spt_map_overlay.LoadMapFile(args.Arg(1), offsets, ztest);
}

void MapOverlay::LoadMapFile(std::string filename, Vector offsets, bool ztest)
{
#define READ_BYTES(f, buffer, bytes) \
	{ \
		f.read((buffer), (bytes)); \
		if (f.gcount() != (bytes)) \
		{ \
			Msg("Unexpected EOF.\n"); \
			return; \
		} \
	} \
	while (0)

#define SEEK_TO_BYTE(f, offset) \
	{ \
		f.seekg((offset), std::ios::beg); \
		if (!f) \
		{ \
			Msg("Unexpected EOF.\n"); \
			return; \
		} \
	} \
	while (0)

	std::ifstream mapFile(GetGameDir() + "\\maps\\" + filename + ".bsp", std::ios::binary);
	if (!mapFile.is_open())
	{
		Msg("Cannot open file.\n");
		return;
	}

	dheader_t header;
	READ_BYTES(mapFile, (char*)&header, sizeof(dheader_t));
	if (header.ident != IDBSPHEADER)
	{
		Msg("Not a bsp file.\n");
		return;
	}

	if (header.version != 20 && header.version != 19
	    && !(utils::DoesGameLookLikeDMoMM() && (header.version >> 16) != 20))
	{
		Msg("Unsupported bsp version.\n");
		return;
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

				    std::vector<VPlane> vplanes;
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

				    if (offsets != Vector(0))
				    {
					    for (int i = 0; i < poly->iVertexCount; i++)
					    {
						    poly->pVertices[i] += offsets;
					    }
				    }

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
}

void MapOverlay::ClearMeshes()
{
	meshes.clear();
}

void MapOverlay::OnMeshRenderSignal(MeshRendererDelegate& mr)
{
	if (std::any_of(meshes.begin(), meshes.end(), [](auto& mesh) { return !mesh.Valid(); }))
		meshes.clear();

	if (!meshes.empty())
	{
		for (const auto& mesh : meshes)
		{
			mr.DrawMesh(mesh,
			            [](const CallbackInfoIn& infoIn, CallbackInfoOut& infoOut)
			            { RenderCallbackZFightFix(infoIn, infoOut); });
		}
	}
}

bool MapOverlay::ShouldLoadFeature()
{
	return true;
}

void MapOverlay::InitHooks() {}

void MapOverlay::LoadFeature()
{
	if (spt_meshRenderer.signal.Works)
	{
		InitCommand(spt_draw_map_overlay);
		spt_meshRenderer.signal.Connect(this, &MapOverlay::OnMeshRenderSignal);
	}
}

void MapOverlay::UnloadFeature() {}

#endif
