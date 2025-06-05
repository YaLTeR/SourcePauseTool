#pragma once

#include <vector>
#include <unordered_set>
#include <span>
#include <unordered_map>
#include <string_view>
#include <limits>

#include "math.hpp"
#include "serialize.hpp"
#include "spt/flatbuffers_schemas/fb_forward_declare.hpp"

#ifdef max
#undef max
#undef min
#endif

namespace utils
{

	struct MbColoredVert
	{
		Vector pos;
		color32 col;

		MbColoredVert() : pos(), col() {};
		MbColoredVert(const Vector& pos, color32 color) : pos{pos}, col{color} {}

		bool operator==(const MbColoredVert& other) const
		{
			return (pos == other.pos) && !(col != other.col);
		}
	};

	/*
	* When creating entity meshes, the game gives us a flattened list of Vectors (as triangles).
	* For serialization purposes, you can compress these back to points + indices. This is also
	* used to compress static meshes. This isn't super fast so try to use this infrequently.
	*/
	class MbCompactMesh
	{
	public:
		using idx_type = uint16_t;

	private:
		static constexpr idx_type MB_CM_INVALID_IDX = std::numeric_limits<idx_type>::max();

	public:
		static constexpr size_t MB_CM_MAX_NUM_VERTS = MB_CM_INVALID_IDX - 1;
		static constexpr color32 MB_CM_DEFAULT_COLOR{255, 255, 255, 255};

		void Clear();

		bool AddTriangle(const MbColoredVert& v1, const MbColoredVert& v2, const MbColoredVert& v3);
		bool AddLine(const MbColoredVert& v1, const MbColoredVert& p2);

		bool AddTriangle(const Vector& v1, const Vector& v2, const Vector& v3)
		{
			return AddTriangle({v1, MB_CM_DEFAULT_COLOR},
			                   {v2, MB_CM_DEFAULT_COLOR},
			                   {v3, MB_CM_DEFAULT_COLOR});
		}

		bool AddLine(const Vector& v1, const Vector& v2)
		{
			return AddLine({v1, MB_CM_DEFAULT_COLOR}, {v2, MB_CM_DEFAULT_COLOR});
		}

		bool AddMbCompactMesh(const MbCompactMesh& other, const matrix3x4_t& matForOther);

		bool AddMbCompactMesh(const MbCompactMesh& other)
		{
			matrix3x4_t mat;
			SetIdentityMatrix(mat);
			AddMbCompactMesh(other, mat);
		}

		// clang-format off
		bool IsEmpty() const { return points.empty(); }
		const auto& GetPoints() const { return points; }
		const auto& GetColors() const { return colors; }
		const auto& GetFaceIndices() const { return faceIndices; }
		const auto& GetLineIndices() const { return lineIndices; }
		// clang-format on

		// deserialization will append the contents to the mesh

		fb::fb_off Serialize(flatbuffers::FlatBufferBuilder& fbb, bool writeColors) const;

		void Deserialize(const fb::mesh::MbCompactMesh& fbMesh,
		                 ser::StatusTracker& stat,
		                 const matrix3x4_t mat = matrix3x4_identity);

	private:
		// returns INVALID_IDX if this is a new vertex and there's no more space
		idx_type GetIdxOfVert(const MbColoredVert& v);

		struct VertHasher
		{
			size_t operator()(const MbColoredVert& v) const
			{
				return std::hash<std::string_view>{}(std::string_view{(const char*)&v, sizeof v});
			}
		};

		std::vector<Vector> points;
		std::vector<color32> colors;
		std::vector<idx_type> faceIndices;
		std::vector<idx_type> lineIndices;

		std::unordered_map<MbColoredVert, idx_type, VertHasher> vertToIdxMap;
		uint32_t lastCachedVertIdx = 0;
	};
} // namespace utils
