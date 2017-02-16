#include "global.h"
#include "Judgment.h"
#include "RageUtil.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"
#include "GameState.h"
#include "ThemeManager.h"
#include "Game.h"

#include "RageLog.h"

static CachedThemeMetric	MARVELOUS_COMMAND		("Judgment","MarvelousCommand");
static CachedThemeMetric	PERFECT_COMMAND			("Judgment","PerfectCommand");
static CachedThemeMetric	GREAT_COMMAND			("Judgment","GreatCommand");
static CachedThemeMetric	GOOD_COMMAND			("Judgment","GoodCommand");
static CachedThemeMetric	BOO_COMMAND			("Judgment","BooCommand");
static CachedThemeMetric	MISS_COMMAND			("Judgment","MissCommand");

static CachedThemeMetric	MARVELOUS_ODD_COMMAND		("Judgment","MarvelousOddCommand");
static CachedThemeMetric	PERFECT_ODD_COMMAND		("Judgment","PerfectOddCommand");
static CachedThemeMetric	GREAT_ODD_COMMAND		("Judgment","GreatOddCommand");
static CachedThemeMetric	GOOD_ODD_COMMAND		("Judgment","GoodOddCommand");
static CachedThemeMetric	BOO_ODD_COMMAND			("Judgment","BooOddCommand");
static CachedThemeMetric	MISS_ODD_COMMAND		("Judgment","MissOddCommand");

static CachedThemeMetric	MARVELOUS_EVEN_COMMAND		("Judgment","MarvelousEvenCommand");
static CachedThemeMetric	PERFECT_EVEN_COMMAND		("Judgment","PerfectEvenCommand");
static CachedThemeMetric	GREAT_EVEN_COMMAND		("Judgment","GreatEvenCommand");
static CachedThemeMetric	GOOD_EVEN_COMMAND		("Judgment","GoodEvenCommand");
static CachedThemeMetric	BOO_EVEN_COMMAND		("Judgment","BooEvenCommand");
static CachedThemeMetric	MISS_EVEN_COMMAND		("Judgment","MissEvenCommand");

static CachedThemeMetricB	USE_BEGINNER_LABEL		("Judgment","BeginnerHasSeparateJudgmentLabels");
static CachedThemeMetricB	USE_EASY_LABEL			("Judgment","EasyHasSeparateJudgmentLabels");
static CachedThemeMetricB	USE_MEDIUM_LABEL		("Judgment","MediumHasSeparateJudgmentLabels");
static CachedThemeMetricB	USE_HARD_LABEL			("Judgment","HardHasSeparateJudgmentLabels");
static CachedThemeMetricB	USE_CHALLENGE_LABEL		("Judgment","ChallengeHasSeparateJudgmentLabels");
static CachedThemeMetricB	USE_EDIT_LABEL			("Judgment","EditHasSeparateJudgmentLabels");
static CachedThemeMetricB	USE_EXTRA_LABEL			("Judgment","ExtraStageHasSeparateJudgmentLabels");


Judgment::Judgment()
{
	MARVELOUS_COMMAND.Refresh();
	PERFECT_COMMAND.Refresh();
	GREAT_COMMAND.Refresh();
	GOOD_COMMAND.Refresh();
	BOO_COMMAND.Refresh();
	MISS_COMMAND.Refresh();

	MARVELOUS_ODD_COMMAND.Refresh();
	PERFECT_ODD_COMMAND.Refresh();
	GREAT_ODD_COMMAND.Refresh();
	GOOD_ODD_COMMAND.Refresh();
	BOO_ODD_COMMAND.Refresh();
	MISS_ODD_COMMAND.Refresh();

	MARVELOUS_EVEN_COMMAND.Refresh();
	PERFECT_EVEN_COMMAND.Refresh();
	GREAT_EVEN_COMMAND.Refresh();
	GOOD_EVEN_COMMAND.Refresh();
	BOO_EVEN_COMMAND.Refresh();
	MISS_EVEN_COMMAND.Refresh();

	USE_BEGINNER_LABEL.Refresh();
	USE_EASY_LABEL.Refresh();
	USE_MEDIUM_LABEL.Refresh();
	USE_HARD_LABEL.Refresh();
	USE_CHALLENGE_LABEL.Refresh();
	USE_EDIT_LABEL.Refresh();
	USE_EXTRA_LABEL.Refresh();

	m_iCount = 0;

	//m_sprJudgment.Load( THEME->GetPathToG("Judgment 1x6") );

	// Load this here... may not do anything, but everything works with this still here!
	m_sprJudgment.Load( THEME->GetPathToG("Judgment") );
	ASSERT( m_sprJudgment.GetNumStates() == 6  ||  m_sprJudgment.GetNumStates() == 12 );

	m_sprJudgment.StopAnimating();
	Reset();
	this->AddChild( &m_sprJudgment );
}

void Judgment::Load( Difficulty dc )
{
	const CString sDifficulty = DifficultyToString( dc );
	CString sFoundLabel = "";
	bool bFoundLabel = false;

	if( USE_EXTRA_LABEL && (GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2()) )
	{
		sFoundLabel = m_sprJudgment.LoadJudgment( THEME->GetPathToG("JudgeLabelExtra", true) );

		if( stricmp(sFoundLabel,"") != 0 )
			bFoundLabel = true;

		// Judgment label wasn't found! Fallback!
		if( !bFoundLabel )
			m_sprJudgment.Load( THEME->GetPathToG("Judgment") );	// If it still isn't found, then you broke a default
	}
	else if( USE_BEGINNER_LABEL && sDifficulty.CompareNoCase("Beginner") == 0 )
	{
		sFoundLabel = m_sprJudgment.LoadJudgment( THEME->GetPathToG("JudgeLabelBeginner", true) );

		if( stricmp(sFoundLabel,"") != 0 )
			bFoundLabel = true;

		// Judgment label wasn't found! Fallback!
		if( !bFoundLabel )
			m_sprJudgment.Load( THEME->GetPathToG("Judgment") );	// If it still isn't found, then you broke a default
	}
	else if( USE_EASY_LABEL && sDifficulty.CompareNoCase("Easy") == 0 )
	{
		sFoundLabel = m_sprJudgment.LoadJudgment( THEME->GetPathToG("JudgeLabelEasy", true) );

		if( stricmp(sFoundLabel,"") != 0 )
			bFoundLabel = true;

		// Judgment label wasn't found! Fallback!
		if( !bFoundLabel )
			m_sprJudgment.Load( THEME->GetPathToG("Judgment") );	// If it still isn't found, then you broke a default
	}
	else if( USE_MEDIUM_LABEL && sDifficulty.CompareNoCase("Medium") == 0 )
	{
		sFoundLabel = m_sprJudgment.LoadJudgment( THEME->GetPathToG("JudgmentLabelMedium", true) );

		if( stricmp(sFoundLabel,"") != 0 )
			bFoundLabel = true;

		// Judgment label wasn't found! Fallback!
		if( !bFoundLabel )
			m_sprJudgment.Load( THEME->GetPathToG("Judgment") );	// If it still isn't found, then you broke a default
	}
	else if( USE_HARD_LABEL && sDifficulty.CompareNoCase("Hard") == 0 )
	{
		sFoundLabel = m_sprJudgment.LoadJudgment( THEME->GetPathToG("JudgeLabelHard", true) );

		if( stricmp(sFoundLabel,"") != 0 )
			bFoundLabel = true;

		// Judgment label wasn't found! Fallback!
		if( !bFoundLabel )
			m_sprJudgment.Load( THEME->GetPathToG("Judgment") );	// If it still isn't found, then you broke a default
	}
	else if( USE_CHALLENGE_LABEL && sDifficulty.CompareNoCase("Challenge") == 0 )
	{
		sFoundLabel = m_sprJudgment.LoadJudgment( THEME->GetPathToG("JudgeLabelChallenge", true) );

		if( stricmp(sFoundLabel,"") != 0 )
			bFoundLabel = true;

		// Judgment label wasn't found! Fallback!
		if( !bFoundLabel )
			m_sprJudgment.Load( THEME->GetPathToG("Judgment") );	// If it still isn't found, then you broke a default
	}
	else if( USE_EDIT_LABEL && sDifficulty.CompareNoCase("Edit") == 0 )
	{
		sFoundLabel = m_sprJudgment.LoadJudgment( THEME->GetPathToG("JudgeLabelEdit", true) );

		if( stricmp(sFoundLabel,"") != 0 )
			bFoundLabel = true;

		// Judgment label wasn't found! Fallback!
		if( !bFoundLabel )
			m_sprJudgment.Load( THEME->GetPathToG("Judgment") );	// If it still isn't found, then you broke a default
	}
	else
		m_sprJudgment.Load( THEME->GetPathToG("Judgment") );

	// Make sure we loaded up correctly here!
	ASSERT( m_sprJudgment.GetNumStates() == 6  ||  m_sprJudgment.GetNumStates() == 12 );
}

void Judgment::Reset()
{
	m_sprJudgment.FinishTweening();
	m_sprJudgment.SetXY( 0, 0 );
	m_sprJudgment.SetEffectNone();
	m_sprJudgment.SetHidden( true );
}

void Judgment::SetJudgment( TapNoteScore score, bool bEarly )
{
	//LOG->Trace( "Judgment::SetJudgment()" );

	Reset();

	if( score == TNS_HIDDEN )
	{
		if( GAMESTATE->ShowMarvelous() )
			score = TNS_MARVELOUS;
		else
			score = TNS_PERFECT;

		// Do game-specific score mapping.
		const Game* pGame = GAMESTATE->GetCurrentGame();
		if( score == TNS_MARVELOUS )	score = pGame->m_mapMarvelousTo;
		if( score == TNS_PERFECT )		score = pGame->m_mapPerfectTo;
	}

	m_sprJudgment.SetHidden( false );

	int iStateMult = (m_sprJudgment.GetNumStates()==12) ? 2 : 1;
	int iStateAdd = ( bEarly || ( iStateMult == 1 ) ) ? 0 : 1;

	switch( score )
	{
	case TNS_MARVELOUS:
		m_sprJudgment.SetState( 0 * iStateMult + iStateAdd );
		m_sprJudgment.Command( (m_iCount%2) ? MARVELOUS_ODD_COMMAND : MARVELOUS_EVEN_COMMAND );
		m_sprJudgment.Command( MARVELOUS_COMMAND );
		break;
	case TNS_PERFECT:
		m_sprJudgment.SetState( 1 * iStateMult + iStateAdd );
		m_sprJudgment.Command( (m_iCount%2) ? PERFECT_ODD_COMMAND : PERFECT_EVEN_COMMAND );
		m_sprJudgment.Command( PERFECT_COMMAND );
		break;
	case TNS_GREAT:
		m_sprJudgment.SetState( 2 * iStateMult + iStateAdd );
		m_sprJudgment.Command( (m_iCount%2) ? GREAT_ODD_COMMAND : GREAT_EVEN_COMMAND );
		m_sprJudgment.Command( GREAT_COMMAND );
		break;
	case TNS_GOOD:
		m_sprJudgment.SetState( 3 * iStateMult + iStateAdd );
		m_sprJudgment.Command( (m_iCount%2) ? GOOD_ODD_COMMAND : GOOD_EVEN_COMMAND );
		m_sprJudgment.Command( GOOD_COMMAND );
		break;
	case TNS_BOO:
		m_sprJudgment.SetState( 4 * iStateMult + iStateAdd );
		m_sprJudgment.Command( (m_iCount%2) ? BOO_ODD_COMMAND : BOO_EVEN_COMMAND );
		m_sprJudgment.Command( BOO_COMMAND );
		break;
	case TNS_MISS:
		m_sprJudgment.SetState( 5 * iStateMult + iStateAdd );
		m_sprJudgment.Command( (m_iCount%2) ? MISS_ODD_COMMAND : MISS_EVEN_COMMAND );
		m_sprJudgment.Command( MISS_COMMAND );
		break;
	case TNS_MISS_HIDDEN:
		break;
	default:
		ASSERT(0);
	}

	m_iCount++;
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
