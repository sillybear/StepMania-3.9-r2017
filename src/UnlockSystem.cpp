#include "global.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "Song.h"
#include "Difficulty.h"
#include "GameManager.h"
#include "Course.h"
#include "RageUtil.h"
#include "UnlockSystem.h"
#include "SongManager.h"
#include "GameState.h"
#include "MsdFile.h"
#include "ProfileManager.h"

UnlockSystem*	UNLOCKMAN = NULL;	// global and accessable from anywhere in our program

#define UNLOCKS_PATH "Data/Unlocks.dat"

static const char *g_UnlockNames[NUM_UNLOCK_TYPES] =
{
	"ArcadePointsAccumulated",
	"DancePointsAccumulated",
	"SongPointsAccumulated",
	"ExtraStagesCleared",
	"ExtraStagesFailed",
	"TotalToastysSeen",
	"TotalStagesCleared",
	"SpecificStageDifficultyCleared",
	"HiddenNotesHit"
};

UnlockSystem::UnlockSystem()
{
	UNLOCKMAN = this;

	Load();
}

void UnlockSystem::UnlockSong( const Song *song )
{
	const UnlockEntry *p = FindSong( song );
	if( !p )
		return;  // does not exist
	if( p->m_iCode == -1 )
		return;

	UnlockCode( p->m_iCode );
}

bool UnlockSystem::CourseIsLocked( const Course *course ) const
{
	if( !PREFSMAN->m_bUseUnlockSystem )
		return false;

	const UnlockEntry *p = FindCourse( course );
	if( p == NULL )
		return false;

	return p->IsLocked();
}

bool UnlockSystem::SongIsLocked( const Song *song ) const
{
	if( !PREFSMAN->m_bUseUnlockSystem )
		return false;

	const UnlockEntry *p = FindSong( song );
	if( p == NULL )
		return false;

	// Make sure we don't lock the song when trying to lock steps!
	if( stricmp(p->m_sItemUnlock,"song") != 0 )
		return false;

	return p->IsLocked();
}

bool UnlockSystem::StepsAreLocked( const Steps *steps ) const
{
	if( !PREFSMAN->m_bUseUnlockSystem )
		return false;

	// Catalog.xml really doesn't like what this function is doing in hiding the steps. Besides, do we
	// want to hide the steps from Catalog.xml?
	if( SONGMAN->m_bWritingCatalogXML )
		return false;

	const UnlockEntry *p = FindSteps( steps );
	if( p == NULL )
		return false;

	return p->IsLocked();
}

// Return true if the song is *only* available in roulette.
bool UnlockSystem::SongIsRouletteOnly( const Song *song ) const
{
	if( !PREFSMAN->m_bUseUnlockSystem )
		return false;

	const UnlockEntry *p = FindSong( song );
	if( !p )
		return false;

	// If the song is locked by a code, and it's a roulette code, honor IsLocked.
	if( p->m_iCode == -1 || m_RouletteCodes.find( p->m_iCode ) == m_RouletteCodes.end() )
		return false;

	return p->IsLocked();
}

const UnlockEntry *UnlockSystem::FindLockEntry( CString songname ) const
{
	for( unsigned i = 0; i < m_LockedEntries.size(); i++ )
	{
		if( songname.CompareNoCase(m_LockedEntries[i].m_sSongName) == 0 )
			return &m_LockedEntries[i];

		if( songname.CompareNoCase(m_LockedEntries[i].m_sCourseName) == 0 )
			return &m_LockedEntries[i];
	}

	return NULL;
}

const UnlockEntry *UnlockSystem::FindSong( const Song *pSong ) const
{
	for( unsigned i = 0; i < m_LockedEntries.size(); i++ )
		if( m_LockedEntries[i].m_pSong == pSong )
			return &m_LockedEntries[i];

	return NULL;
}

const UnlockEntry *UnlockSystem::FindSteps( const Steps *pSteps ) const
{
	for( unsigned i = 0; i < m_LockedEntries.size(); i++ )
		if( m_LockedEntries[i].m_pSteps == pSteps )
			return &m_LockedEntries[i];

	return NULL;
}

const UnlockEntry *UnlockSystem::FindCourse( const Course *pCourse ) const
{
	for(unsigned i = 0; i < m_LockedEntries.size(); i++)
		if (m_LockedEntries[i].m_pCourse == pCourse )
			return &m_LockedEntries[i];

	return NULL;
}

UnlockEntry::UnlockEntry()
{
	memset( m_fRequired, 0, sizeof(m_fRequired) );
	m_iCode = -1;

	m_pSong = NULL;
	m_pSteps = NULL;
	m_pCourse = NULL;
}

static float GetArcadePoints( const Profile *pProfile )
{
	float fAP =	0;

	FOREACH_Grade(g)
	{
		switch(g)
		{
		case GRADE_TIER_1:
		case GRADE_TIER_2:	fAP += 9 * pProfile->m_iNumStagesPassedByGrade[g]; break;
		default:			fAP += 1 * pProfile->m_iNumStagesPassedByGrade[g]; break;

		case GRADE_FAILED:
		case GRADE_NO_DATA:
			;	// no points
			break;
		}
	}

	FOREACH_PlayMode(pm)
	{
		switch(pm)
		{
		case PLAY_MODE_NONSTOP:
		case PLAY_MODE_ONI:
		case PLAY_MODE_ENDLESS:
			fAP += pProfile->m_iNumSongsPlayedByPlayMode[pm];
			break;
		}

	}

	return fAP;
}

static float GetSongPoints( const Profile *pProfile )
{
	float fSP =	0;

	FOREACH_Grade(g)
	{
		switch( g )
		{
		case GRADE_TIER_1:/*AAAA*/	fSP += 20 * pProfile->m_iNumStagesPassedByGrade[g];	break;
		case GRADE_TIER_2:/*AAA*/	fSP += 10* pProfile->m_iNumStagesPassedByGrade[g];	break;
		case GRADE_TIER_3:/*AA*/	fSP += 5* pProfile->m_iNumStagesPassedByGrade[g];	break;
		case GRADE_TIER_4:/*A*/		fSP += 4* pProfile->m_iNumStagesPassedByGrade[g];	break;
		case GRADE_TIER_5:/*B*/		fSP += 3* pProfile->m_iNumStagesPassedByGrade[g];	break;
		case GRADE_TIER_6:/*C*/		fSP += 2* pProfile->m_iNumStagesPassedByGrade[g];	break;
		case GRADE_TIER_7:/*D*/		fSP += 1* pProfile->m_iNumStagesPassedByGrade[g];	break;
		case GRADE_FAILED:
		case GRADE_NO_DATA:
			;	// no points
			break;
		}
	}

	FOREACH_PlayMode(pm)
	{
		switch(pm)
		{
		case PLAY_MODE_NONSTOP:
		case PLAY_MODE_ONI:
		case PLAY_MODE_ENDLESS:
			fSP += pProfile->m_iNumSongsPlayedByPlayMode[pm];
			break;
		}

	}

	return fSP;
}

void UnlockSystem::GetPoints( const Profile *pProfile, float fScores[NUM_UNLOCK_TYPES] ) const
{
	fScores[UNLOCK_ARCADE_POINTS] = GetArcadePoints( pProfile );
	fScores[UNLOCK_SONG_POINTS] = GetSongPoints( pProfile );
	fScores[UNLOCK_DANCE_POINTS] = (float) pProfile->m_iTotalDancePoints;
	fScores[UNLOCK_EXTRA_CLEARED] = (float) pProfile->m_iNumExtraStagesPassed;
	fScores[UNLOCK_EXTRA_FAILED] = (float) pProfile->m_iNumExtraStagesFailed;
	fScores[UNLOCK_TOASTY] = (float) pProfile->m_iNumToasties;
	fScores[UNLOCK_CLEARED] = (float) pProfile->GetTotalNumSongsPassed();
	fScores[UNLOCK_HIDDEN_NOTES] = (float) pProfile->m_iTotalHidden;
	fScores[UNLOCK_STAGE_CLEARED] = 0.0f;	// This will be taken care of later
}

static float IsStageCleared( const Profile *pProfile, const CString sSong, const CString sType,
								const CString sDifficulty, const CString sGrade )
{
	// Convert the strings into usable data
	Song *pSong = SONGMAN->FindSong( sSong );

	StepsType pStepsType = GAMEMAN->StringToStepsType(sType);
	const Style *pStyle = GAMEMAN->GetStyleForStepsType( pStepsType );

	Difficulty pDiff = StringToDifficulty(sDifficulty);

	// Check to see if the steps exist
	Steps *pSteps = pSong->GetStepsByDifficulty( pStepsType, pDiff, false );

	if( pSteps == NULL )
		return 1.0f;	// They didn't exist, so we're going to force this to unlock to avoid problems

	// Match required grade to a grade tier
	Grade gRequired = StringToGrade( sGrade );

	int iRequiredValue = 0;

	// This converts the grade tier into an int for us
	for( int i=GRADE_TIER_1; i<=gRequired; i++ )
		iRequiredValue++;

	// Look up high score information
	CString sFoundGrade = GradeToString( PROFILEMAN->GetHighScoreForDifficultyUnlock( pSong, pStyle, PROFILE_SLOT_MACHINE, pDiff ).grade );
	CStringArray sSplitValue;
	int iGradeTier = 0;

	split(sFoundGrade,"Tier",sSplitValue,false);

	// Convert to an int
	if( sSplitValue.size() == 1 )
		iGradeTier = atoi( sSplitValue[0] );
	else
		iGradeTier = atoi( sSplitValue[1] );

	// Check to see if there is a high-score that meets the requirement
	// <=, since the better the grade, the lower the tier value
	// However, if there is no grade, then we'll get zero... so account for that!
	if( iGradeTier <= iRequiredValue && iGradeTier > 0 )
		return 1.0f;
	
	// Requirement not met
	return 0.0f;
}

bool UnlockEntry::IsLocked() const
{
	bool bIsLocked = true;

	int iProfilesToCheck = 0;
	int iCurrentProfileIndex = 0;

	float fScores[NUM_UNLOCK_TYPES];

	Profile * m_Profile = NULL;

	if( PREFSMAN->m_bUnlockUsesMachineProfileStats )
		iProfilesToCheck++;
	else
		iCurrentProfileIndex = 1;	// Skip the machine profile

	if( PREFSMAN->m_bUnlockUsesPlayerProfileStats )
	{
		FOREACH_PlayerNumber( pn )
		{
			if( PROFILEMAN->IsUsingProfile(pn) && GAMESTATE->IsPlayerEnabled(pn) )
				iProfilesToCheck++;
		}
	}
	else
		iProfilesToCheck = 1;	// Cut out before the player profiles

	// If we're not checking any profiles, then this is never run!
	for( ; iCurrentProfileIndex < iProfilesToCheck; iCurrentProfileIndex++ )
	{
		if( iCurrentProfileIndex == 0 )
			m_Profile = PROFILEMAN->GetMachineProfile();
		else if( iCurrentProfileIndex == 1 )
		{
			if( GAMESTATE->IsPlayerEnabled(PLAYER_1) && PROFILEMAN->IsUsingProfile(PLAYER_1) )
				m_Profile = PROFILEMAN->GetProfile( PLAYER_1 );
			else
				continue;		// Don't bother checking this player
		}
		else if( iCurrentProfileIndex == 2 )
		{
			if( GAMESTATE->IsPlayerEnabled(PLAYER_2) && PROFILEMAN->IsUsingProfile(PLAYER_2) )
				m_Profile = PROFILEMAN->GetProfile( PLAYER_2 );
			else
				continue;		// Don't bother checking this player
		}

		UNLOCKMAN->GetPoints( m_Profile, fScores );

		for( int i = 0; i < NUM_UNLOCK_TYPES; ++i )
		{
			if( i == UNLOCK_STAGE_CLEARED && m_fRequired[i] == 1.0f )	// This unlock has it's own special case
			{
				fScores[i] = IsStageCleared( m_Profile, m_sRequiredSongName, m_sRequiredStepType,
									m_sRequiredStepDifficulty, m_sRequiredGrade );
			}

			if( m_fRequired[i] && fScores[i] >= m_fRequired[i] )
				bIsLocked = false;
		}

		if( m_iCode != -1 && m_Profile->m_UnlockedSongs.find(m_iCode) != m_Profile->m_UnlockedSongs.end() )
			bIsLocked = false;

		if( m_iCode != -1 && m_Profile->m_UnlockedSteps.find(m_iCode) != m_Profile->m_UnlockedSteps.end() )
			bIsLocked = false;

		if( m_iCode != -1 && m_Profile->m_UnlockedCourses.find(m_iCode) != m_Profile->m_UnlockedCourses.end() )
			bIsLocked = false;

		// If we had a problem with an entry, then it'll be force unlocked here
		if( m_bItemForcedUnlocked )
			bIsLocked = false;

		// If it's unlocked, then just break out already
		if( !bIsLocked )
			break;
	}

	return bIsLocked;
}

static const char *g_ShortUnlockNames[NUM_UNLOCK_TYPES] =
{
	"AP",
	"DP",
	"SP",
	"EC",
	"EF",
	"!!",
	"CS",
	"SD",
	"HN",
};

static UnlockType StringToUnlockType( const CString &s )
{
	for( int i = 0; i < NUM_UNLOCK_TYPES; ++i )
		if( g_ShortUnlockNames[i] == s )
			return (UnlockType) i;
	return UNLOCK_INVALID;
}

bool UnlockSystem::Load()
{
	LOG->Trace( "UnlockSystem::Load()" );
	
	if( !IsAFile(UNLOCKS_PATH) )
		return false;

	MsdFile msd;
	if( !msd.ReadFile( UNLOCKS_PATH ) )
	{
		LOG->Warn( "Error opening file '%s' for reading: %s.", UNLOCKS_PATH, msd.GetError().c_str() );
		return false;
	}

	unsigned i;

	bool bIgnoreEntry = false;

	for( i=0; i<msd.GetNumValues(); i++ )
	{
		bIgnoreEntry = false;

		int iNumParams = msd.GetNumParams(i);
		const MsdFile::value_t &sParams = msd.GetValue(i);
		CString sValueName = sParams[0];

		if( iNumParams < 1 )
		{
			LOG->Warn("Got \"%s\" unlock tag with no parameters", sValueName.c_str());
			continue;
		}

		if( stricmp(sParams[0],"ROULETTE") == 0 )
		{
			for( unsigned j = 1; j < sParams.params.size(); ++j )
				m_RouletteCodes.insert( atoi(sParams[j]) );
			continue;
		}
		
		if( stricmp(sParams[0],"UNLOCK") != 0 )
		{
			LOG->Warn("Unrecognized unlock tag \"%s\", ignored.", sValueName.c_str());
			continue;
		}

		UnlockEntry current;

		// Blank out values before loading in
		current.m_sItemUnlock = "";
		current.m_sSongName = "";
		current.m_sStepType = "";
		current.m_sStepDifficulty = "";
		current.m_sCourseName = "";
		current.m_sModifierName = "";

		sParams[1].MakeLower();
		current.m_sItemUnlock = sParams[1];

		if( stricmp(current.m_sItemUnlock, "song") == 0 )
			current.m_sSongName = sParams[2];
		else if( stricmp(current.m_sItemUnlock, "steps") == 0 )
		{
			CStringArray sSongParams;
			split(sParams[2],",",sSongParams, true);

			if( sSongParams.size() != 3 )
			{
				LOG->Warn( "Invalid locked steps entry ignored due to bad parameters: '%s'", sParams[2].c_str() );
				bIgnoreEntry = true;
			}
			else
			{
				current.m_sSongName = sSongParams[0];

				current.m_sStepType = sSongParams[1];
				current.m_sStepType.MakeLower();

				if( stricmp(current.m_sStepType,"") == 0)
				{
					LOG->Warn( "Invalid locked steps entry ignored due to invalid step type: '%s'", sParams[2].c_str() );
					bIgnoreEntry = true;
				}

				sSongParams[2].MakeLower();

				if(	sSongParams[2] == "beginner" )		current.m_sStepDifficulty = "beginner";
				else if( sSongParams[2] == "easy" )		current.m_sStepDifficulty = "easy";
				else if( sSongParams[2] == "basic" )		current.m_sStepDifficulty = "easy";
				else if( sSongParams[2] == "light" )		current.m_sStepDifficulty = "easy";
				else if( sSongParams[2] == "medium" )		current.m_sStepDifficulty = "medium";
				else if( sSongParams[2] == "another" )		current.m_sStepDifficulty = "medium";
				else if( sSongParams[2] == "trick" )		current.m_sStepDifficulty = "medium";
				else if( sSongParams[2] == "standard" )		current.m_sStepDifficulty = "medium";
				else if( sSongParams[2] == "difficult")		current.m_sStepDifficulty = "medium";
				else if( sSongParams[2] == "hard" )		current.m_sStepDifficulty = "hard";
				else if( sSongParams[2] == "ssr" )		current.m_sStepDifficulty = "hard";
				else if( sSongParams[2] == "maniac" )		current.m_sStepDifficulty = "hard";
				else if( sSongParams[2] == "heavy" )		current.m_sStepDifficulty = "hard";
				else if( sSongParams[2] == "smaniac" )		current.m_sStepDifficulty = "challenge";
				else if( sSongParams[2] == "challenge" )	current.m_sStepDifficulty = "challenge";
				else if( sSongParams[2] == "expert" )		current.m_sStepDifficulty = "challenge";
				else if( sSongParams[2] == "oni" )		current.m_sStepDifficulty = "challenge";
				else
				{
					LOG->Warn( "Invalid locked steps entry ignored due to invalid difficulty: '%s'", sParams[2].c_str() );
					bIgnoreEntry = true;
				}
			}
		}
		else if( stricmp(current.m_sItemUnlock, "course") == 0 )
			current.m_sCourseName = sParams[2];
		else
		{
			LOG->Warn( "Invalid lock type: '%s'", sParams[1].c_str() );
			bIgnoreEntry = true;
		}

		current.m_sRequiredSongName = "";
		current.m_sRequiredStepType = "";
		current.m_sRequiredStepDifficulty = "";

		current.m_sRequiredCourseName = "";
		current.m_sRequiredCourseType = "";
		current.m_sRequiredCourseDifficulty = "";

		current.m_sRequiredGrade = "";

		current.m_bItemForcedUnlocked = false;	// Only used for one unlock method, but still must set here

		CStringArray UnlockTypes;
		split( sParams[3], "|", UnlockTypes );

		for( unsigned j=0; j<UnlockTypes.size(); ++j )
		{
			CStringArray readparam;

			split( UnlockTypes[j], "=", readparam );
			const CString &unlock_type = readparam[0];

			LOG->Trace("UnlockTypes line: %s", UnlockTypes[j].c_str() );

			const float fVal = strtof( readparam[1], NULL );
			const int iVal = atoi( readparam[1] );

			const UnlockType ut = StringToUnlockType( unlock_type );

			if( ut != UNLOCK_INVALID && ut != UNLOCK_STAGE_CLEARED )
				current.m_fRequired[ut] = fVal;

			if( ut == UNLOCK_STAGE_CLEARED )
			{
				current.m_fRequired[ut] = 1.0f;

				CStringArray sSongParamsToUnlock;
				split(readparam[1],",",sSongParamsToUnlock, true);

				if( sSongParamsToUnlock.size() != 4 )
				{
					LOG->Warn( "Invalid unlock requirement: '%s'", sParams[3].c_str() );
					current.m_bItemForcedUnlocked = true;
				}
				else
				{
					current.m_sRequiredSongName = sSongParamsToUnlock[0];

					current.m_sRequiredStepType = sSongParamsToUnlock[1];
					current.m_sRequiredStepType.MakeLower();

					sSongParamsToUnlock[2].MakeLower();

					if(	sSongParamsToUnlock[2] == "beginner" )		current.m_sRequiredStepDifficulty = "beginner";
					else if( sSongParamsToUnlock[2] == "easy" )		current.m_sRequiredStepDifficulty = "easy";
					else if( sSongParamsToUnlock[2] == "basic" )		current.m_sRequiredStepDifficulty = "easy";
					else if( sSongParamsToUnlock[2] == "light" )		current.m_sRequiredStepDifficulty = "easy";
					else if( sSongParamsToUnlock[2] == "medium" )		current.m_sRequiredStepDifficulty = "medium";
					else if( sSongParamsToUnlock[2] == "another" )		current.m_sRequiredStepDifficulty = "medium";
					else if( sSongParamsToUnlock[2] == "trick" )		current.m_sRequiredStepDifficulty = "medium";
					else if( sSongParamsToUnlock[2] == "standard" )		current.m_sRequiredStepDifficulty = "medium";
					else if( sSongParamsToUnlock[2] == "difficult")		current.m_sRequiredStepDifficulty = "medium";
					else if( sSongParamsToUnlock[2] == "hard" )		current.m_sRequiredStepDifficulty = "hard";
					else if( sSongParamsToUnlock[2] == "ssr" )		current.m_sRequiredStepDifficulty = "hard";
					else if( sSongParamsToUnlock[2] == "maniac" )		current.m_sRequiredStepDifficulty = "hard";
					else if( sSongParamsToUnlock[2] == "heavy" )		current.m_sRequiredStepDifficulty = "hard";
					else if( sSongParamsToUnlock[2] == "smaniac" )		current.m_sRequiredStepDifficulty = "challenge";
					else if( sSongParamsToUnlock[2] == "challenge" )	current.m_sRequiredStepDifficulty = "challenge";
					else if( sSongParamsToUnlock[2] == "expert" )		current.m_sRequiredStepDifficulty = "challenge";
					else if( sSongParamsToUnlock[2] == "oni" )		current.m_sRequiredStepDifficulty = "challenge";
					else
					{
						LOG->Warn( "Invalid unlock requirement due to invalid difficulty: '%s'", sParams[3].c_str() );
						current.m_bItemForcedUnlocked = true;
					}

					current.m_sRequiredGrade = sSongParamsToUnlock[3];
					current.m_sRequiredGrade.MakeUpper();
				}
			}

			if( ut == UNLOCK_INVALID )
				bIgnoreEntry = true;

			if( unlock_type == "CODE" )
			{
				bIgnoreEntry = false;
				current.m_iCode = iVal;
			}
			if( unlock_type == "RO" )
			{
				bIgnoreEntry = false;
				current.m_iCode = iVal;
				m_RouletteCodes.insert( iVal );
			}
		}

		// If we flagged an entry for any reason, we're going to ignore it now by  not adding it to the vector
		if( !bIgnoreEntry )
			m_LockedEntries.push_back(current);
	}

	UpdateEntries();

	// This updates text
	for(i=0; i < m_LockedEntries.size(); i++)
	{
		CString str = "";

		if( stricmp(m_LockedEntries[i].m_sItemUnlock, "song") == 0 )
			str = ssprintf( "Unlock: %s; ", m_LockedEntries[i].m_sSongName.c_str() );
		else if( stricmp(m_LockedEntries[i].m_sItemUnlock, "steps") == 0 )
			str = ssprintf( "Unlock: %s, %s:%s; ", m_LockedEntries[i].m_sSongName.c_str(), m_LockedEntries[i].m_sStepType.c_str(), m_LockedEntries[i].m_sStepDifficulty.c_str() );
		else if( stricmp(m_LockedEntries[i].m_sItemUnlock, "course") == 0 )
			str = ssprintf( "Unlock: %s; ", m_LockedEntries[i].m_sCourseName.c_str() );

		for( int j = 0; j < NUM_UNLOCK_TYPES; ++j )
			if( m_LockedEntries[i].m_fRequired[j] )
				str += ssprintf( "%s = %f; ", g_UnlockNames[j], m_LockedEntries[i].m_fRequired[j] );

		str += ssprintf( "code = %i ", m_LockedEntries[i].m_iCode );

		str += m_LockedEntries[i].IsLocked()? "locked":"unlocked";

		if( m_LockedEntries[i].m_pSong )
			str += ( " (found song)" );
		if( m_LockedEntries[i].m_pSteps )
			str += ( " (found steps)" );
		if( m_LockedEntries[i].m_pCourse )
			str += ( " (found course)" );

		LOG->Trace( "%s", str.c_str() );
	}
	
	return true;
}

float UnlockSystem::PointsUntilNextUnlock( UnlockType t ) const
{
	float fScores[NUM_UNLOCK_TYPES];
	UNLOCKMAN->GetPoints( PROFILEMAN->GetMachineProfile(), fScores );

	float fSmallestPoints = 400000000;   // or an arbitrarily large value
	for( unsigned a=0; a<m_LockedEntries.size(); a++ )
		if( m_LockedEntries[a].m_fRequired[t] > fScores[t] )
			fSmallestPoints = min( fSmallestPoints, m_LockedEntries[a].m_fRequired[t] );
	
	if( fSmallestPoints == 400000000 )
		return 0;  // no match found
	return fSmallestPoints - fScores[t];
}

/* Update the song pointer.  Only call this when it's likely to have changed,
 * such as on load, or when a song title changes in the editor. */
void UnlockSystem::UpdateEntries()
{
	for( unsigned i = 0; i < m_LockedEntries.size(); ++i )
	{
		// Null values
		m_LockedEntries[i].m_pSong = NULL;
		m_LockedEntries[i].m_pSteps = NULL;
		m_LockedEntries[i].m_pCourse = NULL;

		m_LockedEntries[i].m_pSong = SONGMAN->FindSong( m_LockedEntries[i].m_sSongName );

		// Is a song, so see if we need steps locked with it!
		if( m_LockedEntries[i].m_pSong != NULL && stricmp(m_LockedEntries[i].m_sItemUnlock, "steps") == 0 )
		{
			StepsType steps = GAMEMAN->StringToStepsType(m_LockedEntries[i].m_sStepType);
			Difficulty diff = StringToDifficulty(m_LockedEntries[i].m_sStepDifficulty);

			m_LockedEntries[i].m_pSteps = m_LockedEntries[i].m_pSong->GetStepsByDifficulty( steps, diff, false );
		}

		// Not a song, so check to see if it's a course instead
		if( m_LockedEntries[i].m_pSong == NULL )
			m_LockedEntries[i].m_pCourse = SONGMAN->FindCourse( m_LockedEntries[i].m_sCourseName );

		// display warning on invalid song entry
		if( m_LockedEntries[i].m_pSong == NULL && stricmp(m_LockedEntries[i].m_sItemUnlock, "song") == 0  )
		{
			LOG->Warn("Unlock: Cannot find a matching entry for \"%s\"", m_LockedEntries[i].m_sSongName.c_str() );
			m_LockedEntries.erase(m_LockedEntries.begin() + i);
			--i;
		}
		// display warning on invalid steps entry
		else if( m_LockedEntries[i].m_pSteps == NULL && stricmp(m_LockedEntries[i].m_sItemUnlock, "steps") == 0 )
		{
			LOG->Warn("Unlock: Invalid steps for entry \"%s\"", m_LockedEntries[i].m_sSongName.c_str() );
			m_LockedEntries.erase(m_LockedEntries.begin() + i);
			--i;
		}
		// display warning on invalid course entry
		else if( m_LockedEntries[i].m_pCourse == NULL && stricmp(m_LockedEntries[i].m_sItemUnlock, "course") == 0 )
		{
			LOG->Warn("Unlock: Cannot find a matching entry for \"%s\"", m_LockedEntries[i].m_sCourseName.c_str() );
			m_LockedEntries.erase(m_LockedEntries.begin() + i);
			--i;
		}
	}
}



void UnlockSystem::UnlockCode( int num )
{
	for(unsigned i=0; i < m_LockedEntries.size(); i++)
	{
		if( stricmp(m_LockedEntries[i].m_sItemUnlock, "song") == 0 && m_LockedEntries[i].m_iCode == num )
		{
			FOREACH_PlayerNumber( pn )
				if( PROFILEMAN->IsUsingProfile(pn) )
					PROFILEMAN->GetProfile(pn)->m_UnlockedSongs.insert( num );

			PROFILEMAN->GetMachineProfile()->m_UnlockedSongs.insert( num );
		}

		if( stricmp(m_LockedEntries[i].m_sItemUnlock, "steps") == 0 && m_LockedEntries[i].m_iCode == num )
		{
			FOREACH_PlayerNumber( pn )
				if( PROFILEMAN->IsUsingProfile(pn) )
					PROFILEMAN->GetProfile(pn)->m_UnlockedSteps.insert( num );

			PROFILEMAN->GetMachineProfile()->m_UnlockedSteps.insert( num );
		}

		if( stricmp(m_LockedEntries[i].m_sItemUnlock, "course") == 0 && m_LockedEntries[i].m_iCode == num )
		{
			FOREACH_PlayerNumber( pn )
				if( PROFILEMAN->IsUsingProfile(pn) )
					PROFILEMAN->GetProfile(pn)->m_UnlockedCourses.insert( num );

			PROFILEMAN->GetMachineProfile()->m_UnlockedCourses.insert( num );
		}
	}
}

int UnlockSystem::GetNumUnlocks() const
{
	return m_LockedEntries.size();
}

/*
 * (c) 2001-2004 Kevin Slaughter, Andrew Wong, Glenn Maynard
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
