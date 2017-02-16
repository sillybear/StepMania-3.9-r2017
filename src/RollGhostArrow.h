/* RollGhostArrow - Graphic shown while Rolling a RollNote. */

#ifndef ROLLGHOSTARROW_H
#define ROLLGHOSTARROW_H

#include "Sprite.h"

class RollGhostArrow : public Sprite
{
public:
	RollGhostArrow();

	virtual void Load( CString sNoteSkin, CString sButton, CString sElement );

	virtual void Update( float fDeltaTime );
	virtual bool EarlyAbortDraw() { return !m_bRollIsActive; }

	void  SetRollIsActive( bool bRollIsActive ) { m_bRollIsActive = bRollIsActive; }

private:
	bool m_bRollIsActive;
};


#endif

/*
 * (c) 2007 Mike Hawkins (made from HoldGhostArrow.h, (c) 2001-2004 Ben Nordstrom, Chris Danford)
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
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT RollER OR RollERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
