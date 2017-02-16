#include "global.h"
#include "BackgroundSSM.h"
#include "PrefsManager.h"
#include "SongManager.h"
#include "ThemeManager.h"
#include "RageUtil.h"
#include "Song.h"
#include "RageTextureManager.h"
#include "Course.h"
#include "Character.h"

CachedThemeMetricB SCROLL_BG_RANDOM			("BackgroundSSM","ScrollRandom");
CachedThemeMetricB SCROLL_BG_ROULETTE		("BackgroundSSM","ScrollRoulette");

BackgroundSSM::BackgroundSSM()
{
	SCROLL_BG_RANDOM.Refresh();
	SCROLL_BG_ROULETTE.Refresh();

	m_bScrolling = false;
	m_fPercentScrolling = 0;

	if( PREFSMAN->m_BackgroundCache != PrefsManager::BGCACHE_OFF )
	{
		TEXTUREMAN->CacheTexture( SongBannerTexture(THEME->GetPathG("Background","all music")) );
		TEXTUREMAN->CacheTexture( SongBannerTexture(THEME->GetPathG("Common","fallback background")) );
		TEXTUREMAN->CacheTexture( SongBannerTexture(THEME->GetPathG("Background","roulette")) );
		TEXTUREMAN->CacheTexture( SongBannerTexture(THEME->GetPathG("Background","random")) );
		TEXTUREMAN->CacheTexture( SongBannerTexture(THEME->GetPathG("Background","Sort")) );
		TEXTUREMAN->CacheTexture( SongBannerTexture(THEME->GetPathG("Background","Mode")) );
	}
}

bool BackgroundSSM::Load( RageTextureID ID )
{
	if( ID.filename == "" )
		ID = THEME->GetPathToG("Common fallback background");

	ID = SongBackgroundSSMTexture(ID);

	m_fPercentScrolling = 0;
	m_bScrolling = false;

	TEXTUREMAN->DisableOddDimensionWarning();
	TEXTUREMAN->VolatileTexture( ID );
	bool ret = Sprite::Load( ID );
	TEXTUREMAN->EnableOddDimensionWarning();

	return ret;
};

void BackgroundSSM::Update( float fDeltaTime )
{
	Sprite::Update( fDeltaTime );

	if( m_bScrolling )
	{
        m_fPercentScrolling += fDeltaTime/2;
		m_fPercentScrolling -= (int)m_fPercentScrolling;

		const RectF *pTextureRect = m_pTexture->GetTextureCoordRect(0);
 
		float fTexCoords[8] = 
		{
			0+m_fPercentScrolling, pTextureRect->top,		// top left
			0+m_fPercentScrolling, pTextureRect->bottom,	// bottom left
			1+m_fPercentScrolling, pTextureRect->bottom,	// bottom right
			1+m_fPercentScrolling, pTextureRect->top,		// top right
		};
		Sprite::SetCustomTextureCoords( fTexCoords );
	}
}

void BackgroundSSM::SetScrolling( bool bScroll, float Percent)
{
	m_bScrolling = bScroll;
	m_fPercentScrolling = Percent;

	// Set up the texture coord rects for the current state.
	Update(0);
}

void BackgroundSSM::LoadFromSong( Song* pSong )		// NULL means no song
{
	if( pSong == NULL )						LoadFallback();
	else if( pSong->HasBackground() )		Load( pSong->GetBackgroundPath() );
	else									LoadFallback();

	m_bScrolling = false;
}

void BackgroundSSM::LoadAllMusic()
{
	Load( THEME->GetPathG("Background","All") );
	m_bScrolling = false;
}

void BackgroundSSM::LoadSort()
{
	Load( THEME->GetPathG("Background","Sort") );
	m_bScrolling = false;
}

void BackgroundSSM::LoadMode()
{
	Load( THEME->GetPathG("Background","Mode") );
	m_bScrolling = false;
}

void BackgroundSSM::LoadFromGroup( CString sGroupName )
{
	CString sGroupBackgroundPath = SONGMAN->GetGroupBackgroundPath( sGroupName );
	if( sGroupBackgroundPath != "" )	Load( sGroupBackgroundPath );
	else								LoadFallback();
	m_bScrolling = false;
}

void BackgroundSSM::LoadFromCourse( Course* pCourse )		// NULL means no course
{
	if( pCourse == NULL )							LoadFallback();
	else if( pCourse->m_sBackgroundPath != "" )		Load( pCourse->m_sBackgroundPath );
	else											LoadFallback();

	m_bScrolling = false;
}

void BackgroundSSM::LoadCardFromCharacter( Character* pCharacter )	
{
	ASSERT( pCharacter );

	if( pCharacter->GetCardPath() != "" )		Load( pCharacter->GetCardPath() );
	else										LoadFallback();

	m_bScrolling = false;
}

void BackgroundSSM::LoadIconFromCharacter( Character* pCharacter )	
{
	ASSERT( pCharacter );

	if( pCharacter->GetIconPath() != "" )		Load( pCharacter->GetIconPath() );
	else if( pCharacter->GetCardPath() != "" )	Load( pCharacter->GetCardPath() );
	else										LoadFallback();

	m_bScrolling = false;
}

void BackgroundSSM::LoadTABreakFromCharacter( Character* pCharacter )
{
	if( pCharacter == NULL )					Load( THEME->GetPathToG("Common fallback takingabreak") );
	else 
	{
		Load( pCharacter->GetTakingABreakPath() );
		m_bScrolling = false;
	}
}

void BackgroundSSM::LoadFallback()
{
	Load( THEME->GetPathToG("Common fallback background") );
}

void BackgroundSSM::LoadRoulette()
{
	Load( THEME->GetPathToG("Background roulette") );
	m_bScrolling = (bool)SCROLL_BG_ROULETTE;
}

void BackgroundSSM::LoadRandom()
{
	Load( THEME->GetPathToG("Background random") );
	m_bScrolling = (bool)SCROLL_BG_RANDOM;
}


/*
 * (c) 2001-2004 Chris Danford, 2008 Mike Hawkins
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
