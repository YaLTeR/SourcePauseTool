#pragma once

#include "..\mesh_defs.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include "mathlib\vector.h"
#include "materialsystem\imaterial.h"

#pragma warning(push)
#pragma warning(disable : 5054)
#include "materialsystem\imesh.h"
#pragma warning(pop)

#define VPROF_LEVEL 1
#ifndef SSDK2007
#define RAD_TELEMETRY_DISABLED
#endif
#include "vprof.h"

#define VPROF_BUDGETGROUP_MESH_RENDERER _T("Mesh_Renderer")

#include <stack>

using VertIndex = unsigned short;
using DynamicMeshToken = DynamicMesh;

template<class T>
using VectorStack = std::stack<T, std::vector<T>>;

#endif
