//====== Copyright � 1996-2008, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#ifndef BASEMODEL_PANEL_H
#define BASEMODEL_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "matsys_controls/mdlpanel.h"

//-----------------------------------------------------------------------------
// Resource file data used in posing the model inside of the model panel.
//-----------------------------------------------------------------------------
struct BMPResAnimData_t
{
	const char	*m_pszName;
	const char	*m_pszSequence;
	const char	*m_pszActivity;
	KeyValues	*m_pPoseParameters;
	bool		m_bDefault;

	BMPResAnimData_t()
	{
		m_pszName = NULL;
		m_pszSequence = NULL;
		m_pszActivity = NULL;
		m_pPoseParameters = NULL;
		m_bDefault = false;
	}

	~BMPResAnimData_t()
	{
		if ( m_pszName && m_pszName[0] )
		{
			delete [] m_pszName;
			m_pszName = NULL;
		}

		if ( m_pszSequence && m_pszSequence[0] )
		{
			delete [] m_pszSequence;
			m_pszSequence = NULL;
		}

		if ( m_pszActivity && m_pszActivity[0] )
		{
			delete [] m_pszActivity;
			m_pszActivity = NULL;
		}

		if ( m_pPoseParameters )
		{
			m_pPoseParameters->deleteThis();
			m_pPoseParameters = NULL;
		}
	}
};

struct BMPResAttachData_t
{
	const char	*m_pszModelName;
	int			m_nSkin;

	BMPResAttachData_t()
	{
		m_pszModelName = NULL;
		m_nSkin = 0;
	}

	~BMPResAttachData_t()
	{
		if ( m_pszModelName && m_pszModelName[0] )
		{
			delete [] m_pszModelName;
			m_pszModelName = NULL;
		}
	}
};

struct BMPResData_t
{
	float		m_flFOV;

	const char	*m_pszModelName;
	const char	*m_pszModelName_HWM;
	const char	*m_pszVCD;
	QAngle		m_angModelPoseRot;
	Vector		m_vecOriginOffset;
	Vector		m_vecFramedOriginOffset;
	Vector2D	m_vecViewportOffset;
	int			m_nSkin;
	bool		m_bUseSpotlight;

	CUtlVector<BMPResAnimData_t>		m_aAnimations;
	CUtlVector<BMPResAttachData_t>		m_aAttachModels;

	BMPResData_t()
	{
		m_flFOV = 0.0f;

		m_pszModelName = NULL;
		m_pszModelName_HWM = NULL;
		m_pszVCD = NULL;
		m_angModelPoseRot.Init();
		m_vecOriginOffset.Init();
		m_vecFramedOriginOffset.Init();
		m_vecViewportOffset.Init();
		m_nSkin = 0;
		m_bUseSpotlight = false;
	}

	~BMPResData_t()
	{
		if ( m_pszModelName && m_pszModelName[0] )
		{
			delete [] m_pszModelName;
			m_pszModelName = NULL;
		}

		if ( m_pszModelName_HWM && m_pszModelName_HWM[0] )
		{
			delete [] m_pszModelName_HWM;
			m_pszModelName_HWM = NULL;
		}

		if ( m_pszVCD && m_pszVCD[0] )
		{
			delete [] m_pszVCD;
			m_pszVCD = NULL;
		}

		m_aAnimations.Purge();
		m_aAttachModels.Purge();	
	}
};

//-----------------------------------------------------------------------------
// Base Model Panel
//
//	...vgui::Panel					|--> vgui
//	   +->vgui::EditablePanel		|
//	      +->PotterWheelPanel		|--> matsys_controls
//           +->MDLPanel			|
//		        +->BaseModelPanel	|--> game_controls, client.dll
//
//-----------------------------------------------------------------------------
class CBaseModelPanel : public CMDLPanel
{
	DECLARE_CLASS_SIMPLE( CBaseModelPanel, CMDLPanel );

public:

	// Constructor, Destructor.
	CBaseModelPanel( vgui::Panel *pParent, const char *pName );
	virtual ~CBaseModelPanel();

	// Overridden mdlpanel.h
	virtual void SetMDL( MDLHandle_t handle );
	virtual void SetMDL( const char *pMDLName );

	// Overridden methods of vgui::Panel
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void PerformLayout();

	// Animaiton.
	int FindDefaultAnim( void );
	int FindAnimByName( const char *pszName );
	void SetModelAnim( int iAnim );

	// Manipulation.
	virtual void OnKeyCodePressed ( vgui::KeyCode code );
	virtual void OnKeyCodeReleased( vgui::KeyCode code );
	virtual void OnMousePressed ( vgui::MouseCode code );
	virtual void OnMouseReleased( vgui::MouseCode code );
	virtual void OnCursorMoved( int x, int y );
	virtual void OnMouseWheeled( int delta );

private:

	// Resource file data.
	void ParseModelResInfo( KeyValues *inResourceData );
	void ParseModelAnimInfo( KeyValues *inResourceData );
	void ParseModelAttachInfo( KeyValues *inResourceData );

	void SetupModelDefaults( void );
	void SetupModelAnimDefaults( void );

	void LookAtBounds( const Vector &vecBoundsMin, const Vector &vecBoundsMax );

	int FindSequenceFromActivity( CStudioHdr *pStudioHdr, const char *pszActivity );

private:

	BMPResData_t	m_BMPResData;			// Base model panel data set in the .res file.
	QAngle			m_angPlayer;
	Vector			m_vecPlayerPos;
	bool			m_bForcePos;
	bool			m_bMousePressed;
	bool			m_bAllowRotation;

	// VGUI script accessible variables.
	CPanelAnimationVar( bool, m_bStartFramed, "start_framed", "0" );
	CPanelAnimationVar( bool, m_bDisableManipulation, "disable_manipulation", "0" );
};

#endif // BASEMODEL_PANEL_H