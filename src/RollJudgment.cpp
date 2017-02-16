#include "global.h"
#include "RollJudgment.h"
#include "RageUtil.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"
#include "ThemeManager.h"

CachedThemeMetric	ROLL_OK_COMMAND		("RollJudgment","OKCommand");
CachedThemeMetric	ROLL_NG_COMMAND		("RollJudgment","NGCommand");

CachedThemeMetric	ROLL_OK_ODD_COMMAND	("RollJudgment","OKOddCommand");
CachedThemeMetric	ROLL_NG_ODD_COMMAND	("RollJudgment","NGOddCommand");

CachedThemeMetric	ROLL_OK_EVEN_COMMAND	("RollJudgment","OKEvenCommand");
CachedThemeMetric	ROLL_NG_EVEN_COMMAND	("RollJudgment","NGEvenCommand");


RollJudgment::RollJudgment()
{
	ROLL_OK_COMMAND.Refresh();
	ROLL_NG_COMMAND.Refresh();

	ROLL_OK_ODD_COMMAND.Refresh();
	ROLL_NG_ODD_COMMAND.Refresh();

	ROLL_OK_EVEN_COMMAND.Refresh();
	ROLL_NG_EVEN_COMMAND.Refresh();

	m_iCount = 0;

	m_sprJudgment.Load( THEME->GetPathToG("HoldJudgment 1x2") );
	m_sprJudgment.StopAnimating();
	Reset();
	this->AddChild( &m_sprJudgment );
}

void RollJudgment::Update( float fDeltaTime )
{
	ActorFrame::Update( fDeltaTime );
}

void RollJudgment::DrawPrimitives()
{
	ActorFrame::DrawPrimitives();
}

void RollJudgment::Reset()
{
	m_sprJudgment.SetDiffuse( RageColor(1,1,1,0) );
	m_sprJudgment.SetXY( 0, 0 );
	m_sprJudgment.StopTweening();
	m_sprJudgment.SetEffectNone();
}

void RollJudgment::SetRollJudgment( RollNoteScore rns )
{
	//LOG->Trace( "Judgment::SetJudgment()" );

	Reset();

	switch( rns )
	{
	case RNS_NONE:
		ASSERT(0);
	case RNS_OK:
		m_sprJudgment.SetState( 0 );
		m_sprJudgment.Command( (m_iCount%2) ? ROLL_OK_ODD_COMMAND : ROLL_OK_EVEN_COMMAND );
		m_sprJudgment.Command( ROLL_OK_COMMAND );
		break;
	case RNS_NG:
		m_sprJudgment.SetState( 1 );
		m_sprJudgment.Command( (m_iCount%2) ? ROLL_NG_ODD_COMMAND : ROLL_NG_EVEN_COMMAND );
		m_sprJudgment.Command( ROLL_NG_COMMAND );
		break;
	default:
		ASSERT(0);
	}

	m_iCount++;
}

/*
 * (c) 2007 Mike Hawkins (made from HoldJudgment.cpp, (c) 2001-2004 Chris Danford)
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
