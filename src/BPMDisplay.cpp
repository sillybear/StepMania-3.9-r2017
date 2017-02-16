#include "global.h"
#include "BPMDisplay.h"
#include "RageUtil.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"
#include "GameState.h"
#include "ThemeManager.h"
#include "Course.h"
#include "Style.h"

#define NORMAL_COLOR		THEME->GetMetricC(m_sName,"NormalColor")
#define CHANGE_COLOR		THEME->GetMetricC(m_sName,"ChangeColor")
#define RANDOM_COLOR		THEME->GetMetricC(m_sName,"RandomColor") // Color defn for randomly cycling BPMs
#define EXTRA_COLOR			THEME->GetMetricC(m_sName,"ExtraColor")
#define CYCLE				THEME->GetMetricB(m_sName,"Cycle")
#define SEPARATOR			THEME->GetMetric (m_sName,"Separator")
#define NO_BPM_TEXT			THEME->GetMetric (m_sName,"NoBPMText")
#define SHOW_QUESTION_MARK	THEME->GetMetricB(m_sName,"Show???")
#define SHOW_NEGATIVE		THEME->GetMetricB(m_sName,"ShowNegative")
#define USE_BPS				THEME->GetMetricB(m_sName,"ShowAsBPS")
#define BPM_RAND_CYCLE		THEME->GetMetric (m_sName,"RandomBPMCycleSpeed")

BPMDisplay::BPMDisplay()
{
	m_fBPMFrom = m_fBPMTo = 0;
	m_iCurrentBPM = 0;
	m_BPMS.push_back(0);
	m_fPercentInState = 0;
	m_fCycleTime = 1.0f;
}

void BPMDisplay::Load()
{
	m_textBPM.SetName( "Text" );
	m_textBPM.LoadFromFont( THEME->GetPathToF(m_sName) );
	SET_XY_AND_ON_COMMAND( m_textBPM );
	m_textBPM.SetDiffuse( NORMAL_COLOR );
	this->AddChild( &m_textBPM );

	m_sprLabel.Load( THEME->GetPathToG(ssprintf("%s label", m_sName.c_str())) );
	m_sprLabel->SetName( "Label" );
	SET_XY_AND_ON_COMMAND( m_sprLabel );
	m_sprLabel->SetDiffuse( NORMAL_COLOR );
	this->AddChild( m_sprLabel );
}

float BPMDisplay::GetActiveBPM() const
{
	return m_fBPMTo + (m_fBPMFrom-m_fBPMTo) * m_fPercentInState;
}

void BPMDisplay::Update( float fDeltaTime ) 
{ 
	ActorFrame::Update( fDeltaTime ); 

	if( !CYCLE )
		return;
	if( m_BPMS.size() == 0 )
		return; // no bpm

	m_fPercentInState -= fDeltaTime / m_fCycleTime;
	if( m_fPercentInState < 0 )
	{
		// go to next state
		m_fPercentInState = 1;		// reset timer

		m_iCurrentBPM = (m_iCurrentBPM + 1) % m_BPMS.size();
		m_fBPMFrom = m_fBPMTo;
		m_fBPMTo = m_BPMS[m_iCurrentBPM];

		if(m_fBPMTo == -1)
		{
			m_fBPMFrom = -1;
			if( SHOW_QUESTION_MARK )
				m_textBPM.SetText( (RandomFloat(0,1)>0.90) ? CString("xxx") : ssprintf("%03.0f",RandomFloat(0,999)) );
			else
				m_textBPM.SetText( ssprintf("%03.0f",RandomFloat(0,999)) );
		} 
		else if( m_fBPMFrom == -1 )
			m_fBPMFrom = m_fBPMTo;
	}

	// update m_textBPM
	if( m_fBPMTo != -1)
	{
		if( USE_BPS )
		{
			const float fActualBPS = GetActiveBPM()/60.0f;

			// If we have a negative BPM in display, just display '000' if we've set it in the theme to hide it
			if( fActualBPS < 0 && !SHOW_NEGATIVE )
				m_textBPM.SetText( ssprintf("000") );
			else
				m_textBPM.SetText( ssprintf("%03.0f", fActualBPS) );
		}
		else
		{
			const float fActualBPM = GetActiveBPM();

			// If we have a negative BPM in display, just display '000' if we've set it in the theme to hide it
			if( fActualBPM < 0 && !SHOW_NEGATIVE )
				m_textBPM.SetText( ssprintf("000") );
			else
				m_textBPM.SetText( ssprintf("%03.0f", fActualBPM) );
		}
	}
}


void BPMDisplay::SetBPMRange( const DisplayBpms &bpms )
{
	ASSERT( !bpms.vfBpms.empty() );

	const vector<float> &BPMS = bpms.vfBpms;

	unsigned i;
	bool AllIdentical = true;
	bool IsRandom = false;
	for( i = 0; i < BPMS.size(); ++i )
	{
		if( i > 0 && BPMS[i] != BPMS[i-1] )
			AllIdentical = false;
	}

	if( !CYCLE )
	{
		int MinBPM=99999999;
		int MaxBPM=-99999999;
		for( i = 0; i < BPMS.size(); ++i )
		{
			MinBPM = min( MinBPM, (int) roundf(BPMS[i]) );
			MaxBPM = max( MaxBPM, (int) roundf(BPMS[i]) );
		}
		if( MinBPM == MaxBPM )
		{
			if( MinBPM == -1 )
			{
				m_textBPM.SetText( "..." ); // random
				IsRandom = true;
			}
			else
				m_textBPM.SetText( ssprintf("%i", MinBPM) );
		}
		else
			m_textBPM.SetText( ssprintf("%i%s%i", MinBPM, SEPARATOR.c_str(), MaxBPM) );
	} 
	else 
	{
		m_BPMS.clear();
		for( i = 0; i < BPMS.size(); ++i )
		{
			m_BPMS.push_back(BPMS[i]);
			if( BPMS[i] != -1 )
				m_BPMS.push_back(BPMS[i]); /* hold */
		}

		m_iCurrentBPM = min(1u, m_BPMS.size()); /* start on the first hold */
		m_fBPMFrom = BPMS[0];
		m_fBPMTo = BPMS[0];
		m_fPercentInState = 1;
	}

	m_textBPM.StopTweening();
	m_sprLabel->StopTweening();
	m_textBPM.BeginTweening(0.5f);
	m_sprLabel->BeginTweening(0.5f);

	if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )
	{
		m_textBPM.SetDiffuse( EXTRA_COLOR );
		m_sprLabel->SetDiffuse( EXTRA_COLOR );
	}
	else if( !AllIdentical )
	{
		m_textBPM.SetDiffuse( CHANGE_COLOR );
		m_sprLabel->SetDiffuse( CHANGE_COLOR );
	}
	else
	{
		m_textBPM.SetDiffuse( NORMAL_COLOR );
		m_sprLabel->SetDiffuse( NORMAL_COLOR );
	}
}

void BPMDisplay::CycleRandomly()
{
	DisplayBpms bpms;
	bpms.Add(-1);
	SetBPMRange( bpms );

	m_textBPM.SetDiffuse( RANDOM_COLOR );
	m_sprLabel->SetDiffuse( RANDOM_COLOR );

	CString sValue = BPM_RAND_CYCLE;
	sValue.MakeLower();

	// Set the cycle speed
	if( stricmp(sValue, "turtle") == 0 )
		m_fCycleTime = 1.0f;	// I doubt this one will ever see any use
	else if( stricmp(sValue, "slow") == 0 )
		m_fCycleTime = 0.5f;
	else if( stricmp(sValue, "fast") == 0 )
		m_fCycleTime = 0.1f;
	else if( stricmp(sValue, "vfast") == 0 )
		m_fCycleTime = 0.05f;	// This is what DDR Extreme runs around
	else if( stricmp(sValue, "hyper") == 0 )
		m_fCycleTime = 0.01f;
	else if( stricmp(sValue, "overdrive") == 0 )
		m_fCycleTime = 0.005f;
	else if( stricmp(sValue, "insanity") == 0 )
		m_fCycleTime = 0.0001f;
	else if( stricmp(sValue, "retardation") == 0 )
		m_fCycleTime = 0.0005f;
	else if( stricmp(sValue, "omfg") == 0 )
		m_fCycleTime = 0.00001f;	// I actually added this one as a joke... - Mike
	else
		m_fCycleTime = 0.2f;
}

void BPMDisplay::NoBPM()
{
	m_BPMS.clear();
	m_textBPM.SetText( NO_BPM_TEXT ); 

	m_textBPM.SetDiffuse( NORMAL_COLOR );
	m_sprLabel->SetDiffuse( NORMAL_COLOR );
}

void BPMDisplay::SetBPM( const Song* pSong )
{
	ASSERT( pSong );
	switch( pSong->m_DisplayBPMType )
	{
	case Song::DISPLAY_ACTUAL:
	case Song::DISPLAY_SPECIFIED:
		{
			DisplayBpms bpms;
			pSong->GetDisplayBpms( bpms );
			SetBPMRange( bpms );
			m_fCycleTime = 1.0f;
		}
		break;
	case Song::DISPLAY_RANDOM:
		CycleRandomly();
		break;
	default:
		ASSERT(0);
	}
}

void BPMDisplay::SetBPM( const Course* pCourse )
{
	ASSERT( pCourse );

	StepsType st = GAMESTATE->GetCurrentStyle()->m_StepsType;
	Trail *pTrail = pCourse->GetTrail( st );
	ASSERT( pTrail );

	DisplayBpms bpms;
	pTrail->GetDisplayBpms( bpms );
	
	SetBPMRange( bpms );
	m_fCycleTime = 0.2f;
}

void BPMDisplay::DrawPrimitives()
{
	ActorFrame::DrawPrimitives();
}

/*
 * (c) 2001-2002 Chris Danford
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
