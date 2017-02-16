#include "global.h"
#include "FadingBackground.h"
#include "RageTextureManager.h"
#include "BackgroundCache.h"
#include "Song.h"
#include "RageLog.h"
#include "Course.h"
#include "PrefsManager.h"
#include "ThemeManager.h"
#include "SongManager.h"

CachedThemeMetricF FADE_BG_SECONDS			("FadingBackground","FadeSeconds");

FadingBackground::FadingBackground()
{
	FADE_BG_SECONDS.Refresh();

	m_bMovingFast = false;
	m_bSkipNextBackgroundUpdate = false;
	m_iIndexFront = 0;
	for( int i=0; i<2; i++ )
		this->AddChild( &m_Background[i] );
}

void FadingBackground::ScaleToClipped( float fWidth, float fHeight )
{
	for( int i=0; i<2; i++ )
		m_Background[i].ScaleToClipped( fWidth, fHeight );
}

void FadingBackground::Update( float fDeltaTime )
{
	// update children manually
	// ActorFrame::Update( fDeltaTime );
	Actor::Update( fDeltaTime );

	if( !m_bSkipNextBackgroundUpdate )
	{
		m_Background[0].Update( fDeltaTime );
		m_Background[1].Update( fDeltaTime );
	}

	m_bSkipNextBackgroundUpdate = false;
}

void FadingBackground::DrawPrimitives()
{
	// draw manually
//	ActorFrame::DrawPrimitives();
	m_Background[GetBackIndex()].Draw();
	m_Background[m_iIndexFront].Draw();
}

bool FadingBackground::Load( RageTextureID ID )
{
	BeforeChange();
	bool bRet = m_Background[GetBackIndex()].Load(ID);
	return bRet;
}

void FadingBackground::BeforeChange()
{
	m_Background[m_iIndexFront].SetDiffuse( RageColor(1,1,1,1) );

	m_iIndexFront = GetBackIndex();

	m_Background[m_iIndexFront].SetDiffuse( RageColor(1,1,1,1) );
	m_Background[m_iIndexFront].StopTweening();
	m_Background[m_iIndexFront].BeginTweening( FADE_BG_SECONDS );		// fade out
	m_Background[m_iIndexFront].SetDiffuse( RageColor(1,1,1,0) );

	// We're about to load a background.  It'll probably cause a frame skip or two. Skip an update, so 
	// the fade-in doesn't skip.
	m_bSkipNextBackgroundUpdate = true;
}

// If this returns true, a low-resolution background was loaded, and the full-res background should be loaded later.
bool FadingBackground::LoadFromCachedBackground( const CString &path )
{
	/* If we're already on the given background, don't fade again. */
	if( path != "" && m_Background[GetBackIndex()].GetTexturePath() == path )
		return false;

	if( path == "" )
	{
		LoadFallback();
		return false;
	}

	// If we're currently fading to the given background, go through this again, which will cause the fade-in
	// to be further delayed.

	RageTextureID ID;
	bool bLowRes = (PREFSMAN->m_BackgroundCache != PrefsManager::BGCACHE_FULL);
	if( !bLowRes )
		ID = Sprite::SongBackgroundSSMTexture( path );
	else
		/* Try to load the low quality version. */
		ID = BACKGROUNDCACHE->LoadCachedBackground( path );

	if( !TEXTUREMAN->IsTextureRegistered(ID) )
	{
		/* Oops.  We couldn't load a background quickly.  We can load the actual
		 * background, but that's slow, so we don't want to do that when we're moving
		 * fast on the music wheel.  In that case, we should just keep the background
		 * that's there (or load a "moving fast" background).  Once we settle down,
		 * we'll get called again and load the real background. */

		if( m_bMovingFast )
			return false;

		if( IsAFile(path) )
			Load( path );
		else
			LoadFallback();

		return false;
	}

	BeforeChange();
	m_Background[GetBackIndex()].Load( ID );

	return bLowRes;
}

void FadingBackground::LoadFromSong( const Song* pSong )
{
	// Don't call HasBackground.  That'll do disk access and cause the music wheel to skip.
	LoadFromCachedBackground( pSong->GetBackgroundPath() );
}

void FadingBackground::LoadAllMusic()
{
	BeforeChange();
	m_Background[GetBackIndex()].LoadAllMusic();
}

void FadingBackground::LoadSort()
{
	BeforeChange();
	m_Background[GetBackIndex()].LoadSort();
}

void FadingBackground::LoadMode()
{
	BeforeChange();
	m_Background[GetBackIndex()].LoadMode();
}

void FadingBackground::LoadFromGroup( CString sGroupName )
{
	const CString sGroupBackgroundPath = SONGMAN->GetGroupBackgroundPath( sGroupName );
	LoadFromCachedBackground( sGroupBackgroundPath );
}

void FadingBackground::LoadFromCourse( const Course* pCourse )
{
	LoadFromCachedBackground( pCourse->m_sBackgroundPath );
}

void FadingBackground::LoadRoulette()
{
	BeforeChange();
	m_Background[GetBackIndex()].LoadRoulette();
}

void FadingBackground::LoadRandom()
{
	BeforeChange();
	m_Background[GetBackIndex()].LoadRandom();
}

void FadingBackground::LoadFallback()
{
	BeforeChange();
	m_Background[GetBackIndex()].LoadFallback();
}

/*
 * (c) 2001-2004 Chris Danford
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
