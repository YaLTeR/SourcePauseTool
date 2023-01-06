#include "stdafx.hpp"

#include "..\mesh_defs_private.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

StaticMesh* StaticMesh::first = nullptr;

StaticMesh::StaticMesh() : meshPtr(nullptr), prev(nullptr), next(nullptr) {}

StaticMesh::StaticMesh(const StaticMesh& other) : meshPtr(other.meshPtr), prev(nullptr), next(nullptr)
{
	AttachToFront();
}

StaticMesh::StaticMesh(StaticMesh&& other) : meshPtr(std::move(other.meshPtr)), prev(other.prev), next(other.next)
{
	// keep the linked list chain valid
	if (other.prev)
		other.prev->next = this;
	if (other.next)
		other.next->prev = this;
	if (first == &other)
		first = this;
	other.prev = other.next = nullptr;
}

StaticMesh::StaticMesh(MeshUnit* mesh) : meshPtr(std::shared_ptr<MeshUnit>(mesh)), prev(nullptr), next(nullptr)
{
	AttachToFront();
}

StaticMesh& StaticMesh::operator=(const StaticMesh& r)
{
	if (this != &r)
	{
		Destroy();
		meshPtr = r.meshPtr;
		AttachToFront();
	}
	return *this;
}

bool StaticMesh::Valid() const
{
	return !!meshPtr;
}

StaticMesh::~StaticMesh()
{
	Destroy();
}

void StaticMesh::AttachToFront()
{
	// assume we're not attached
	prev = nullptr;
	if (first)
	{
		next = first;
		first->prev = this;
	}
	else
	{
		prev = next = nullptr;
	}
	first = this;
}

void StaticMesh::Destroy()
{
	meshPtr.reset();
	if (this == first)
		first = next;
	if (prev)
		prev->next = next;
	if (next)
		next->prev = prev;
	prev = next = nullptr;
}

void StaticMesh::DestroyAll()
{
	while (first)
		first->Destroy();
}

#endif
