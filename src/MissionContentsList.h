/* MissionContentsList - Holds cmission objectives. */

#ifndef MISSION_CONTENTS_LIST_H
#define MISSION_CONTENTS_LIST_H

#include "BitmapText.h"
#include "ActorFrame.h"
#include "Sprite.h"
class Course;
class Song;
class Steps;


const int MAX_VISIBLE_MISSIONS = 8;
const int MAX_TOTAL_MISSIONS = 56;



class MissionContentsList : public ActorFrame
{
public:
	MissionContentsList();

	virtual void Update( float fDeltaTime );
	virtual void DrawPrimitives();

	void SetFromGameState();
	void TweenInAfterChangedMission();

	CString ConvertToGoalString( CString sTarget, CString sType, int iValue );

protected:

	int	m_iNumContents;
	vector<CString> m_sTextLines;

	BitmapText	m_textMissionGoals[MAX_VISIBLE_MISSIONS];

	float m_fTimeUntilScroll;
	int	  m_iItemAtTopOfList;	// between 0 and m_iNumContents
};

#endif

/*
 * (c) 22008 Mike Hawkins
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
