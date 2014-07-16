//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef HL_MOVEDATA_H
#define HL_MOVEDATA_H
#ifdef _WIN32
#pragma once
#endif


#include "igamemovement.h"


// This class contains HL2-specific prediction data.
class CHLMoveData : public CMoveData
{
public:
	bool		m_bIsSprinting;
};

class CFuncLadder;
class CReservePlayerSpot;

#endif // HL_MOVEDATA_H
