#include "global.h"
#include "MissionContentsList.h"
#include "RageUtil.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "PrefsManager.h"
#include "Course.h"
#include "SongManager.h"
#include "ThemeManager.h"
#include "Steps.h"
#include "GameState.h"
#include "Style.h"
#include "RageTexture.h"
#include "ActorUtil.h"

/* CachedThemeMetric	HIDDEN_LABEL		("MissionMode","Hidden");
CachedThemeMetric	MARVELOUS_LABEL		("MissionMode","Marvelous");
CachedThemeMetric	PERFECT_LABEL		("MissionMode","Perfect");
CachedThemeMetric	GREAT_LABEL		("MissionMode","Great");
CachedThemeMetric	GOOD_LABEL		("MissionMode","Good");
CachedThemeMetric	BOO_LABEL		("MissionMode","Boo");
CachedThemeMetric	MISS_LABEL		("MissionMode","Miss");
CachedThemeMetric	HIT_MINE_LABEL		("MissionMode","HitMine");
CachedThemeMetric	HOLD_OK_LABEL		("MissionMode","HoldOK");
CachedThemeMetric	HOLD_NG_LABEL		("MissionMode","HoldNG");
CachedThemeMetric	ROLL_OK_LABEL		("MissionMode","RollOK");
CachedThemeMetric	ROLL_NG_LABEL		("MissionMode","RollNG");
CachedThemeMetric	SCORE_LABEL		("MissionMode","Score");
CachedThemeMetric	MAX_COMBO_LABEL		("MissionMode","MaxCombo"); */


MissionContentsList::MissionContentsList()
{
	m_iNumContents = 0;
	m_fTimeUntilScroll = 0;
	m_iItemAtTopOfList = 0;

	/*HIDDEN_LABEL.Refresh();
	MARVELOUS_LABEL.Refresh();
	PERFECT_LABEL.Refresh();
	GREAT_LABEL.Refresh();
	GOOD_LABEL.Refresh();
	BOO_LABEL.Refresh();
	MISS_LABEL.Refresh();
	HIT_MINE_LABEL.Refresh();
	HOLD_OK_LABEL.Refresh();
	HOLD_NG_LABEL.Refresh();
	ROLL_OK_LABEL.Refresh();
	ROLL_NG_LABEL.Refresh();
	SCORE_LABEL.Refresh();
	MAX_COMBO_LABEL.Refresh();*/

	for( int l=0; l<MAX_VISIBLE_MISSIONS; l++ ) 
	{
		m_textMissionGoals[l].LoadFromFont( THEME->GetPathF("", "ScreenSelectMusic mission text") );
		m_textMissionGoals[l].SetShadowLength( 0 );
		m_textMissionGoals[l].SetName( ssprintf("Mission %d",l+1) );
		SET_XY_AND_ON_COMMAND( m_textMissionGoals[l] );
		this->AddChild( &m_textMissionGoals[l] );
	}
}


void MissionContentsList::SetFromGameState()
{
	if( !GAMESTATE->IsMissionMode() )
	{
		m_iNumContents = 0;
		return;
	}

	Course* pCourse = GAMESTATE->m_pCurCourse;
	if( pCourse == NULL )
	{
		m_iNumContents = 0;
		return;
	}

	int iNumEntriesToShow = min((int)(pCourse->m_iGoalLess.size()+pCourse->m_iGoalEqual.size()+pCourse->m_iGoalGreater.size()), MAX_TOTAL_MISSIONS);
	m_iNumContents = 0;
	
	for( int l=0; l<iNumEntriesToShow; l++ )
	{
		unsigned i = 0;
		unsigned j = 10000;
		unsigned k = 10000;

		// Done loading Goal Less, go to Goal Equal
		if( i == pCourse->m_sGoalLess.size() )
			j = 0;

		// Done loading Goal Equal, go to Goal Greater
		if( j == pCourse->m_sGoalEqual.size() )
			k = 0;

		// Load the goals here
		if( i < pCourse->m_sGoalLess.size() )
		{
			CString sString = ConvertToGoalString( pCourse->m_sGoalLess[i], "<", pCourse->m_iGoalLess[i] );

			m_sTextLines.push_back( sString );
		}

		if( j < pCourse->m_sGoalEqual.size() )
		{
			CString sString = ConvertToGoalString( pCourse->m_sGoalEqual[j], "=", pCourse->m_iGoalEqual[j] );

			m_sTextLines.push_back( sString );
		}

		if( k < pCourse->m_sGoalGreater.size() )
		{
			CString sString = ConvertToGoalString( pCourse->m_sGoalGreater[k], ">", pCourse->m_iGoalGreater[k] );

			m_sTextLines.push_back( sString );
		}
		
		m_iNumContents++;
	}

	// Set initial text
	for( int l=0; l<MAX_VISIBLE_MISSIONS; l++ ) 
		m_textMissionGoals[l].SetText( m_sTextLines[l] );
}

void MissionContentsList::Update( float fDeltaTime )
{
	ActorFrame::Update( fDeltaTime );

	if( m_fTimeUntilScroll > 0  &&  m_iNumContents > MAX_VISIBLE_MISSIONS)
		m_fTimeUntilScroll -= fDeltaTime;

	if( m_fTimeUntilScroll <= 0 )
	{
		// Don't scroll if all objectives have been shown!
		if( (m_iItemAtTopOfList + MAX_VISIBLE_MISSIONS) < m_iNumContents )
			m_iItemAtTopOfList++;

		for( int l=0; l<MAX_VISIBLE_MISSIONS; l++ ) 
			m_textMissionGoals[l].SetText( m_sTextLines[l+m_iItemAtTopOfList] );

		// Reset this!
		m_fTimeUntilScroll = 2;
	}
}

void MissionContentsList::DrawPrimitives()
{
	ActorFrame::DrawPrimitives();
}

void MissionContentsList::TweenInAfterChangedMission()
{
	m_iItemAtTopOfList = 0;
	m_fTimeUntilScroll = 2;
}

// Messy function. Ugh.
CString MissionContentsList::ConvertToGoalString( CString sTarget, CString sType, int iValue )
{
	CString sTypeReturn = "";
	CString sReturn = "";

	if( sType.CompareNoCase("<") == 0 )
		sTypeReturn = "less than";
	if( sType.CompareNoCase("=") == 0 )
		sTypeReturn = "exactly";
	if( sType.CompareNoCase("<") == 0 )
		sTypeReturn = "greater than";
	
	/*
	if( iValue == 1 )
	{
		if( sTarget.CompareNoCase("Hidden") == 0 )
			sReturn = ssprintf("Get %s one %s", sTypeReturn.c_str(), HIDDEN_LABEL);
		if( sTarget.CompareNoCase("Marvelous") == 0 )
			sReturn = ssprintf("Pass the song with %s one %s", sTypeReturn.c_str(), MARVELOUS_LABEL);
		if( sTarget.CompareNoCase("Perfect") == 0 )
			sReturn = ssprintf("Pass the song with %s one %s", sTypeReturn.c_str(), PERFECT_LABEL);
		if( sTarget.CompareNoCase("Great") == 0 )
			sReturn = ssprintf("Pass the song with %s one %s", sTypeReturn.c_str(), GREAT_LABEL);
		if( sTarget.CompareNoCase("Good") == 0 )
			sReturn = ssprintf("Pass the song with %s one %s", sTypeReturn.c_str(), GOOD_LABEL);
		if( sTarget.CompareNoCase("Boo") == 0 )
			sReturn = ssprintf("Pass the song with %s one %s", sTypeReturn.c_str(), BOO_LABEL);
		if( sTarget.CompareNoCase("Miss") == 0 )
			sReturn = ssprintf("Pass the song with %s one %s", sTypeReturn.c_str(), MISS_LABEL);
		if( sTarget.CompareNoCase("HitMine") == 0 )
			sReturn = ssprintf("Pass the song with %s one %s being hit", sTypeReturn.c_str(), HIT_MINE_LABEL);

		if( sTarget.CompareNoCase("HoldOK") == 0 )
			sReturn = ssprintf("Pass the song with %s one %s", sTypeReturn.c_str(), HOLD_OK_LABEL);
		if( sTarget.CompareNoCase("HoldNG") == 0 )
			sReturn = ssprintf("Pass the song with %s one %s", sTypeReturn.c_str(), HOLD_NG_LABEL);

		if( sTarget.CompareNoCase("RollOK") == 0 )
			sReturn = ssprintf("Pass the song with %s one %s", sTypeReturn.c_str(), ROLL_OK_LABEL);
		if( sTarget.CompareNoCase("RollNG") == 0 )
			sReturn = ssprintf("Pass the song with %s one %s", sTypeReturn.c_str(), ROLL_OK_LABEL);
	}
	else
	{
		if( sTarget.CompareNoCase("Hidden") == 0 )
			sReturn = ssprintf("Get %s %d %ss", sTypeReturn.c_str(), iValue, HIDDEN_LABEL);
		if( sTarget.CompareNoCase("Marvelous") == 0 )
			sReturn = ssprintf("Pass the song with %s %d %s'", sTypeReturn.c_str(), iValue, MARVELOUS_LABEL);
		if( sTarget.CompareNoCase("Perfect") == 0 )
			sReturn = ssprintf("Pass the song with %s %d %ss", sTypeReturn.c_str(), iValue, PERFECT_LABEL);
		if( sTarget.CompareNoCase("Great") == 0 )
			sReturn = ssprintf("Pass the song with %s %d %ss", sTypeReturn.c_str(), iValue, GREAT_LABEL);
		if( sTarget.CompareNoCase("Good") == 0 )
			sReturn = ssprintf("Pass the song with %s %d %ss", sTypeReturn.c_str(), iValue, GOOD_LABEL);
		if( sTarget.CompareNoCase("Boo") == 0 )
			sReturn = ssprintf("Pass the song with %s %d %ss", sTypeReturn.c_str(), iValue, BOO_LABEL);
		if( sTarget.CompareNoCase("Miss") == 0 )
			sReturn = ssprintf("Pass the song with %s %d %ses", sTypeReturn.c_str(), iValue, MISS_LABEL);
		if( sTarget.CompareNoCase("HitMine") == 0 )
			sReturn = ssprintf("Pass the song with %s %d %ses being hit", sTypeReturn.c_str(), iValue, HIT_MINE_LABEL);

		if( sTarget.CompareNoCase("HoldOK") == 0 )
			sReturn = ssprintf("Pass the song with %s %d %s's", sTypeReturn.c_str(), iValue, HOLD_OK_LABEL);
		if( sTarget.CompareNoCase("HoldNG") == 0 )
			sReturn = ssprintf("Pass the song with %s %d %s's", sTypeReturn.c_str(), iValue, HOLD_NG_LABEL);

		if( sTarget.CompareNoCase("RollOK") == 0 )
			sReturn = ssprintf("Pass the song with %s %d %s's", sTypeReturn.c_str(), iValue, ROLL_OK_LABEL);
		if( sTarget.CompareNoCase("RollNG") == 0 )
			sReturn = ssprintf("Pass the song with %s %d %s's", sTypeReturn.c_str(), iValue, ROLL_OK_LABEL);
	}

	if( sTarget.CompareNoCase("Score") == 0 )
		sReturn = ssprintf("Pass the song with a %s of %s %d", SCORE_LABEL, sTypeReturn.c_str(), iValue);

	if( sTarget.CompareNoCase("MaxCombo") == 0 )
		sReturn = ssprintf("Pass the song with a %s of %s %d", MAX_COMBO_LABEL, sTypeReturn.c_str(), iValue);

	*/

	return sReturn;
}

/*
 * (c) 2008 Mike Hawkins
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
