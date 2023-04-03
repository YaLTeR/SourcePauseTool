//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//


// This file is a modified version of the original, function declarations and some other things have been removed.

#pragma once

#define MAX_QPATH 96 // max length of a game pathname, from qlimits.h

#include "bitvec.h"
#include "bspfile.h"
#include "gametrace.h"

#include "dispcoll_common.h"

class CDispCollTree;
class CCollisionBSPData;
struct cbrush_t;

#define MAX_CHECK_COUNT_DEPTH 2

typedef uint32 TraceCounter_t;
typedef CUtlVector<TraceCounter_t> CTraceCounterVec;

struct TraceInfo_t
{
	TraceInfo_t()
	{
		memset( this, 0, offsetof( TraceInfo_t, m_BrushCounters ) );
		m_nCheckDepth = -1;
	}

	Vector m_start;
	Vector m_end;
	Vector m_mins;
	Vector m_maxs;
	Vector m_extents;
	Vector m_delta;
	Vector m_invDelta;

	trace_t m_trace;
	trace_t m_stabTrace;

	int m_contents;
	bool m_ispoint;
	bool m_isswept;

	// BSP Data
	CCollisionBSPData *m_pBSPData;

	// Displacement Data
	Vector m_DispStabDir;		// the direction to stab in
	int m_bDispHit;				// hit displacement surface last

	bool m_bCheckPrimary;
	int m_nCheckDepth;
	TraceCounter_t m_Count[MAX_CHECK_COUNT_DEPTH];

	CTraceCounterVec m_BrushCounters[MAX_CHECK_COUNT_DEPTH];
	CTraceCounterVec m_DispCounters[MAX_CHECK_COUNT_DEPTH];

	TraceCounter_t GetCount()				{ return m_Count[m_nCheckDepth]; }
	TraceCounter_t *GetBrushCounters()		{ return m_BrushCounters[m_nCheckDepth].Base(); }
	TraceCounter_t *GetDispCounters()		{ return m_DispCounters[m_nCheckDepth].Base(); }
};

#define		NEVER_UPDATED		-99999

//=============================================================================
//
// Local CModel Structures (cmodel.cpp and cmodel_disp.cpp)
//

struct cbrushside_t
{
	cplane_t	*plane;
	unsigned short surfaceIndex;
	unsigned short bBevel;							// is the side a bevel plane?
};

#define NUMSIDES_BOXBRUSH	0xFFFF

struct cbrush_t
{
	int				contents;
	unsigned short	numsides;
	unsigned short	firstbrushside;

	inline int GetBox() const { return firstbrushside; }
	inline void SetBox( int boxID )
	{
		numsides = NUMSIDES_BOXBRUSH;
		firstbrushside = boxID;
	}
	inline bool IsBox() const { return numsides == NUMSIDES_BOXBRUSH ? true : false; }
};

// 48-bytes, aligned to 16-byte boundary
// this is a brush that is an AABB.  It's encoded this way instead of with 6 brushsides
struct cboxbrush_t
{
	VectorAligned	mins;
	VectorAligned	maxs;

	unsigned short	surfaceIndex[6];
	unsigned short	pad2[2];
};

struct cleaf_t
{
	int			    contents;
	short			cluster;
	short			area : 9;
	short			flags : 7;
	unsigned short	firstleafbrush;
	unsigned short	numleafbrushes;
	unsigned short	dispListStart;
	unsigned short	dispCount;
};

struct carea_t
{
	int		numareaportals;
	int		firstareaportal;
	int		floodnum;							// if two areas have equal floodnums, they are connected
	int		floodvalid;
};


struct cnode_t
{
	cplane_t	*plane;
	int			children[2];		// negative numbers are leafs
};

//-----------------------------------------------------------------------------
// A dumpable version of RangeValidatedArray
//-----------------------------------------------------------------------------
#include "tier0/memdbgon.h"
template <class T> 
class CDiscardableArray 
{
public:
	CDiscardableArray()
	{
		m_nCount = 0;
		m_nOffset = -1;
		memset( m_pFilename, 0, sizeof( m_pFilename ) );
	}

	~CDiscardableArray()
	{
	}

	void Init( char* pFilename, int nOffset, int nCount, void *pData = NULL )
	{
		if ( m_buf.TellPut() )
		{
			Discard();
		}

		m_nCount = nCount;
		Q_strcpy( m_pFilename, pFilename );
		m_nOffset = nOffset;

		// can preload as required
		if ( pData )
		{
			m_buf.Put( pData, nCount );
		}
	}

	void Discard()
	{
		m_buf.Purge();
	}

protected:
	int			m_nCount;
	CUtlBuffer	m_buf;
	char		m_pFilename[256];
	int			m_nOffset;
};
#include "tier0/memdbgoff.h"

const int SURFACE_INDEX_INVALID = 0xFFFF;

//=============================================================================
//
// Collision BSP Data Class
//
class CCollisionBSPData
{
public:
	// This is sort of a hack, but it was a little too painful to do this any other way
	// The goal of this dude is to allow us to override the tree with some
	// other tree (or a subtree)
	cnode_t*					map_rootnode;

	char						map_name[MAX_QPATH];
	static csurface_t			nullsurface;

	int							numbrushsides;
	cbrushside_t*				map_brushsides;
	int							numboxbrushes;
	cboxbrush_t*				map_boxbrushes;
	int							numplanes;
	cplane_t*					map_planes;
	int							numnodes;
	cnode_t*					map_nodes;
	int							numleafs;				// allow leaf funcs to be called without a map
	cleaf_t*					map_leafs;
	int							emptyleaf, solidleaf;
	int							numleafbrushes;
	unsigned short*				map_leafbrushes;
	int							numcmodels;
	cmodel_t*					map_cmodels;
	int							numbrushes;
	cbrush_t*					map_brushes;
	int							numdisplist;
	unsigned short*				map_dispList;
	
	// this points to the whole block of memory for vis data, but it is used to
	// reference the header at the top of the block.
	int							numvisibility;
	dvis_t						*map_vis;

	int							numentitychars;
	CDiscardableArray<char>		map_entitystring;

	int							numareas;
	carea_t*					map_areas;
	int							numareaportals;
	dareaportal_t*				map_areaportals;
	int							numclusters;
	char						*map_nullname;
	int							numtextures;
	char						*map_texturenames;
	csurface_t*					map_surfaces;
	int							floodvalid;
	int							numportalopen;
	bool*						portalopen;

	csurface_t					*GetSurfaceAtIndex( unsigned short surfaceIndex );
};
