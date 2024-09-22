//======= Copyright � 1996-2006, Valve Corporation, All rights reserved. ======
//
// Purpose:
//
//=============================================================================

#ifndef VALVEMAYA_H
#define VALVEMAYA_H

#if defined( _WIN32 )
#pragma once
#endif

// Maya Includes

#include <maya/MAngle.h>
#include <maya/MDagModifier.h>
#include <maya/MEulerRotation.h>
#include <maya/MFloatVector.h>
#include <maya/MGlobal.h>
#include <maya/MIOStream.h>
#include <maya/MMatrix.h>
#include <maya/MObject.h>
#include <maya/MQuaternion.h>
#include <maya/MSyntax.h>
#include <maya/MString.h>
#include <maya/MDagPath.h>
#include <maya/MVector.h>

// Valve Includes

#include "tier1/stringpool.h"
#include "tier1/utlstring.h"
#include "tier1/utlstringmap.h"
#include "tier1/utlvector.h"
#include "tier1/interface.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IMayaVGui;


//-----------------------------------------------------------------------------
// Maya-specific library singletons
//-----------------------------------------------------------------------------
extern IMayaVGui *g_pMayaVGui;


//-----------------------------------------------------------------------------
//
// Purpose: Group a bunch of functions into the Valve Maya Namespace
//
//-----------------------------------------------------------------------------


namespace ValveMaya
{
	//-----------------------------------------------------------------------------
	// Forward declarations
	//-----------------------------------------------------------------------------
	class CUndo;

	//-----------------------------------------------------------------------------
	// Statics
	//-----------------------------------------------------------------------------
	extern const MQuaternion v2mQuat;	// Valve Engine -> Maya Quaternion
	extern const MQuaternion m2vQuat;	// Maya -> Valve Engine Quaternion
	extern const MMatrix v2mMat;		// Valve Engine -> Maya Matrix
	extern const MMatrix m2vMat;		// Maya -> Valve Engine Matrix

	extern const MQuaternion vc2mcQuat;	// Valve Engine Camera -> Maya Camera Quaternion
	extern const MQuaternion mc2vcQuat;	// Maya Camera -> Valve Camera Engine Quaternion
	extern const MMatrix vc2mcMat;		// Valve Engine Camera -> Maya Camera Quaternion
	extern const MMatrix mc2vcMat;		// Maya Camera -> Valve Camera Engine Quaternion

	inline MQuaternion ValveToMaya( const MQuaternion &q )				{ return q * v2mQuat; }
	inline MQuaternion ValveCameraToMayaCamera( const MQuaternion &q )	{ return vc2mcQuat * q * v2mQuat; }
	inline MQuaternion MayaToValve( const MQuaternion &q )				{ return q * m2vQuat; }
	inline MQuaternion MayaCameraToValveCamera( const MQuaternion &q )	{ return mc2vcQuat * q * m2vQuat; }

	inline MVector ValveToMaya( const MVector &p )						{ return p.rotateBy( v2mQuat ); }
	inline MVector ValveCameraToMayaCamera( const MVector &p )			{ return p.rotateBy( v2mQuat ); }
	inline MVector MayaToValve( const MVector &p )						{ return p.rotateBy( m2vQuat ); }
	inline MVector MayaCameraToValveCamera( const MVector &p )			{ return p.rotateBy( m2vQuat ); }

//-----------------------------------------------------------------------------
// Connect, disconnect
//-----------------------------------------------------------------------------
bool ConnectLibraries( CreateInterfaceFn factory );
void DisconnectLibraries();

MStatus CreateDagNode(
	const char *const i_nodeType,
	const char *const i_transformName = NULL,
	const MObject &i_parentObj = MObject::kNullObj,
	MObject *o_pTransformObj = NULL,
	MObject *o_pShapeObj = NULL,
	MDagModifier *i_mDagModifier = NULL);

inline MStatus CreateDagNode(
	CUndo &undo,
	const char *const i_nodeType,
	const char *const i_transformName = NULL,
	const MObject &i_parentObj = MObject::kNullObj,
	MObject *o_pTransformObj = NULL,
	MObject *o_pShapeObj = NULL )
{
	return CreateDagNode( i_nodeType, i_transformName, i_parentObj, o_pTransformObj, o_pShapeObj );
}

bool IsNodeVisible(	const MDagPath &mDagPath, bool bTemplateAsInvisible = true );
bool IsPathVisible( MDagPath mDagPath, bool bTemplateAsInvisible = true );

class CMSyntaxHelp 
{
public:
	CMSyntaxHelp()
	: m_groupedHelp( false )	// Make case sensitive
	, m_helpCount( 0 )
	, m_shortNameLength( 0 )
	{}

	void Clear();

	MStatus AddFlag(
		MSyntax &i_mSyntax,
		const char *i_shortName,
		const char *i_longName,
		const char *i_group,
		const char *i_help,
		const MSyntax::MArgType i_argType1 = MSyntax::kNoArg,
		const MSyntax::MArgType i_argType2 = MSyntax::kNoArg,
		const MSyntax::MArgType i_argType3 = MSyntax::kNoArg,
		const MSyntax::MArgType i_argType4 = MSyntax::kNoArg,
		const MSyntax::MArgType i_argType5 = MSyntax::kNoArg,
		const MSyntax::MArgType i_argType6 = MSyntax::kNoArg);

	void PrintHelp(
		const char *const i_cmdName,
		const char *const i_cmdDesc,
		int i_lineLength = 0U);

	void PrintHelp(
		const MString &i_cmdName,
		const MString &i_cmdDesc,
		int i_lineLength = 0U)
	{
		PrintHelp( i_cmdName.asChar(), i_cmdDesc.asChar(), i_lineLength );
	}

protected:
public:
protected:
	CCountedStringPool m_stringPool;

	struct HelpData_t
	{
		const char *m_shortName;
		const char *m_longName;
		const char *m_help;
		CUtlVector<MSyntax::MArgType> m_argTypes;
	};

	CUtlVector<const char *> m_groupOrder;
	CUtlStringMap<CUtlVector<HelpData_t> > m_groupedHelp;
	int m_helpCount;
	int m_shortNameLength;
};

MString RemoveNameSpace( const MString &mString );

MString &BackslashToSlash( MString &mString );

template < class T_t > bool IsDefault( T_t &, const MPlug & );

bool IsDefault(	const MPlug &aPlug );

uint NextAvailable( MPlug &mPlug );

MObject AddColorSetToMesh(
	const MString &colorSetName,
	const MDagPath &mDagPath,
	MDagModifier &mDagModifier );

MString GetMaterialPath(
	const MObject &shadingGroupObj,
	MObject *pSurfaceShaderObj,
	MObject *pFileObj,
	MObject *pPlace2dTextureObj,
	MObject *pVmtObj,
	bool *pbTransparent,
	MString *pDebugWhy );

// Returns the first node that is connected to the specified plug as input or output
MObject FindConnectedNode( const MPlug &mPlug );

// Returns the plug connected to the specified plug as an input, a NULL plug if no plug is connected
MPlug FindInputPlug( const MPlug &dstPlug );

MPlug FindInputPlug( const MObject &dstNodeObj, const MString &dstPlugName );

MObject FindInputNode( const MPlug &dstPlug );

MObject FindInputNode( const MObject &dstNodeObj, const MString &dstPlugName );

MObject FindInputAttr( const MPlug &dstPlug );

MObject FindInputAttr( const MObject &dstNodeObj, const MString &dstPlugName );

// Returns the first found node MObject of the specified type in the history of the specified node
MObject FindInputNodeOfType( const MObject &dstNodeObj, const MString &typeName, const MString &dstPlugName );

MObject FindInputNodeOfType( const MObject &dstNodeObj, const MString &typeName );

// Creates a deformer of the specified type and deforms the specified shape with an optional component
MObject DeformShape( ValveMaya::CUndo &undo, const MString &deformerType, const MDagPath &inOrigDagPath, MObject &origCompObj = MObject::kNullObj );

}  // end namespace ValveMaya


// Make an alias for the ValveMaya namespace

namespace vm = ValveMaya;


//-----------------------------------------------------------------------------
// A simple stream class for printing information to the Maya script editor
//-----------------------------------------------------------------------------
class CMayaStream
{
public:
	enum StreamType { kInfo, kWarning, kError };

	CMayaStream( const StreamType i_streamType = kInfo )
	: m_streamType( i_streamType )
	{}

	template < class T_t >
	CMayaStream &operator<<( const T_t &v );

	// This is a hack so CMayaStream << std::endl works as expected
	CMayaStream &operator<<( std::ostream &(*StdEndl_t)( std::ostream & ) )
	{
		return flush();
	}

	CMayaStream &flush() { return outputString(); }

protected:
	CMayaStream &outputString()
	{
		// Always ensure it's terminated with a newline
		if ( *( m_string.asChar() + m_string.length() - 1 ) != '\n' )
		{
			m_string += "\n";
		}

		const char *pBegin = m_string.asChar();
		const char *const pEnd = pBegin + m_string.length();
		const char *pCurr = pBegin;

		while ( *pCurr && pCurr < pEnd )
		{
			if ( *pCurr == '\n' )
			{
				switch ( m_streamType )
				{
				case kWarning:
					MGlobal::displayWarning( MString( pBegin, pCurr - pBegin ) );
					break;
				case kError:
					MGlobal::displayError( MString( pBegin, pCurr - pBegin ) );
					break;
				default:
					MGlobal::displayInfo( MString( pBegin, pCurr - pBegin ) );
					break;
				}

				++pCurr;
				pBegin = pCurr;
			}
			else
			{
				++pCurr;
			}
		}

		m_string.clear();

		return *this;
	}

	CMayaStream &checkForFlush()
	{
		const char *pCurr = m_string.asChar();
		const char *pEnd = pCurr + m_string.length();
		while ( *pCurr && pCurr != pEnd )
		{
			if ( *pCurr == '\n' )
			{
				return outputString();
			}

			++pCurr;
		}

		return *this;
	}

	StreamType m_streamType;
	MString m_string;
};


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class CMayaWarnStream : public CMayaStream
{
public:
	CMayaWarnStream()
	: CMayaStream( CMayaStream::kWarning )
	{}

};


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class CMayaErrStream : public CMayaStream
{
public:
	CMayaErrStream()
	: CMayaStream( CMayaStream::kError )
	{}
};


//-----------------------------------------------------------------------------
// Specialization for std::string
//-----------------------------------------------------------------------------
template <>
inline CMayaStream &CMayaStream::operator<<( const std::string &v )
{
	*this << v.c_str();
	return *this;
}


//-----------------------------------------------------------------------------
// Specialization for char
//-----------------------------------------------------------------------------
template <>
inline CMayaStream &CMayaStream::operator<<( const char &v )
{
	m_string += MString( &v, 1 );
	return *this;
}


//-----------------------------------------------------------------------------
// Specialization for MVector
//-----------------------------------------------------------------------------
template <>
inline CMayaStream &CMayaStream::operator<<( const MVector &v )
{
	m_string += v.x;
	m_string += " ";
	m_string += v.y;
	m_string += " ";
	m_string += v.z;
	return *this;
}


//-----------------------------------------------------------------------------
// Specialization for MFloatVector
//-----------------------------------------------------------------------------
template <>
inline CMayaStream &CMayaStream::operator<<( const MFloatVector &v )
{
	m_string += v.x;
	m_string += " ";
	m_string += v.y;
	m_string += " ";
	m_string += v.z;
	return *this;
}


//-----------------------------------------------------------------------------
// Specialization for MQuaternion
//-----------------------------------------------------------------------------
template <>
inline CMayaStream &CMayaStream::operator<<( const MQuaternion &q )
{
	m_string += q.x;
	m_string += " ";
	m_string += q.y;
	m_string += " ";
	m_string += q.z;
	m_string += " ";
	m_string += q.w;
	return *this;
}


//-----------------------------------------------------------------------------
// Specialization for MEulerRotation
//-----------------------------------------------------------------------------
template <>
inline CMayaStream &CMayaStream::operator<<( const MEulerRotation &e )
{
	m_string += MAngle( e.x, MAngle::kRadians ).asDegrees();
	m_string += " ";
	m_string += MAngle( e.y, MAngle::kRadians ).asDegrees();
	m_string += " ";
	m_string += MAngle( e.z, MAngle::kRadians ).asDegrees();
	switch ( e.order )
	{
	case MEulerRotation::kXYZ:
		m_string += " (XYZ)";
		break;
	case MEulerRotation::kXZY:
		m_string += " (XZY)";
		break;
	case MEulerRotation::kYXZ:
		m_string += " (YXZ)";
		break;
	case MEulerRotation::kYZX:
		m_string += " (YZX)";
		break;
	case MEulerRotation::kZXY:
		m_string += " (ZXY)";
		break;
	case MEulerRotation::kZYX:
		m_string += " (ZYX)";
		break;
	default:
		m_string += " (Unknown)";
		break;
	}
	return *this;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
template < class T_t >
inline CMayaStream &CMayaStream::operator<<( const T_t &v )
{
	m_string += v;
	return checkForFlush();
}


//-----------------------------------------------------------------------------
//
// minfo, mwarn & merr are ostreams which can be used to send stuff to
// the Maya history window
//
//-----------------------------------------------------------------------------

extern CMayaStream minfo;
extern CMayaWarnStream mwarn;
extern CMayaErrStream merr;


#endif // VALVEMAYA_H
