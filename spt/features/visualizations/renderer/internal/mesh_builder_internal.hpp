#pragma once

#include "internal_defs.hpp"
#include "vector_slice.hpp"
#include "materials_manager.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include <forward_list>

/*
* For lack of a better place, the life cycle of meshes is described here. Grab a cookie and make some tea.
* 
* This file contains most of the wrappers I use of the game's IMesh* system, as well as the internal mesh builder.
* Like the mesh renderer, there are 3 mesh builders:
* - MeshBuilderPro (spt_meshBuilder)
* - MeshBuilderDelegate
* - MeshBuilderInternal (g_meshBuilderInternal)
* The MeshBuilderPro is stateless, and will give the user a stateless delegate when they want to create a mesh.
* The user then calls the various methods of the delegate which will edit the state of the internal builder.
* 
* At the end of the day, the mesh builder's purpose is to create IMesh* objects so that the renderer can call
* IMesh->Draw(). For static meshes, this is simple enough - the user can fill up the builder's buffers with data,
* and when they're done we can call IMatRenderContext::CreateStaticMesh(...). The IMesh* is then yeeted onto the
* heap and given to the renderer later. The life cycle for static meshes looks like this:
* 
* 1) The user calls MeshBuilderPro::CreateStaticMesh(...)
* 2) The MeshBuiderPro gives the user a MeshBuilderDelegate
* 3) The MeshBuilderDelegate edits the state of the MeshBuilderInternal, populating its current MeshVertData
* 4) The MeshBuilderPro asks the MeshBuilderInternal to create an IMesh* object from the MeshVertData
* 5) The MeshBuilderPro puts the IMesh* object into the heap (wrapped in a StaticMesh) and gives that to the user
* 6) The user can then destroy or give that StaticMesh to the MeshRendererFeature
* 
* Dynamic meshes are a little bit more complicated. Under the hood (in game code) there is only one dynamic mesh,
* and we simply edit its state. However, that means that only a single dynamic IMesh* object can exist at a time.
* This means that we must keep the instructions to build dynamic meshes for the entire frame, and recreate IMesh*
* objects just as they're about to be drawn. The life cycle for dynamic meshes looks like this:
* 
* 1) The user calls MeshBuilderPro::CreateDynamicMesh(...)
* 2) Same as above
* 3) Same as above
* 4) The MeshBuilderPro asks to the MeshBuilderInternal to save the current MeshVertData
* 5) The MeshBuilderPro gives the user a token for that data (only internal classes can use it)
* 6) The user can give that token to the renderer
* 7) The renderer asks the MeshBuilderInternal to create the IMesh* object when it's about to draw it (every time)
* 8) At the start of the next frame, the MeshBuilderInternal clears its MeshVertData arrays
* 
* As such, the user is never responsible for the destruction of dynamic meshes. There are a few optimizations,
* probably the most complicated of which being the fusing of dynamic meshes. When asking the MeshBuilderInternal
* for an IMesh*, the renderer provides not just 1 but a whole interval of meshes, then the builder will
* automatically fuse consecutive elements and iteratively construct a single IMesh* object at a time until it's
* done iterating over that interval.
*/

// a single vertex - may have position, color, uv coordinates, normal, etc
struct VertexData
{
	Vector pos;
	color32 col;

	VertexData() : pos(), col(){};
	VertexData(const Vector& pos, color32 color) : pos(pos), col(color) {}
};

enum class MeshPrimitiveType : unsigned char
{
	Lines,
	Triangles,
	Count
};

// we will set aside buffers for the simple component types, meshes with other types will be allocated separately
#define MAX_SIMPLE_COMPONENTS ((size_t)MeshPrimitiveType::Count * (size_t)MeshMaterialSimple::Count)
#define SIMPLE_COMPONENT_INDEX(type, matType) ((size_t)(type) * (size_t)MeshMaterialSimple::Count + (size_t)(matType))

// contains the instructions for creating an IMesh* object
struct MeshVertData
{
	VectorSlice<VertexData> verts;
	VectorSlice<VertIndex> indices;
	MeshPrimitiveType type;
	MaterialRef material;

	MeshVertData() = default;

	MeshVertData(MeshVertData&& other);

	MeshVertData(std::vector<VertexData>& vertDataVec,
	             std::vector<VertIndex>& vertIndexVec,
	             MeshPrimitiveType type,
	             MaterialRef material);

	MeshVertData& operator=(MeshVertData&& other);

	bool Empty() const;
};

// the lowest level wrapper of an IMesh*, does not handle its destruction
struct IMeshWrapper
{
	IMesh* iMesh;
	MaterialRef material;
};

/*
* Mesh units are collections of meshes. Each IMesh* object can only have one material and one primitive type, but
* as a user I want to say "this mesh should have lines AND triangles". Dynamic mesh units will have a list of
* MeshVertData which can be used to create IMeshWrappers. Static meshes will just have a list of IMeshWrappers.
* Both static and dynamic meshes also need a position metric which is used for sorting translucents.
*/

struct DynamicMeshUnit
{
	VectorSlice<MeshVertData> vDataSlice;
	const MeshPositionInfo posInfo;

	DynamicMeshUnit(VectorSlice<MeshVertData>& vDataSlice, const MeshPositionInfo& posInfo);
	DynamicMeshUnit(const DynamicMeshUnit&) = delete;
	DynamicMeshUnit(DynamicMeshUnit&& other);
};

struct StaticMeshUnit
{
	IMeshWrapper* meshesArr;
	const size_t nMeshes;
	const MeshPositionInfo posInfo;

	StaticMeshUnit(size_t nMeshes, const MeshPositionInfo& posInfo);
	~StaticMeshUnit();
};

/*
* A MeshComponent is a single element of a mesh unit for statics OR dynamics. The renderer creates a list of these
* and sorts so that consecutive elements of certain intervals are elligable for fusing (the spaceship operator is
* used for this). Then it gives these intervals of dynamic meshes to the builder to actually fuse them. The
* builder only needs the vertData in this struct, but intervals are used elsewhere in the renderer which is why
* the other two fields exist.
*/
struct MeshComponent
{
	struct MeshUnitWrapper* unitWrapper;
	MeshVertData* vertData; // null for statics
	IMeshWrapper iMeshWrapper;

	std::weak_ordering operator<=>(const MeshComponent& rhs) const;
};

/*
* These intervals represent a portion of a container usually in the context of mesh fusing. The renderer will
* figure out which components are elligable for fusing and will give the builder an interval of meshes. These
* intervals are [first, second).
*/
using CompContainer = std::vector<MeshComponent>;
using CompIntrvl = std::pair<CompContainer::iterator, CompContainer::iterator>;
using ConstCompIntrvl = std::pair<CompContainer::const_iterator, CompContainer::const_iterator>;

/*
* When a mesh is being created, its data is stored in curMeshVertData. Dynamic meshes then get moved to the shared
* list, but statics are put on the heap. Each MeshVertData is simply a slice of the shared vertex/index lists. The
* reason we need a list of shared arrays is because each mesh can have multiple components. As a mesh is created,
* it will request MeshVertData from curMeshVertData that has a specific material and primitive type by calling
* GetComponentInCurrentMesh(). If one is not found, a new one is created that has slices of the end of the Nth
* shared list (for both verts & indices).
* 
* For example, say we have 5 dynamic meshes and each one has [1, 3, 1, 2, 3] components respectively. The first
* mesh will only use a slice of the first shared list, the second will use a slice of the first 3, etc. The shared
* vertex lists will look like this:
* 
* sharedVertLists[0]: comp_slice[0,0]..comp_slice[1,0]..comp_slice[2,0]..comp_slice[3,0]..comp_slice[4,0]
* sharedVertLists[1]: comp_slice[1,1]..comp_slice[3,1]
* sharedVertLists[2]: comp_slice[1,2]
* 
* The shared index lists will look exactly the same, and the shared MeshVertData list will look like this:
* 
* sharedVertDataArrays: meshVertData[0]..meshVertData[1]....meshVertData[2]..meshVertData[3]....meshVertData[4]
*                       V                V                  V                V                  V
*                       comp_slice[0,0]..comp_slice[1,0:3]..comp_slice[2,0]..comp_slice[3,0:2]..comp_slice[4,0:3]
*/

struct MeshBuilderInternal
{
	struct
	{
		struct
		{
			std::array<std::vector<VertexData>, MAX_SIMPLE_COMPONENTS> verts;
			std::array<std::vector<VertIndex>, MAX_SIMPLE_COMPONENTS> indices;
		} simple;

		/*struct
		{
			std::forward_list<std::vector<VertexData>> verts;
			std::forward_list<std::vector<VertIndex>> indices;
		} complex;*/

		std::vector<MeshVertData> dynamicMeshData;

	} sharedLists;

	struct TmpMesh
	{
		// will always have at least MAX_SIMPLE_COMPONENTS
		std::vector<MeshVertData> components;
		size_t maxVerts, maxIndices;

		void Create(const MeshCreateFunc& createFunc, bool dynamic);
		MeshPositionInfo CalcPosInfo();
	} tmpMesh;

	VectorStack<DynamicMeshUnit> dynamicMeshUnits;

	inline MeshVertData& GetSimpleMeshComponent(MeshPrimitiveType type, MeshMaterialSimple material)
	{
		return tmpMesh.components[SIMPLE_COMPONENT_INDEX(type, material)];
	}

	struct Fuser
	{
		/*
		* During rendering (or when creating static meshes), we'll be given an interval via BeginIMeshCreation().
		* Consecutive elements are fused so long as they don't exceed the max vert/index count.
		*/

		// for keeping track of where we are in the given interval
		ConstCompIntrvl curIntrvl;
		// used for debug meshes
		ConstCompIntrvl lastFusedIntrvl;
		size_t maxVerts, maxIndices;
		bool dynamic;

		void BeginIMeshCreation(ConstCompIntrvl intrvl, bool dynamic);
		IMeshWrapper GetNextIMeshWrapper();

	private:
		IMeshWrapper CreateIMeshFromInterval(ConstCompIntrvl intrvl, size_t totalVerts, size_t totalIndices);

	} fuser;

	void FrameCleanup();
	const DynamicMeshUnit& GetDynamicMeshFromToken(DynamicMeshToken token) const;
};

inline MeshBuilderInternal g_meshBuilderInternal;

#endif
