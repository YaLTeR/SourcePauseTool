#include "stdafx.hpp"

#include "renderer\mesh_renderer.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include <vector>

// Draw the brushes from another map
class DrawLine : public FeatureWrapper<DrawLine>
{
public:
	struct LineInfo
	{
		Vector start, end;
		color32 color;
	};

	void AddLine(LineInfo line);
	void ClearLines();

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	void OnMeshRenderSignal(MeshRendererDelegate& mr);

	bool should_recompute_meshes = false;
	std::vector<LineInfo> lines;
	std::vector<StaticMesh> meshes;
};

static DrawLine feat_drawline;

CON_COMMAND(spt_drawline,
            "Draws line between two 3D Points.\n"
            "spt_drawline <x> <y> <z> <x> <y> <z> [r] [g] [b]\n"
            "\"spt_drawline 0\" to clear all lines")
{
	int argc = args.ArgC();
	if (argc == 2 && strcmp(args.Arg(1), "0") == 0)
	{
		feat_drawline.ClearLines();
		return;
	}

	if (argc != 7 && argc != 10)
	{
		return Msg(
		    "spt_drawline <x> <y> <z> <x> <y> <z> [r] [g] [b]\n"
		    "\"spt_drawline 0\" to clear all lines\n");
	}

	float x0 = atof(args[1]);
	float y0 = atof(args[2]);
	float z0 = atof(args[3]);
	float x1 = atof(args[4]);
	float y1 = atof(args[5]);
	float z1 = atof(args[6]);

	uint8_t r = 255;
	uint8_t g = 255;
	uint8_t b = 255;

	if (args.ArgC() == 10)
	{
		r = (uint8_t)atoi(args[7]);
		g = (uint8_t)atoi(args[8]);
		b = (uint8_t)atoi(args[9]);
	}

	feat_drawline.AddLine({
	    {x0, y0, z0},
	    {x1, y1, z1},
	    {r, g, b, 255},
	});
}

void DrawLine::AddLine(LineInfo line)
{
	should_recompute_meshes = true;
	lines.push_back(line);
}

void DrawLine::ClearLines()
{
	meshes.clear();
	lines.clear();
}

void DrawLine::OnMeshRenderSignal(MeshRendererDelegate& mr)
{
	if (std::any_of(meshes.begin(), meshes.end(), [](auto& mesh) { return !mesh.Valid(); }))
		should_recompute_meshes = true;

	if (should_recompute_meshes)
	{
		should_recompute_meshes = false;
		meshes.clear();

		auto linesIter = lines.begin();
		const auto linesEnd = lines.end();
		while (linesIter != linesEnd)
		{
			meshes.push_back(spt_meshBuilder.CreateStaticMesh(
			    [&](MeshBuilderDelegate& mb)
			    {
				    for (; linesIter != linesEnd; linesIter++)
					    if (!mb.AddLine((*linesIter).start,
					                    (*linesIter).end,
					                    LineColor((*linesIter).color, false)))
						    break;
			    }));
		}
	}

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

bool DrawLine::ShouldLoadFeature()
{
	return true;
}

void DrawLine::InitHooks() {}

void DrawLine::LoadFeature()
{
	if (spt_meshRenderer.signal.Works)
	{
		InitCommand(spt_drawline);
		spt_meshRenderer.signal.Connect(this, &DrawLine::OnMeshRenderSignal);
	}
}

void DrawLine::UnloadFeature() {}

#endif
