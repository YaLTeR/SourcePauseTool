//========= Copyright � 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IMAGE_MOUSE_OVER_BUTTON_H
#define IMAGE_MOUSE_OVER_BUTTON_H

#include "vgui/isurface.h"
#include "mouseoverpanelbutton.h"

//===============================================
// CImageMouseOverButton - used for class images
//===============================================

template <class T>
class CImageMouseOverButton : public MouseOverButton<T>
{
private:
	//DECLARE_CLASS_SIMPLE( CImageMouseOverButton, MouseOverButton );

public:
	CImageMouseOverButton( vgui::Panel *parent, const char *panelName, T *templatePanel );

	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnSizeChanged( int newWide, int newTall );

	void RecalculateImageSizes( void );
	void SetActiveImage( const char *imagename );
	void SetInactiveImage( const char *imagename );
	void SetActiveImage( vgui::IImage *image );
	void SetInactiveImage( vgui::IImage *image );

public:
	virtual void Paint();
	virtual void ShowPage( void );
	virtual void HidePage( void );

private:
	vgui::IImage *m_pActiveImage;	
	char *m_pszActiveImageName;

	vgui::IImage *m_pInactiveImage;
	char *m_pszInactiveImageName;

	bool m_bScaleImage;
};

template <class T>
CImageMouseOverButton<T>::CImageMouseOverButton( vgui::Panel *parent, const char *panelName, T *templatePanel ) :
	MouseOverButton<T>( parent, panelName, templatePanel )
{
	m_pszActiveImageName = NULL;
	m_pszInactiveImageName = NULL;

	m_pActiveImage = NULL;
	m_pInactiveImage = NULL;
}

template <class T>
void CImageMouseOverButton<T>::ApplySettings( KeyValues *inResourceData )
{
	m_bScaleImage = inResourceData->GetInt( "scaleImage", 0 );

	// Active Image
	delete [] m_pszActiveImageName;
	m_pszActiveImageName = NULL;

	const char *activeImageName = inResourceData->GetString( "activeimage", "" );
	if ( *activeImageName )
	{
		SetActiveImage( activeImageName );
	}

	// Inactive Image
	delete [] m_pszInactiveImageName;
	m_pszInactiveImageName = NULL;

	const char *inactiveImageName = inResourceData->GetString( "inactiveimage", "" );
	if ( *inactiveImageName )
	{
		SetInactiveImage( inactiveImageName );
	}

	MouseOverButton<T>::ApplySettings( inResourceData );

	InvalidateLayout( false, true ); // force applyschemesettings to run
}

template <class T>
void CImageMouseOverButton<T>::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	MouseOverButton<T>::ApplySchemeSettings( pScheme );

	if ( m_pszActiveImageName && strlen( m_pszActiveImageName ) > 0 )
	{
		SetActiveImage(scheme()->GetImage( m_pszActiveImageName, m_bScaleImage ) );
	}

	if ( m_pszInactiveImageName && strlen( m_pszInactiveImageName ) > 0 )
	{
		SetInactiveImage(scheme()->GetImage( m_pszInactiveImageName, m_bScaleImage ) );
	}

	IBorder *pBorder = pScheme->GetBorder( "NoBorder" );
	SetDefaultBorder( pBorder);
	SetDepressedBorder( pBorder );
	SetKeyFocusBorder( pBorder );

	Color defaultFgColor = GetSchemeColor( "Button.TextColor", Color(255, 255, 255, 255), pScheme );
	Color armedFgColor = GetSchemeColor( "Button.ArmedTextColor", Color(255, 255, 255, 255), pScheme );
	Color depressedFgColor = GetSchemeColor( "Button.DepressedTextColor", Color(255, 255, 255, 255), pScheme );

	Color blank(0,0,0,0);
	SetDefaultColor( defaultFgColor, blank );
	SetArmedColor( armedFgColor, blank );
	SetDepressedColor( depressedFgColor, blank );
}

template <class T>
void CImageMouseOverButton<T>::RecalculateImageSizes( void )
{
	// Reset our images, which will force them to recalculate their size.
	// Necessary for images shared with other scaling buttons.
	SetActiveImage( m_pActiveImage );
	SetInactiveImage( m_pInactiveImage );
}

template <class T>
void CImageMouseOverButton<T>::SetActiveImage( const char *imagename )
{
	int len = Q_strlen( imagename ) + 1;
	m_pszActiveImageName = new char[ len ];
	Q_strncpy( m_pszActiveImageName, imagename, len );
}

template <class T>
void CImageMouseOverButton<T>::SetInactiveImage( const char *imagename )
{
	int len = Q_strlen( imagename ) + 1;
	m_pszInactiveImageName = new char[ len ];
	Q_strncpy( m_pszInactiveImageName, imagename, len );
}

template <class T>
void CImageMouseOverButton<T>::SetActiveImage( vgui::IImage *image )
{
	m_pActiveImage = image;

	if ( m_pActiveImage )
	{
		int wide, tall;
		if ( m_bScaleImage )
		{
			// scaling, force the image size to be our size
			GetSize( wide, tall );
			m_pActiveImage->SetSize( wide, tall );
		}
		else
		{
			// not scaling, so set our size to the image size
			m_pActiveImage->GetSize( wide, tall );
			SetSize( wide, tall );
		}
	}

	Repaint();
}

template <class T>
void CImageMouseOverButton<T>::SetInactiveImage( vgui::IImage *image )
{
	m_pInactiveImage = image;

	if ( m_pInactiveImage )
	{
		int wide, tall;
		if ( m_bScaleImage)
		{
			// scaling, force the image size to be our size
			GetSize( wide, tall );
			m_pInactiveImage->SetSize( wide, tall );
		}
		else
		{
			// not scaling, so set our size to the image size
			m_pInactiveImage->GetSize( wide, tall );
			SetSize( wide, tall );
		}
	}

	Repaint();
}

template <class T>
void CImageMouseOverButton<T>::OnSizeChanged( int newWide, int newTall )
{
	if ( m_bScaleImage )
	{
		// scaling, force the image size to be our size

		if ( m_pActiveImage )
			m_pActiveImage->SetSize( newWide, newTall );

		if ( m_pInactiveImage )
			m_pInactiveImage->SetSize( newWide, newTall );
	}
	MouseOverButton<T>::OnSizeChanged( newWide, newTall );
}

template <class T>
void CImageMouseOverButton<T>::Paint()
{
	SetActiveImage( m_pActiveImage );
	SetInactiveImage( m_pInactiveImage );

	if ( IsArmed() )
	{
		// draw the active image
		if ( m_pActiveImage )
		{
			vgui::surface()->DrawSetColor( 255, 255, 255, 255 );
			m_pActiveImage->SetPos( 0, 0 );
			m_pActiveImage->Paint();
		}
	}
	else 
	{
		// draw the inactive image
		if ( m_pInactiveImage )
		{
			vgui::surface()->DrawSetColor( 255, 255, 255, 255 );
			m_pInactiveImage->SetPos( 0, 0 );
			m_pInactiveImage->Paint();
		}
	}
	
	MouseOverButton<T>::Paint();
}

template <class T>
void CImageMouseOverButton<T>::ShowPage( void )
{
	MouseOverButton<T>::ShowPage();

	// send message to parent that we triggered something
	PostActionSignal( new KeyValues( "ShowPage", "page", GetName() ) );
}

template <class T>
void CImageMouseOverButton<T>::HidePage( void )
{
	MouseOverButton<T>::HidePage();

	// send message to parent that we triggered something
	PostActionSignal( new KeyValues( "ShowPage", "page", GetName() ) );
}

#endif //IMAGE_MOUSE_OVER_BUTTON_H