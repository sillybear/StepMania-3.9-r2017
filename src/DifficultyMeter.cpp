#include "global.h"
#include "DifficultyMeter.h"
#include "RageUtil.h"
#include "GameConstantsAndTypes.h"
#include "GameState.h"
#include "PrefsManager.h"
#include "ThemeManager.h"
#include "Steps.h"
#include "Course.h"
#include "SongManager.h"
#include "ActorUtil.h"
#include "Style.h"


#define NUM_FEET_IN_METER				THEME->GetMetricI(m_sName,"NumFeetInMeter")
#define MAX_FEET_IN_METER				THEME->GetMetricI(m_sName,"MaxFeetInMeter")
#define GLOW_IF_METER_GREATER_THAN			THEME->GetMetricI(m_sName,"GlowIfMeterGreaterThan")
#define GLOW_IF_METER_GREATER_THAN_ITG			THEME->GetMetricI(m_sName,"GlowIfITGMeterGreaterThan")
#define GLOW_IF_METER_GREATER_THAN_X			THEME->GetMetricI(m_sName,"GlowIfXMeterGreaterThan")
#define SHOW_FEET					THEME->GetMetricB(m_sName,"ShowFeet")
/* "easy", "hard" */
#define SHOW_DIFFICULTY					THEME->GetMetricB(m_sName,"ShowDifficulty")
/* 3, 9 */
#define SHOW_METER					THEME->GetMetricB(m_sName,"ShowMeter")
#define FEET_IS_DIFFICULTY_COLOR			THEME->GetMetricB(m_sName,"FeetIsDifficultyColor")
#define FEET_PER_DIFFICULTY				THEME->GetMetricB(m_sName,"FeetPerDifficulty")

#define GENERATE_RANDOM_IF_HIDDEN			THEME->GetMetricB(m_sName,"GenRandomIfHiddenDifficulty")
#define HIDDEN_DIFFICULTY_STRING			THEME->GetMetric (m_sName,"HiddenDifficultyCharacters")

#define HIDE_AUTOGEN_DIFFICULTY				THEME->GetMetricB(m_sName,"AutogenUsesHiddenDifficulty")
	
DifficultyMeter::DifficultyMeter()
{
}

/* sID experiment:
 *
 * Names of an actor, "Foo":
 * [Foo]
 * Metric=abc
 *
 * [ScreenSomething]
 * FooP1X=20
 * FooP2Y=30
 *
 * Graphics\Foo under p1
 *
 * We want to call it different things in different contexts: we may only want one
 * set of internal metrics for a given use, but separate metrics for each player at
 * the screen level, and we may or may not want separate names at the asset level.
 *
 * As is, we tend to end up having to either duplicate [Foo] to [FooP1] and [FooP2]
 * or not use m_sName for [Foo], which limits its use.  Let's try using a separate
 * name for internal metrics.  I'm not sure if this will cause more confusion than good,
 * so I'm trying it first in only this object.
 */

void DifficultyMeter::Load()
{
	if( SHOW_FEET )
	{
		m_textFeet.SetName( "Feet" );
		CString Feet;
		if( FEET_PER_DIFFICULTY )
		{
			for( unsigned i = 0; i < NUM_DIFFICULTIES; ++i )
				Feet += char(i + '1'); // 01234
			Feet += '0'; // Off
		}
		else
			Feet = "0123";
		m_textFeet.LoadFromTextureAndChars( THEME->GetPathG(m_sName,"bar"), Feet );
		SET_XY_AND_ON_COMMAND( &m_textFeet );
		this->AddChild( &m_textFeet );
	}

	if( SHOW_DIFFICULTY )
	{
		m_Difficulty.Load( THEME->GetPathG(m_sName,"difficulty") );
		m_Difficulty->SetName( "Difficulty" );
		SET_XY_AND_ON_COMMAND( m_Difficulty );
		this->AddChild( m_Difficulty );
	}

	if( SHOW_METER )
	{
		m_textMeter.SetName( "Meter" );
		m_textMeter.LoadFromFont( THEME->GetPathF(m_sName,"meter") );
		SET_XY_AND_ON_COMMAND( m_textMeter );
		this->AddChild( &m_textMeter );
	}

	Unset();
}

void DifficultyMeter::SetFromGameState( PlayerNumber pn )
{
	if( GAMESTATE->IsCourseMode() )
	{
		const Trail* pTrail = GAMESTATE->m_pCurTrail[pn];
		if( pTrail )
			SetFromTrail( pTrail );
		else
			SetFromCourseDifficulty( GAMESTATE->m_PreferredCourseDifficulty[pn] );
	}
	else
	{
		const Steps* pSteps = GAMESTATE->m_pCurSteps[pn];
		if( pSteps )
			SetFromSteps( pSteps );
		else
			SetFromDifficulty( GAMESTATE->m_PreferredDifficulty[pn] );
	}
}

void DifficultyMeter::SetFromSteps( const Steps* pSteps )
{
	if( pSteps == NULL )
	{
		Unset();
		return;
	}

	if( pSteps->IsAutogen() )
	{

		if( HIDE_AUTOGEN_DIFFICULTY )
			SetFromMeterAndDifficulty( ssprintf("%i", pSteps->GetMeter()), pSteps->GetDifficulty(), pSteps->m_sMeterType, true );
		else
			SetFromMeterAndDifficulty( ssprintf("%i", pSteps->GetMeter()), pSteps->GetDifficulty(), pSteps->m_sMeterType, false );
	}
	else
		SetFromMeterAndDifficulty( ssprintf("%i", pSteps->GetMeter()), pSteps->GetDifficulty(), pSteps->m_sMeterType, pSteps->m_bHiddenDifficulty );

	SetDifficulty( DifficultyToString( pSteps->GetDifficulty() ) );
}

void DifficultyMeter::SetFromTrail( const Trail* pTrail )
{
	if( pTrail == NULL )
	{
		Unset();
		return;
	}

	SetFromMeterAndDifficulty( ssprintf("%i", pTrail->GetMeter()), pTrail->m_CourseDifficulty, "DDR", false );
	SetDifficulty( CourseDifficultyToString(pTrail->m_CourseDifficulty) + "Course" );
}

void DifficultyMeter::Unset()
{
	SetFromMeterAndDifficulty( "0", DIFFICULTY_BEGINNER, "DDR", false );
	SetDifficulty( "None" );
}

void DifficultyMeter::SetFromDifficulty( Difficulty dc )
{
	SetFromMeterAndDifficulty( "0", DIFFICULTY_BEGINNER, "DDR", false );
	SetDifficulty( DifficultyToString( dc ) );
}

void DifficultyMeter::SetFromCourseDifficulty( CourseDifficulty cd )
{
	SetFromMeterAndDifficulty( "0", DIFFICULTY_BEGINNER, "DDR", false );
	SetDifficulty( CourseDifficultyToString( cd ) + "Course" );
}

void DifficultyMeter::SetFromMeterAndDifficulty( CString sMeter, Difficulty dc, CString sMeterType, bool bHideDiff )
{
	if( SHOW_FEET )
	{
		int iMeter = atoi( sMeter );

		CString on = "2";
		CString off = "1";
		if( FEET_PER_DIFFICULTY )
			on = char(dc + '0');

		CString sNewText;
		int f;

		if( bHideDiff )
		{
			if( GENERATE_RANDOM_IF_HIDDEN )
				iMeter = (rand() % NUM_FEET_IN_METER ) + 1 ;
			else
				iMeter = 1;
		}

		for( f=0; f<NUM_FEET_IN_METER; f++ )
		{
			if( iMeter <= NUM_FEET_IN_METER )
				sNewText += (f<iMeter) ? "2" : (iMeter != 0) ? "1" : "0";
			else
				sNewText += (f<(iMeter%NUM_FEET_IN_METER)) ? "3" : "2";
		}

		m_textFeet.SetText( sNewText );

		// Don't glow if the difficulty is hidden!
		if( !bHideDiff )
		{
			if( iMeter > GLOW_IF_METER_GREATER_THAN && stricmp(sMeterType,"DDR")==0 )
				m_textFeet.SetEffectGlowShift();
			else if( iMeter > GLOW_IF_METER_GREATER_THAN_ITG && stricmp(sMeterType,"ITG")==0 )
				m_textFeet.SetEffectGlowShift();
			else if( iMeter > GLOW_IF_METER_GREATER_THAN_X && stricmp(sMeterType,"DDR X")==0 )
				m_textFeet.SetEffectGlowShift();
			else
				m_textFeet.SetEffectNone();
		}
		else
				m_textFeet.SetEffectNone();

		if( FEET_IS_DIFFICULTY_COLOR )
			m_textFeet.SetDiffuse( SONGMAN->GetDifficultyColor( dc ) );
	}

	if( SHOW_METER )
	{
		if( bHideDiff )
			m_textMeter.SetText( HIDDEN_DIFFICULTY_STRING );
		else if( sMeter.CompareNoCase( "0" ) == 0 )	// Unset calls with this
			m_textMeter.SetText( "x" );
		else if( sMeter.CompareNoCase( "?" ) == 0 )
			m_textMeter.SetText( sMeter );
		else
		{
			// We do this to remove any remaining garbage in the string
			int iMeter = atoi( sMeter );
			sMeter = ssprintf( "%i", iMeter );

			if( sMeter.CompareNoCase( "0" ) == 0 )
				m_textMeter.SetText( "x" );
			else
				m_textMeter.SetText( sMeter );
		}
	}
}

void DifficultyMeter::SetDifficulty( CString diff )
{
	if( m_CurDifficulty == diff )
		return;
	m_CurDifficulty = diff;

	if( SHOW_DIFFICULTY )
		COMMAND( m_Difficulty, "Set" + Capitalize(diff) );
	if( SHOW_METER )
		COMMAND( m_textMeter, "Set" + Capitalize(diff) );
}

/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard
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
