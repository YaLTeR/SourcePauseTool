#pragma once

#include "..\mesh_defs.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include "mathlib\vector.h"
#include "materialsystem\imaterial.h"

#pragma warning(push)
#pragma warning(disable : 5054)
#include "materialsystem\imesh.h"
#pragma warning(pop)

#include "spt\utils\spt_vprof.hpp"
#include "spt\utils\mesh_utils.hpp"

#define VPROF_BUDGETGROUP_MESH_RENDERER _T("Mesh_Renderer")

#include <stack>

using VertIndex = utils::MbCompactMesh::idx_type;
using DynamicMeshToken = DynamicMesh;

template<class T>
using VectorStack = std::stack<T, std::vector<T>>;

#endif
