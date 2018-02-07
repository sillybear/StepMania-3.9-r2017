#include "global.h"
#include "NotesWriterSM.h"
#include "StepsUtil.h"
#include "Steps.h"
#include "RageUtil.h"
#include "GameManager.h"
#include "RageLog.h"
#include "RageFile.h"
#include "RageFileManager.h"
#include "Song.h"
#include "PrefsManager.h"
#include <cstring>
#include <cerrno>

static void write_tag( RageFile& f, CString const &format, CString const &value)
{
	if( !value.empty() )
		f.PutLine( ssprintf(format, SmEscape(value).c_str()) );
}

void NotesWriterSM::WriteGlobalTags( RageFile &f, const Song &out, bool bSavingCache )
{
	write_tag( f, "#TITLE:%s;", out.m_sMainTitle );
	write_tag( f, "#ARTIST:%s;", out.m_sArtist );
	write_tag( f, "#TITLETRANSLIT:%s;", out.m_sMainTitleTranslit );
	write_tag( f, "#SUBTITLETRANSLIT:%s;", out.m_sSubTitleTranslit );
	write_tag( f, "#ARTISTTRANSLIT:%s;", out.m_sArtistTranslit );

	if( bSavingCache || !PREFSMAN->m_bNoSavePlusFeatures )
		write_tag( f, "#GENRE:%s;", out.m_sGenre );

	write_tag( f, "#CREDIT:%s;", out.m_sCredit );

	if( bSavingCache || !PREFSMAN->m_bNoSavePlusFeatures )
	{
		write_tag( f, "#MENUCOLOR:%s;", out.m_sMenuColor );

		if( out.m_sMeterType != "DDR" )
			write_tag( f, "#METERTYPE:%s;", out.m_sMeterType );
	}

	write_tag( f, "#BANNER:%s;", out.m_sBannerFile );
	write_tag( f, "#BACKGROUND:%s;", out.m_sBackgroundFile );
	write_tag( f, "#LYRICSPATH:%s;", out.m_sLyricsFile );
	write_tag( f, "#CDTITLE:%s;", out.m_sCDTitleFile );
	write_tag( f, "#MUSIC:%s;", out.m_sMusicFile );

	if( out.m_Timing.m_fBeat0OffsetInSeconds != 0 )
		write_tag( f, "#OFFSET:%s;", FormatDouble("%.3f", out.m_Timing.m_fBeat0OffsetInSeconds) );

	write_tag( f, "#SAMPLESTART:%s;", FormatDouble("%.3f", out.m_fMusicSampleStartSeconds) );
	write_tag( f, "#SAMPLELENGTH:%s;", "15" );
	//write_tag( f, "#SAMPLELENGTH:%s;", FormatDouble("%.3f", out.m_fMusicSampleLengthSeconds) );

	switch(out.m_SelectionDisplay)
	{
		default: ASSERT(0);  /* fallthrough */
		case Song::SHOW_ALWAYS:
			break;
		case Song::SHOW_NEVER:
			f.Write( "#SELECTABLE:NO;" ); break;
		case Song::SHOW_ROULETTE:
			f.Write( "#SELECTABLE:ROULETTE;" ); break;
		case Song::SHOW_ES:
			f.Write( "#SELECTABLE:ES;" ); break;
		case Song::SHOW_OMES:
			f.Write( "#SELECTABLE:OMES;" ); break;
		case Song::SHOW_1:
			f.Write( "#SELECTABLE:1;" ); break;
		case Song::SHOW_2:
			f.Write( "#SELECTABLE:2;" ); break;
		case Song::SHOW_3:
			f.Write( "#SELECTABLE:3;" ); break;
		case Song::SHOW_4:
			f.Write( "#SELECTABLE:4;" ); break;
		case Song::SHOW_5:
			f.Write( "#SELECTABLE:5;" ); break;
		case Song::SHOW_6:
			f.Write( "#SELECTABLE:6;" ); break;
	}

	if( bSavingCache || !PREFSMAN->m_bNoSavePlusFeatures )
		write_tag( f, "#LISTSORT:%s;", out.m_sListSortPosition );

	switch( out.m_DisplayBPMType )
	{
		case Song::DISPLAY_ACTUAL:
			// write nothing
			break;
		case Song::DISPLAY_SPECIFIED:
			if( out.m_fSpecifiedBPMMin == out.m_fSpecifiedBPMMax )
				write_tag( f, "#DISPLAYBPM:%s;", FormatDouble("%.3f", out.m_fSpecifiedBPMMin).c_str() );
			else
				f.PutLine( ssprintf( "#DISPLAYBPM:%s:%s;", FormatDouble("%.3f", out.m_fSpecifiedBPMMin).c_str(),
					FormatDouble("%.3f", out.m_fSpecifiedBPMMax).c_str() ) );
			break;
		case Song::DISPLAY_RANDOM:
			f.PutLine( ssprintf( "#DISPLAYBPM:*;" ) );
			break;
	}

	vector<CString> bpmLines;
	for( unsigned i=0; i<out.m_Timing.m_BPMSegments.size(); i++ )
	{
		const BPMSegment &bs = out.m_Timing.m_BPMSegments[i];
		//bpmLines.push_back( ssprintf("%.3f=%.3f", bs.m_fStartBeat, bs.m_fBPM) );
		bpmLines.push_back( ssprintf("%s=%s", FormatDouble(bs.m_fStartBeat).c_str(), FormatDouble("%.3f", bs.m_fBPM).c_str()) );
	}
	write_tag( f, "#BPMS:%s;", join(",", bpmLines) );

	vector<CString> stopLines;
	for( unsigned i=0; i<out.m_Timing.m_StopSegments.size(); i++ )
	{
		const StopSegment &fs = out.m_Timing.m_StopSegments[i];
		//stopLines.push_back( ssprintf("%.3f=%.3f", fs.m_fStartBeat, fs.m_fStopSeconds) );
		stopLines.push_back( ssprintf("%s=%s", FormatDouble(fs.m_fStartBeat).c_str(), FormatDouble("%.3f", fs.m_fStopSeconds).c_str()) );
	}
	write_tag( f, "#STOPS:%s;", join(",", stopLines) );

	if( out.m_BackgroundChanges.size() > 1 || (!out.m_BackgroundChanges.empty() && out.m_BackgroundChanges[0].m_fStartBeat != 0) )
	{
		f.Write( "#BGCHANGES:" );
		for( unsigned i=0; i<out.m_BackgroundChanges.size(); i++ )
		{
			const BackgroundChange &seg = out.m_BackgroundChanges[i];

			f.Write( ssprintf( "%s=%s=%s=%d=%d=%d", FormatDouble(seg.m_fStartBeat).c_str(), seg.m_sBGName.c_str(), FormatDouble("%.3f", seg.m_fRate).c_str(), seg.m_bFadeLast, seg.m_bRewindMovie, seg.m_bLoop ) );

			if( i < out.m_BackgroundChanges.size()-1 )
				f.Write( "," );
		}
		f.Write( ";" );
		f.PutLine( "" );
	}

	if( out.m_ForegroundChanges.size() )
	{
		f.Write( "#FGCHANGES:" );
		for( unsigned i=0; i<out.m_ForegroundChanges.size(); i++ )
		{
			const BackgroundChange &seg = out.m_ForegroundChanges[i];

			f.PutLine( ssprintf( "%s=%s=%s=%d=%d=%d", FormatDouble(seg.m_fStartBeat).c_str(), seg.m_sBGName.c_str(), FormatDouble("%.3f", seg.m_fRate).c_str(), seg.m_bFadeLast, seg.m_bRewindMovie, seg.m_bLoop ) );
			if( i != out.m_ForegroundChanges.size()-1 )
				f.Write( "," );
		}
		f.PutLine( ";" );
	}

	if( bSavingCache || !PREFSMAN->m_bNoSavePlusFeatures )
	{
		if( out.m_sAttackString.size() )
		{
			f.Write( "#ATTACKS:" );
			int iRowCount = 0;

			for( unsigned i=0; i < out.m_sAttackString.size(); i++ )
			{
				CString sData = out.m_sAttackString[i];

				if( iRowCount == 2 )
					f.PutLine( ssprintf( "%s", sData.c_str() ) );
				else
					f.Write( ssprintf( "%s", sData.c_str() ) );

				if( i != (out.m_sAttackString.size() - 1) )
					f.Write( ":" );	// Not the end, so write in the divider ':'
			}
			f.Write( ";" );
			f.PutLine( "" );
		}
	}
}

static void WriteLineList( RageFile &f, vector<CString> &lines, bool SkipLeadingBlankLines, bool OmitLastNewline )
{
	for( unsigned i = 0; i < lines.size(); ++i )
	{
		TrimLeft( lines[i] );
		TrimRight( lines[i] );
		if( SkipLeadingBlankLines )
		{
			if( lines.size() == 0 )
				continue;
			SkipLeadingBlankLines = false;
		}
		f.Write( lines[i] );

		if( !OmitLastNewline || i+1 < lines.size() )
			f.PutLine( "" ); /* newline */
	}
}

void NotesWriterSM::WriteSMNotesTag( const Steps &in, RageFile &f, bool bSavingCache )
{
	f.PutLine( "" );
	f.PutLine( ssprintf( "//---------------%s - %s----------------",
		GameManager::StepsTypeToString(in.m_StepsType).c_str(), in.GetDescription().c_str() ) );
	f.PutLine( "#NOTES:" );
	f.PutLine( ssprintf( "     %s:", GameManager::StepsTypeToString(in.m_StepsType).c_str() ) );
	f.PutLine( ssprintf( "     %s:", in.GetDescription().c_str() ) );
	f.PutLine( ssprintf( "     %s:", DifficultyToString(in.GetDifficulty()).c_str() ) );

	if( in.m_bHiddenDifficulty )
	{
		if( !bSavingCache )
			f.PutLine( ssprintf( "     %d*:", in.GetOriginalMeter() ) );
		else
			f.PutLine( ssprintf( "     %d*:", in.GetMeter() ) );
	}
	else
	{
		if( !bSavingCache )
			f.PutLine( ssprintf( "     %d:", in.GetOriginalMeter() ) );
		else
			f.PutLine( ssprintf( "     %d:", in.GetMeter() ) );
	}

	int MaxRadar = bSavingCache? NUM_RADAR_CATEGORIES:5;
	CStringArray asRadarValues;
	for( int r=0; r < MaxRadar; r++ )
		asRadarValues.push_back( ssprintf("%s", FormatDouble("%.3f", in.GetRadarValues()[r]).c_str()) );
	/* Don't append a newline here; it's added in NoteDataUtil::GetSMNoteDataString.
	 * If we add it here, then every time we write unmodified data we'll add an extra
	 * newline and they'll accumulate. */
	f.Write( "     :" );
	//f.Write( ssprintf( "     %s:", join(",",asRadarValues).c_str() ) );

	f.PutLine( "" );

	CString sNoteData;
	CString sAttackData;
	in.GetSMNoteData( sNoteData, sAttackData );

	vector<CString> lines;
	TrimLeft( sNoteData );
	split( sNoteData, "\n", lines, false );
	WriteLineList( f, lines, true, true );

	if( sAttackData.empty() )
		f.PutLine( ";" );
	else
	{
		f.PutLine( ":" );

		lines.clear();
		split( sAttackData, "\n", lines, false );
		WriteLineList( f, lines, true, true );

		f.PutLine( ";" );
	}
}

bool NotesWriterSM::Write(CString sPath, const Song &out, bool bSavingCache )
{
	/* Flush dir cache when writing steps, so the old size isn't cached. */
	FILEMAN->FlushDirCache( Dirname(sPath) );

	unsigned i;

	int flags = RageFile::WRITE;

	/* If we're not saving cache, we're saving real data, so enable SLOW_FLUSH
	 * to prevent data loss.  If we're saving cache, this will slow things down
	 * too much. */
	if( !bSavingCache )
		flags |= RageFile::SLOW_FLUSH;

	RageFile f;
	if( !f.Open( sPath, flags ) )
	{
		LOG->Warn( "Error opening song file '%s' for writing: %s", sPath.c_str(), f.GetError().c_str() );
		return false;
	}

	WriteGlobalTags( f, out, bSavingCache );
	if( bSavingCache )
	{
		f.PutLine( "" );
		f.PutLine( ssprintf( "// cache tags:" ) );
		f.PutLine( ssprintf( "#FIRSTBEAT:%s;", FormatDouble("%.3f", out.m_fFirstBeat).c_str() ) );
		f.PutLine( ssprintf( "#LASTBEAT:%s;", FormatDouble("%.3f", out.m_fLastBeat).c_str() ) );
		f.PutLine( ssprintf( "#SONGFILENAME:%s;", out.m_sSongFileName.c_str() ) );
		f.PutLine( ssprintf( "#HASMUSIC:%i;", out.m_bHasMusic ) );
		f.PutLine( ssprintf( "#HASBANNER:%i;", out.m_bHasBanner ) );
		f.PutLine( ssprintf( "#MUSICLENGTH:%s;", FormatDouble("%.3f", out.m_fMusicLengthSeconds).c_str() ) );
		f.PutLine( ssprintf( "// end cache tags" ) );
	}

	//
	// Save all Steps for this file
	//
	vector<Steps*> vpSteps = out.GetAllSteps();
	StepsUtil::SortStepsByTypeAndDifficulty( vpSteps );

	for( i=0; i<vpSteps.size(); i++ ) 
	{
		const Steps* pSteps = vpSteps[i];
		if( pSteps->IsAutogen() )
			continue; /* don't write autogen notes */

		/* Only save steps that weren't loaded from a profile. */
		if( pSteps->WasLoadedFromProfile() )
			continue;

		WriteSMNotesTag( *pSteps, f, bSavingCache );
	}

	return true;
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
