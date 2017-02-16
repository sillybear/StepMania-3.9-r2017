#include "global.h"
#include "MusicWheelItem.h"
#include "RageUtil.h"
#include "SongManager.h"
#include "GameManager.h"
#include "PrefsManager.h"
#include "ScreenManager.h"	// for sending SM_PlayMusicSample
#include "RageLog.h"
#include "GameConstantsAndTypes.h"
#include "GameState.h"
#include "ThemeManager.h"
#include "Steps.h"
#include "Song.h"
#include "Course.h"
#include "ProfileManager.h"
#include "ActorUtil.h"

// WheelItem stuff
#define ICON_X			THEME->GetMetricF("MusicWheelItem","IconX")
#define SONG_NAME_X		THEME->GetMetricF("MusicWheelItem","SongNameX")
#define SECTION_NAME_X		THEME->GetMetricF("MusicWheelItem","SectionNameX")
#define SECTION_ZOOM		THEME->GetMetricF("MusicWheelItem","SectionZoom")
#define ROULETTE_X		THEME->GetMetricF("MusicWheelItem","RouletteX")
#define ROULETTE_ZOOM		THEME->GetMetricF("MusicWheelItem","RouletteZoom")
#define GRADE_X( p )		THEME->GetMetricF("MusicWheelItem",ssprintf("GradeP%dX",p+1))

#define USE_BANNER_WHEEL	THEME->GetMetricB("ScreenSelectMusic","UseBannerWheel")
#define HIGH_QUAL_TIME		THEME->GetMetricF("FadingBanner","FadeSeconds")

#define BANNER_WIDTH		THEME->GetMetricF("MusicWheelItem","BannerWidth")
#define BANNER_HEIGHT		THEME->GetMetricF("MusicWheelItem","BannerHeight")

WheelItemData::WheelItemData( WheelItemType wit, Song* pSong, CString sSectionName, Course* pCourse, RageColor color, SortOrder so )
{
	m_Type = wit;
	m_pSong = pSong;
	m_sSectionName = sSectionName;
	m_pCourse = pCourse;
	m_color = color;
	m_Flags = WheelNotifyIcon::Flags();
	m_SortOrder = so;
}

MusicWheelItem::MusicWheelItem()
{
	data = NULL;

	SetName( "MusicWheelItem" );

	m_fPercentGray = 0;
	m_WheelNotifyIcon.SetXY( ICON_X, 0 );

	m_sprSongBar.Load( THEME->GetPathToG("MusicWheelItem song") );
	m_sprSongBar.SetXY( 0, 0 );
	m_All.AddChild( &m_sprSongBar );

	m_sprSectionBar.Load( THEME->GetPathToG("MusicWheelItem section") );
	m_sprSectionBar.SetXY( 0, 0 );
	m_All.AddChild( &m_sprSectionBar );

	m_textSectionName.LoadFromFont( THEME->GetPathToF("MusicWheelItem section") );
	m_textSectionName.SetShadowLength( 0 );
	m_textSectionName.SetVertAlign( align_middle );
	m_textSectionName.SetXY( SECTION_NAME_X, 0 );
	m_textSectionName.SetZoom( SECTION_ZOOM );
	m_All.AddChild( &m_textSectionName );

	// Load the fonts for the various sorts
	GAMESTATE->m_bLoadingSortFonts = true;

	m_textSortGroup.LoadFromFont( THEME->GetPathToF("MusicWheelItem sort group") );
	m_textSortGroup.SetShadowLength( 0 );
	m_textSortGroup.SetVertAlign( align_middle );
	m_textSortGroup.SetXY( SECTION_NAME_X, 0 );
	m_textSortGroup.SetZoom( SECTION_ZOOM );
	m_All.AddChild( &m_textSortGroup );

	m_textSortTitle.LoadFromFont( THEME->GetPathToF("MusicWheelItem sort title") );
	m_textSortTitle.SetShadowLength( 0 );
	m_textSortTitle.SetVertAlign( align_middle );
	m_textSortTitle.SetXY( SECTION_NAME_X, 0 );
	m_textSortTitle.SetZoom( SECTION_ZOOM );
	m_All.AddChild( &m_textSortTitle );

	m_textSortGenre.LoadFromFont( THEME->GetPathToF("MusicWheelItem sort genre") );
	m_textSortGenre.SetShadowLength( 0 );
	m_textSortGenre.SetVertAlign( align_middle );
	m_textSortGenre.SetXY( SECTION_NAME_X, 0 );
	m_textSortGenre.SetZoom( SECTION_ZOOM );
	m_All.AddChild( &m_textSortGenre );

	m_textSortBPM.LoadFromFont( THEME->GetPathToF("MusicWheelItem sort bpm") );
	m_textSortBPM.SetShadowLength( 0 );
	m_textSortBPM.SetVertAlign( align_middle );
	m_textSortBPM.SetXY( SECTION_NAME_X, 0 );
	m_textSortBPM.SetZoom( SECTION_ZOOM );
	m_All.AddChild( &m_textSortBPM );

	m_textSortGrade.LoadFromFont( THEME->GetPathToF("MusicWheelItem sort grade") );
	m_textSortGrade.SetShadowLength( 0 );
	m_textSortGrade.SetVertAlign( align_middle );
	m_textSortGrade.SetXY( SECTION_NAME_X, 0 );
	m_textSortGrade.SetZoom( SECTION_ZOOM );
	m_All.AddChild( &m_textSortGrade );

	m_textSortArtist.LoadFromFont( THEME->GetPathToF("MusicWheelItem sort artist") );
	m_textSortArtist.SetShadowLength( 0 );
	m_textSortArtist.SetVertAlign( align_middle );
	m_textSortArtist.SetXY( SECTION_NAME_X, 0 );
	m_textSortArtist.SetZoom( SECTION_ZOOM );
	m_All.AddChild( &m_textSortArtist );

	m_textSortEasy.LoadFromFont( THEME->GetPathToF("MusicWheelItem sort easy meter") );
	m_textSortEasy.SetShadowLength( 0 );
	m_textSortEasy.SetVertAlign( align_middle );
	m_textSortEasy.SetXY( SECTION_NAME_X, 0 );
	m_textSortEasy.SetZoom( SECTION_ZOOM );
	m_All.AddChild( &m_textSortEasy );

	m_textSortMedium.LoadFromFont( THEME->GetPathToF("MusicWheelItem sort medium meter") );
	m_textSortMedium.SetShadowLength( 0 );
	m_textSortMedium.SetVertAlign( align_middle );
	m_textSortMedium.SetXY( SECTION_NAME_X, 0 );
	m_textSortMedium.SetZoom( SECTION_ZOOM );
	m_All.AddChild( &m_textSortMedium );

	m_textSortHard.LoadFromFont( THEME->GetPathToF("MusicWheelItem sort hard meter") );
	m_textSortHard.SetShadowLength( 0 );
	m_textSortHard.SetVertAlign( align_middle );
	m_textSortHard.SetXY( SECTION_NAME_X, 0 );
	m_textSortHard.SetZoom( SECTION_ZOOM );
	m_All.AddChild( &m_textSortHard );

	m_textSortChallenge.LoadFromFont( THEME->GetPathToF("MusicWheelItem sort challenge meter") );
	m_textSortChallenge.SetShadowLength( 0 );
	m_textSortChallenge.SetVertAlign( align_middle );
	m_textSortChallenge.SetXY( SECTION_NAME_X, 0 );
	m_textSortChallenge.SetZoom( SECTION_ZOOM );
	m_All.AddChild( &m_textSortChallenge );

	// Finally, load for Roulette
	GAMESTATE->m_bLoadingSortFonts = false;

	m_textRoulette.LoadFromFont( THEME->GetPathToF("MusicWheelItem roulette") );
	m_textRoulette.SetShadowLength( 0 );
	m_textRoulette.TurnRainbowOn();
	m_textRoulette.SetZoom( ROULETTE_ZOOM );
	m_textRoulette.SetXY( ROULETTE_X, 0 );
	m_All.AddChild( &m_textRoulette );

	// Load if we're using the banner wheel
	if( USE_BANNER_WHEEL )
	{
		m_Banner.SetName( "Banner" );
		m_Banner.SetZTestMode( ZTEST_WRITE_ON_PASS );	// do have to pass the z test
		m_Banner.ScaleToClipped( BANNER_WIDTH, BANNER_HEIGHT );
		SET_XY_AND_ON_COMMAND( m_Banner );
		m_All.AddChild( &m_Banner );
	}
	// If not, load the normal text selection!
	else
	{
		m_TextBanner.SetName( "TextBanner" );
		m_TextBanner.SetHorizAlign( align_left );
		m_TextBanner.SetXY( SONG_NAME_X, 0 );
		m_All.AddChild( &m_TextBanner );
	}

	FOREACH_PlayerNumber( p )
	{
		m_GradeDisplay[p].Load( THEME->GetPathToG("MusicWheelItem grades") );
		m_GradeDisplay[p].SetZoom( 1.0f );
		m_GradeDisplay[p].SetXY( GRADE_X(p), 0 );
		m_All.AddChild( &m_GradeDisplay[p] );
	}

	m_textCourse.SetName( "CourseName" );
	m_textCourse.LoadFromFont( THEME->GetPathToF("MusicWheelItem course") );
	SET_XY_AND_ON_COMMAND( &m_textCourse );
	m_All.AddChild( &m_textCourse );

	m_textSort.SetName( "Sort" );
	m_textSort.LoadFromFont( THEME->GetPathToF("MusicWheelItem sort") );
	SET_XY_AND_ON_COMMAND( &m_textSort );
	m_All.AddChild( &m_textSort );
}


void MusicWheelItem::LoadFromWheelItemData( WheelItemData* pWID )
{
	ASSERT( pWID != NULL );
	
	data = pWID;
	/*
	// copy all data items
	this->m_Type			= pWID->m_Type;
	this->m_sSectionName		= pWID->m_sSectionName;
	this->m_pCourse			= pWID->m_pCourse;
	this->m_pSong			= pWID->m_pSong;
	this->m_color			= pWID->m_color;
	this->m_Type			= pWID->m_Type; */

	// init type specific stuff
	switch( pWID->m_Type )
	{
	case TYPE_SECTION:
	case TYPE_COURSE:
	case TYPE_SORT:
		{
			CString sDisplayName, sTranslitName;
			BitmapText *bt = NULL;

			switch( pWID->m_Type )
			{
				case TYPE_SECTION:
				{
					switch( GAMESTATE->m_SortOrder )
					{
					case SORT_GROUP:
						{
							//if( USE_BANNER_WHEEL )
							//{
							//	CString sPath;
							//	sPath = SONGMAN->GetGroupBannerPath(data->m_sSectionName);
							//	m_Banner.Load( sPath );
							//}
							//else
							//{
								sDisplayName = SONGMAN->ShortenGroupName(data->m_sSectionName);
								bt = &m_textSortGroup;
							//}
						}
						break;
					case SORT_TITLE:
						sDisplayName = SONGMAN->ShortenGroupName(data->m_sSectionName);
						bt = &m_textSortTitle;
						break;
					case SORT_GENRE:
						sDisplayName = SONGMAN->ShortenGroupName(data->m_sSectionName);
						bt = &m_textSortGenre;
						break;
					case SORT_BPM:
						sDisplayName = SONGMAN->ShortenGroupName(data->m_sSectionName);
						bt = &m_textSortBPM;
						break;
					case SORT_GRADE:
						sDisplayName = SONGMAN->ShortenGroupName(data->m_sSectionName);
						bt = &m_textSortGrade;
						break;
					case SORT_ARTIST:
						sDisplayName = SONGMAN->ShortenGroupName(data->m_sSectionName);
						bt = &m_textSortArtist;
						break;
					case SORT_EASY_METER:
						sDisplayName = SONGMAN->ShortenGroupName(data->m_sSectionName);
						bt = &m_textSortEasy;
						break;
					case SORT_MEDIUM_METER:
						sDisplayName = SONGMAN->ShortenGroupName(data->m_sSectionName);
						bt = &m_textSortMedium;
						break;
					case SORT_HARD_METER:
						sDisplayName = SONGMAN->ShortenGroupName(data->m_sSectionName);
						bt = &m_textSortHard;
						break;
					case SORT_CHALLENGE_METER:
						sDisplayName = SONGMAN->ShortenGroupName(data->m_sSectionName);
						bt = &m_textSortChallenge;
						break;
					default:
						sDisplayName = SONGMAN->ShortenGroupName(data->m_sSectionName);
						bt = &m_textSectionName;
						break;
					}
					break;
				}
				case TYPE_COURSE:
					{
						//if( USE_BANNER_WHEEL )
						//{
						//	CString sPath;
						//	sPath = data->m_pCourse->m_sBannerPath;
						//	m_Banner.Load( sPath );
						//}
						//else
						//{
							sDisplayName = data->m_pCourse->GetFullDisplayTitle();
							sTranslitName = data->m_pCourse->GetFullTranslitTitle();
							bt = &m_textCourse;
						//}
					}
					break;
				case TYPE_SORT:
					sDisplayName = data->m_sLabel;
					bt = &m_textSort;
					break;
				default:
					ASSERT(0);
			}

			//if( !USE_BANNER_WHEEL || pWID->m_Type == TYPE_SORT || (pWID->m_Type == TYPE_SECTION && GAMESTATE->m_SortOrder != SORT_GROUP) )
			//{
				bt->SetZoom( 1 );
				bt->SetText( sDisplayName, sTranslitName );
				bt->SetDiffuse( data->m_color );
				bt->TurnRainbowOff();

				const float fSourcePixelWidth = (float)bt->GetUnzoomedWidth();
				const float fMaxTextWidth = 200;

				if( fSourcePixelWidth > fMaxTextWidth  )
					bt->SetZoomX( fMaxTextWidth / fSourcePixelWidth );
			//}
		}
		break;
	case TYPE_SONG:
		{
			if( USE_BANNER_WHEEL )
			{
				CString sPath;
				sPath = data->m_pSong->GetBannerPath();
				m_Banner.Load( sPath );
			}
			else
			{
				m_TextBanner.LoadFromSong( data->m_pSong );
				m_TextBanner.SetDiffuse( data->m_color );
			}

			m_WheelNotifyIcon.SetFlags( data->m_Flags );
			RefreshGrades();
		}
		break;
	case TYPE_ROULETTE:
		{
			//if( USE_BANNER_WHEEL )
			//	m_Banner.LoadRoulette();
			//else
				m_textRoulette.SetText( THEME->GetMetric("MusicWheel","Roulette") );
		}
		break;
	case TYPE_RANDOM:
		{
			//if( USE_BANNER_WHEEL )
			//	m_Banner.LoadRandom();
			//else
				m_textRoulette.SetText( THEME->GetMetric("MusicWheel","Random") );
		}
		break;

	case TYPE_PORTAL:
		{
			//if( USE_BANNER_WHEEL )
			//{
			//	CString sPath;
			//	sPath = data->m_pSong->GetBannerPath();
			//	m_Banner.Load( sPath );
			//}
			//else
				m_textRoulette.SetText( THEME->GetMetric("MusicWheel","Portal") );
		}
		break;

	default:
		ASSERT( 0 );	// invalid type
	}
}

void MusicWheelItem::RefreshGrades()
{
	// Refresh Grades
	FOREACH_PlayerNumber( p )
	{
		if( !data->m_pSong || !GAMESTATE->IsHumanPlayer(p) ) // this isn't a song display, or not a human player
		{
			m_GradeDisplay[p].SetDiffuse( RageColor(1,1,1,0) );
			continue;
		}

		Difficulty dc;
		Grade grade;

		if( GAMESTATE->m_pCurSteps[p] )
			dc = GAMESTATE->m_pCurSteps[p]->GetDifficulty();
		else
			dc = GAMESTATE->m_PreferredDifficulty[p];

		if( PROFILEMAN->IsUsingProfile((PlayerNumber)p) )
			grade = PROFILEMAN->GetHighScoreForDifficulty( data->m_pSong, GAMESTATE->GetCurrentStyle(), (ProfileSlot)p, dc ).grade;
		else
			grade = PROFILEMAN->GetHighScoreForDifficulty( data->m_pSong, GAMESTATE->GetCurrentStyle(), PROFILE_SLOT_MACHINE, dc ).grade;

		m_GradeDisplay[p].SetGrade( (PlayerNumber)p, grade );
	}

}


void MusicWheelItem::Update( float fDeltaTime )
{
	Actor::Update( fDeltaTime );

	switch( data->m_Type )
	{
	case TYPE_SECTION:
	{
		m_sprSectionBar.Update( fDeltaTime );

		switch( GAMESTATE->m_SortOrder )
		{
		case SORT_GROUP:
			m_textSortGroup.Update( fDeltaTime );
			break;
		case SORT_TITLE:
			m_textSortTitle.Update( fDeltaTime );
			break;
		case SORT_GENRE:
			m_textSortGenre.Update( fDeltaTime );
			break;
		case SORT_BPM:
			m_textSortBPM.Update( fDeltaTime );
			break;
		case SORT_GRADE:
			m_textSortGrade.Update( fDeltaTime );
			break;
		case SORT_ARTIST:
			m_textSortArtist.Update( fDeltaTime );
			break;
		case SORT_EASY_METER:
			m_textSortEasy.Update( fDeltaTime );
			break;
		case SORT_MEDIUM_METER:
			m_textSortMedium.Update( fDeltaTime );
			break;
		case SORT_HARD_METER:
			m_textSortHard.Update( fDeltaTime );
			break;
		case SORT_CHALLENGE_METER:
			m_textSortChallenge.Update( fDeltaTime );
			break;
		default:
			m_textSectionName.Update( fDeltaTime );
			break;
		}
		break;
	}
	case TYPE_ROULETTE:
	case TYPE_RANDOM:
	case TYPE_PORTAL:
		m_sprSectionBar.Update( fDeltaTime );
		m_textRoulette.Update( fDeltaTime );
		break;
	case TYPE_SONG:
		{
			m_sprSongBar.Update( fDeltaTime );
			m_WheelNotifyIcon.Update( fDeltaTime );

			if( USE_BANNER_WHEEL )
				m_Banner.Update(HIGH_QUAL_TIME);
			else
				m_TextBanner.Update( fDeltaTime );

			FOREACH_PlayerNumber( p )
				m_GradeDisplay[p].Update( fDeltaTime );
		}
		break;
	case TYPE_COURSE:
		m_sprSongBar.Update( fDeltaTime );
		m_textCourse.Update( fDeltaTime );
		break;
	case TYPE_SORT:
		m_sprSectionBar.Update( fDeltaTime );
		m_textSort.Update( fDeltaTime );
		break;
	default:
		ASSERT(0);
	}
}

void MusicWheelItem::DrawPrimitives()
{
	Sprite *bar = NULL;
	switch( data->m_Type )
	{
	case TYPE_SECTION: 
	case TYPE_ROULETTE:
	case TYPE_RANDOM:
	case TYPE_PORTAL:
	case TYPE_SORT:
		bar = &m_sprSectionBar; 
		break;
	case TYPE_SONG:		
	case TYPE_COURSE:
		bar = &m_sprSongBar; 
		break;
	default: ASSERT(0);
	}
	
	bar->Draw();

	switch( data->m_Type )
	{
	case TYPE_SECTION:
	{
		switch( GAMESTATE->m_SortOrder )
		{
		case SORT_GROUP:
			m_textSortGroup.Draw();
			break;
		case SORT_TITLE:
			m_textSortTitle.Draw();
			break;
		case SORT_GENRE:
			m_textSortGenre.Draw();
			break;
		case SORT_BPM:
			m_textSortBPM.Draw();
			break;
		case SORT_GRADE:
			m_textSortGrade.Draw();
			break;
		case SORT_ARTIST:
			m_textSortArtist.Draw();
			break;
		case SORT_EASY_METER:
			m_textSortEasy.Draw();
			break;
		case SORT_MEDIUM_METER:
			m_textSortMedium.Draw();
			break;
		case SORT_HARD_METER:
			m_textSortHard.Draw();
			break;
		case SORT_CHALLENGE_METER:
			m_textSortChallenge.Draw();
			break;
		default:
			m_textSectionName.Draw();
			break;
		}
		break;
	}
	case TYPE_ROULETTE:
	case TYPE_RANDOM:
	case TYPE_PORTAL:
		m_textRoulette.Draw();
		break;
	case TYPE_SONG:	
		if( USE_BANNER_WHEEL )
			m_Banner.Draw();
		else
			m_TextBanner.Draw();

		m_WheelNotifyIcon.Draw();
		FOREACH_PlayerNumber( p )
			m_GradeDisplay[p].Draw();
		break;
	case TYPE_COURSE:
		m_textCourse.Draw();
		break;
	case TYPE_SORT:
		m_textSort.Draw();
		break;
	default:
		ASSERT(0);
	}

	if( m_fPercentGray > 0 )
	{
		bar->SetGlow( RageColor(0,0,0,m_fPercentGray) );
		bar->SetDiffuse( RageColor(0,0,0,0) );
		bar->Draw();
		bar->SetDiffuse( RageColor(0,0,0,1) );
		bar->SetGlow( RageColor(0,0,0,0) );
	}
}


void MusicWheelItem::SetZTestMode( ZTestMode mode )
{
	ActorFrame::SetZTestMode( mode );

	// set all sub-Actors
	m_All.SetZTestMode( mode );
}

void MusicWheelItem::SetZWrite( bool b )
{
	ActorFrame::SetZWrite( b );

	// set all sub-Actors
	m_All.SetZWrite( b );
}

/*
 * (c) 2001-2004 Chris Danford, Chris Gomez, Glenn Maynard
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
