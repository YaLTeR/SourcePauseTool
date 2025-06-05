#include "stdafx.hpp"

#include "mesh_utils.hpp"

#include "spt/flatbuffers_schemas/fb_forward_declare.hpp"
#include "spt/flatbuffers_schemas/gen/mesh_generated.h"
#include "spt/flatbuffers_schemas/fb_pack_defs.hpp"

#ifdef OE
#include "vmatrix.h"
#else
#include "mathlib/vmatrix.h"
#endif

#include <span>
#include <memory_resource>

namespace utils
{
	void MbCompactMesh::Clear()
	{
		points.clear();
		colors.clear();
		faceIndices.clear();
		lineIndices.clear();
		vertToIdxMap.clear();
		lastCachedVertIdx = 0;
	}

	bool MbCompactMesh::AddTriangle(const MbColoredVert& p1, const MbColoredVert& p2, const MbColoredVert& p3)
	{
		idx_type i1 = GetIdxOfVert(p1);
		idx_type i2 = GetIdxOfVert(p2);
		idx_type i3 = GetIdxOfVert(p3);

		if (i1 == MB_CM_INVALID_IDX || i2 == MB_CM_INVALID_IDX || i3 == MB_CM_INVALID_IDX) [[unlikely]]
			return false;

		faceIndices.push_back(i1);
		faceIndices.push_back(i2);
		faceIndices.push_back(i3);

		return true;
	}

	bool MbCompactMesh::AddLine(const MbColoredVert& p1, const MbColoredVert& p2)
	{
		idx_type i1 = GetIdxOfVert(p1);
		idx_type i2 = GetIdxOfVert(p2);

		if (i1 == MB_CM_INVALID_IDX || i2 == MB_CM_INVALID_IDX) [[unlikely]]
			return false;

		lineIndices.push_back(i1);
		lineIndices.push_back(i2);

		return true;
	}

	bool MbCompactMesh::AddMbCompactMesh(const MbCompactMesh& other, const matrix3x4_t& matForOther)
	{
		if (points.size() + other.points.size() > MB_CM_MAX_NUM_VERTS)
			return false;
		VMatrix mat{matForOther};
		for (idx_type fi : other.faceIndices)
			faceIndices.push_back(GetIdxOfVert({mat * other.points[fi], other.colors[fi]}));
		for (idx_type li : other.lineIndices)
			lineIndices.push_back(GetIdxOfVert({mat * other.points[li], other.colors[li]}));
		return true;
	}

	fb::fb_off MbCompactMesh::Serialize(flatbuffers::FlatBufferBuilder& fbb, bool writeColors) const
	{
		auto fbPointOff = fbb.CreateVectorOfNativeStructs(points, fb_pack::PackVector);
		auto fbcolorOff = writeColors ? fbb.CreateVectorOfNativeStructs(colors, fb_pack::PackColor32) : 0;
		auto fbFaceIdxOff = fbb.CreateVector(faceIndices);
		auto fbLineIdxOff = fbb.CreateVector(lineIndices);
		return fb::mesh::CreateMbCompactMesh(fbb, fbPointOff, fbcolorOff, fbFaceIdxOff, fbLineIdxOff).o;
	}

	void MbCompactMesh::Deserialize(const fb::mesh::MbCompactMesh& fbMesh,
	                                ser::StatusTracker& stat,
	                                const matrix3x4_t mat)
	{
		auto fbPoints = fbMesh.points();
		auto fbColors = fbMesh.colors();
		auto fbFaceIndices = fbMesh.face_indices();
		auto fbLineIndices = fbMesh.line_indices();

		if (fbPoints && fbColors && fbPoints->size() != fbColors->size())
		{
			stat.Err("compact mesh points/colors size mismatch");
			return;
		}

		if (fbFaceIndices)
		{
			if (fbFaceIndices->size() % 3 != 0)
			{
				stat.Err("invalid amount of face indices");
				return;
			}
			for (idx_type idx : *fbFaceIndices)
			{
				if (!fbPoints || idx >= fbPoints->size()) [[unlikely]]
				{
					stat.Err("compact mesh face index out of bounds");
					return;
				}
			}
		}

		if (fbLineIndices)
		{
			if (fbLineIndices->size() % 2 != 0)
			{
				stat.Err("invalid amount of line indices");
				return;
			}
			for (idx_type idx : *fbLineIndices)
			{
				if (!fbPoints || idx >= fbPoints->size()) [[unlikely]]
				{
					stat.Err("compact mesh line index out of bounds");
					return;
				}
			}
		}

		__assume(fbPoints);
		bool hasColors = fbColors && fbColors->size() == fbPoints->size();

		struct
		{
			decltype(fbFaceIndices) fbIdxs;
			decltype(faceIndices)& outIdxs;
		} idxs[2]{
		    {fbFaceIndices, faceIndices},
		    {fbLineIndices, lineIndices},
		};

		if (points.empty())
		{
			// copy data directly without adding it to the map

			if (fbPoints)
			{
				std::ranges::transform(*fbPoints,
				                       std::back_inserter(points),
				                       [&mat](const fb::math::Vector3* v)
				                       {
					                       Vector outV = fb_pack::UnpackVector(*v);
					                       utils::VectorTransform(mat, outV);
					                       return outV;
				                       });
			}
			if (hasColors)
			{
				std::ranges::transform(*fbColors,
				                       std::back_inserter(colors),
				                       [](const fb::math::Color32* c)
				                       { return fb_pack::UnpackColor32(*c); });
			}
			else
			{
				colors.assign(points.size(), MB_CM_DEFAULT_COLOR);
			}

			for (size_t i = 0; i < ARRAYSIZE(idxs); i++)
			{
				if (!idxs[i].fbIdxs)
					continue;
				idxs[i].outIdxs.assign(idxs[i].fbIdxs->begin(), idxs[i].fbIdxs->end());
			}
		}
		else
		{
			// add points one at time and fuse them with the existing cache

			for (size_t i = 0; i < ARRAYSIZE(idxs); i++)
			{
				if (!idxs[i].fbIdxs)
					continue;
				for (idx_type idx : *idxs[i].fbIdxs)
				{
					Vector v = fb_pack::UnpackVector(*fbPoints->Get(idx));
					utils::VectorTransform(mat, v);
					color32 c = hasColors ? fb_pack::UnpackColor32(*fbColors->Get(idx))
					                      : MB_CM_DEFAULT_COLOR;
					idxs[i].outIdxs.push_back(GetIdxOfVert({v, c}));
				}
			}
		}
	}

	auto MbCompactMesh::GetIdxOfVert(const MbColoredVert& v) -> idx_type
	{
		Assert(points.size() == colors.size());

		// make sure cache is up to date
		for (; lastCachedVertIdx < points.size(); lastCachedVertIdx++)
		{
			vertToIdxMap.emplace(MbColoredVert{points[lastCachedVertIdx], colors[lastCachedVertIdx]},
			                     lastCachedVertIdx);
		}

		auto [it, isNew] = vertToIdxMap.try_emplace(v, points.size());
		if (isNew)
		{
			if (points.size() >= MB_CM_MAX_NUM_VERTS)
			{
				vertToIdxMap.erase(it);
				return MB_CM_INVALID_IDX;
			}
			points.push_back(v.pos);
			colors.push_back(v.col);
		}
		return it->second;
	}

} // namespace utils
