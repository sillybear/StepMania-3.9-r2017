#include "global.h"
#include "SongManager.h"
#include "IniFile.h"
#include "RageLog.h"
#include "MsdFile.h"
#include "NotesLoaderDWI.h"
#include "BannerCache.h"
#include "BackgroundCache.h"
#include "arch/arch.h"

#include "GameState.h"
#include "PrefsManager.h"
#include "RageException.h"
#include "arch/LoadingWindow/LoadingWindow.h"
#include "Course.h"

#include "AnnouncerManager.h"
#include "ThemeManager.h"
#include "GameManager.h"
#include "RageFile.h"
#include "RageTextureManager.h"
#include "Sprite.h"
#include "ProfileManager.h"
#include "MemoryCardManager.h"
#include "NotesLoaderSM.h"
#include "SongUtil.h"
#include "StepsUtil.h"
#include "CourseUtil.h"
#include "RageFileManager.h"
#include "UnlockSystem.h"
#include "CatalogXml.h"
#include "Foreach.h"
#include "StageStats.h"
#include "Style.h"

// Needed for tweaked Extra Stage code
#include "NoteSkinManager.h"

SongManager*	SONGMAN = NULL;	// global and accessable from anywhere in our program

#define SONGS_DIR				"Songs/"
#define COURSES_DIR				"Courses/"

#define ATTACKS_PATH				"Data/RandomAttacks.dat"

#define MAX_EDITS_PER_PROFILE			200
#define MAX_EDIT_SIZE_BYTES			30*1024		// 30KB

#define NUM_GROUP_COLORS			THEME->GetMetricI("SongManager","NumGroupColors")
#define GROUP_COLOR( i )			THEME->GetMetricC("SongManager",ssprintf("GroupColor%d",i+1))

CachedThemeMetricC BEGINNER_COLOR		("SongManager","BeginnerColor");
CachedThemeMetricC EASY_COLOR			("SongManager","EasyColor");
CachedThemeMetricC MEDIUM_COLOR			("SongManager","MediumColor");
CachedThemeMetricC HARD_COLOR			("SongManager","HardColor");
CachedThemeMetricC CHALLENGE_COLOR		("SongManager","ChallengeColor");
CachedThemeMetricC EDIT_COLOR			("SongManager","EditColor");
CachedThemeMetricC EXTRA_COLOR			("SongManager","ExtraColor");
CachedThemeMetricI EXTRA_COLOR_METER		("SongManager","ExtraColorMeter");
CachedThemeMetricI EXTRA_COLOR_METER_ITG	("SongManager","ExtraColorMeterITG");
CachedThemeMetricI EXTRA_COLOR_METER_X		("SongManager","ExtraColorMeterX");

vector<RageColor> g_vGroupColors;
RageTimer g_LastMetricUpdate; /* can't use RageTimer globally */

static void UpdateMetrics()
{
	if( !g_LastMetricUpdate.IsZero() && g_LastMetricUpdate.PeekDeltaTime() < 1 )
		return;

	g_LastMetricUpdate.Touch();
	g_vGroupColors.clear();
	for( int i=0; i<NUM_GROUP_COLORS; i++ )
		g_vGroupColors.push_back( GROUP_COLOR(i) );

	BEGINNER_COLOR.Refresh();
	EASY_COLOR.Refresh();
	MEDIUM_COLOR.Refresh();
	HARD_COLOR.Refresh();
	CHALLENGE_COLOR.Refresh();
	EDIT_COLOR.Refresh();
	EXTRA_COLOR.Refresh();
	EXTRA_COLOR_METER.Refresh();
	EXTRA_COLOR_METER_ITG.Refresh();
	EXTRA_COLOR_METER_X.Refresh();
}

SongManager::SongManager()
{
	g_LastMetricUpdate.SetZero();
	UpdateMetrics();

	m_bReloading = false;
	m_bWritingCatalogXML = false;
}

SongManager::~SongManager()
{
	// Courses depend on Songs and Songs don't depend on Courses.
	// So, delete the Courses first.
	FreeCourses();
	FreeSongs();
}

void SongManager::InitAll( LoadingWindow *ld )
{
	m_iHighestListSortValue = 0;

	InitSongsFromDisk( ld );
	InitCoursesFromDisk( ld );
	InitAutogenCourses();
	LoadListSortSongs( ld );

	// These aren't affected by fast load, so they won't need to be reloaded mid-game
	if( !m_bReloading )
		InitRandomAttacks( ld );

	// Save Catalog.xml if we allow it via preferences
	if( PREFSMAN->m_bCatalogXML )
	{
		m_bWritingCatalogXML = true;
		/* This shouldn't need to be here; if it's taking long enough that this is
		 * even visible, we should be fixing it, not showing a progress display. */
		if( ld )
			ld->SetText( "Saving Catalog.xml ..." );
	
		SaveCatalogXml();

		m_bWritingCatalogXML = false;
	}
}

void SongManager::Reload( LoadingWindow *ld )
{
	// We're reloading, so make sure we set this!
	m_bReloading = true;

	FlushDirCache();

	if( ld )
		ld->SetText( "Reloading ..." );

	// save scores before unloading songs, or the scores will be lost
	PROFILEMAN->SaveMachineProfile();

	FreeCourses();
	FreeSongs();

	/* Always check songs for changes. */
	const bool OldVal = PREFSMAN->m_bFastLoad;
	PREFSMAN->m_bFastLoad = false;

	InitAll( ld );

	// reload scores afterward
	PROFILEMAN->LoadMachineProfile();

	PREFSMAN->m_bFastLoad = OldVal;

	// No longer reloading, so change this back to false
	m_bReloading = false;
}

void SongManager::InitSongsFromDisk( LoadingWindow *ld )
{
	RageTimer tm;
	LoadStepManiaSongDir( SONGS_DIR, ld );
	LOG->Trace( "Found %d songs in %f seconds.", (int)m_pSongs.size(), tm.GetDeltaTime() );
}

void SongManager::SanityCheckGroupDir( CString sDir ) const
{
	// Check to see if they put a song directly inside the group folder.
	CStringArray arrayFiles;
	GetDirListing( sDir + "/*.mp3", arrayFiles );
	GetDirListing( sDir + "/*.ogg", arrayFiles );
	GetDirListing( sDir + "/*.wav", arrayFiles );
	if( !arrayFiles.empty() )
		RageException::Throw( 
			"The folder '%s' contains music files.\n\n"
			"This means that you have a music outside of a song folder.\n"
			"All song folders must reside in a group folder.  For example, 'Songs/DDR 4th Mix/B4U'.\n"
			"See the StepMania readme for more info.",
			sDir.c_str()
		);
	
}

void SongManager::DeletePlayerGroup( PlayerNumber pn )
{
	LOG->Trace( "SongManager::DeletePlayerGroup()" );

	// Get our working directory
	CString sDir = PROFILEMAN->GetProfileDir( pn );

	// Make sure sDir has a trailing slash.
	if( sDir.Right(1) != "/" )
		sDir += "/";
	
	// We want the songs folder. Append appropriately.
	sDir = sDir + "Songs/";

	// Set this up beforehand, so we have a place to actually put these things.
	CString sGroupName;

	if( PROFILEMAN->IsUsingProfile(pn) )
	{
		Profile* p = PROFILEMAN->GetProfile( pn );
		CString sDisplayName = p->GetDisplayName();

		// No name set. Revert to "Player 1" format.
		if( !sDisplayName || sDisplayName == "NoName" )
			sDisplayName = ssprintf( "Player %d", pn+1 );
			
		sGroupName = sDisplayName + "\'s Songs";
	}
	else
		sGroupName = ssprintf( "Player %d\'s Songs", pn+1 );

	CStringArray sTempGroup;

	for( unsigned i=0; i < m_sGroupNames.size(); i++ )
	{
		// Push the non-player song dirs into the temp array
		if( m_sGroupNames[i] != sGroupName )
			sTempGroup.push_back( m_sGroupNames[i] );
	}

	// Clear the main array, and then transfer the data back to it
	m_sGroupNames.clear();

	for( unsigned i=0; i < sTempGroup.size(); i++ )
		m_sGroupNames.push_back( sTempGroup[i] );
}

void SongManager::AddGroup( CString sDir, CString sGroupDirName )
{
	unsigned j;
	for(j = 0; j < m_sGroupNames.size(); ++j)
		if( sGroupDirName == m_sGroupNames[j] ) break;

	if( j != m_sGroupNames.size() )
		return; /* the group is already added */

	// Look for a group banner in this group folder
	CStringArray arrayGroupBanners;
	GetDirListing( sDir+sGroupDirName+"/*.png", arrayGroupBanners );
	GetDirListing( sDir+sGroupDirName+"/*.jpg", arrayGroupBanners );
	GetDirListing( sDir+sGroupDirName+"/*.gif", arrayGroupBanners );
	GetDirListing( sDir+sGroupDirName+"/*.bmp", arrayGroupBanners );

	CString sBannerPath;
	if( !arrayGroupBanners.empty() )
		sBannerPath = sDir+sGroupDirName+"/"+arrayGroupBanners[0] ;
	else
	{
		// Look for a group banner in the parent folder
		GetDirListing( sDir+sGroupDirName+".png", arrayGroupBanners );
		GetDirListing( sDir+sGroupDirName+".jpg", arrayGroupBanners );
		GetDirListing( sDir+sGroupDirName+".gif", arrayGroupBanners );
		GetDirListing( sDir+sGroupDirName+".bmp", arrayGroupBanners );
		if( !arrayGroupBanners.empty() )
			sBannerPath = sDir+arrayGroupBanners[0];
	}

	LOG->Trace( "Group banner for '%s' is '%s'.", sGroupDirName.c_str(), 
		sBannerPath != ""? sBannerPath.c_str():"(none)" );
	m_sGroupNames.push_back( sGroupDirName );
	m_sGroupBannerPaths.push_back(sBannerPath);

	// Look for a group background in this group folder
	CStringArray arrayGroupBackgrounds;
	GetDirListing( sDir+sGroupDirName+"/*-bg.png", arrayGroupBackgrounds );
	GetDirListing( sDir+sGroupDirName+"/*-bg.jpg", arrayGroupBackgrounds );
	GetDirListing( sDir+sGroupDirName+"/*-bg.gif", arrayGroupBackgrounds );
	GetDirListing( sDir+sGroupDirName+"/*-bg.bmp", arrayGroupBackgrounds );

	CString sBackgroundPath;
	if( !arrayGroupBackgrounds.empty() )
		sBackgroundPath = sDir+sGroupDirName+"/"+arrayGroupBackgrounds[0] ;
	else
	{
		// Look for a group background in the parent folder
		GetDirListing( sDir+sGroupDirName+"-bg.png", arrayGroupBackgrounds );
		GetDirListing( sDir+sGroupDirName+"-bg.jpg", arrayGroupBackgrounds );
		GetDirListing( sDir+sGroupDirName+"-bg.gif", arrayGroupBackgrounds );
		GetDirListing( sDir+sGroupDirName+"-bg.bmp", arrayGroupBackgrounds );
		if( !arrayGroupBackgrounds.empty() )
			sBackgroundPath = sDir+arrayGroupBackgrounds[0];
	}

	m_sGroupBGPaths.push_back(sBackgroundPath);
}

void SongManager::LoadStepManiaSongDir( CString sDir, LoadingWindow *ld )
{
	// Make sure sDir has a trailing slash
	if( sDir.Right(1) != "/" )
		sDir += "/";

	// Find all group directories in "Songs" folder
	CStringArray arrayGroupDirs;
	GetDirListing( sDir+"*", arrayGroupDirs, true );
	SortCStringArray( arrayGroupDirs );

	for( unsigned i=0; i < arrayGroupDirs.size(); i++ )	// for each dir in /Songs/
	{
		CString sGroupDirName = arrayGroupDirs[i];

		if( 0 == stricmp( sGroupDirName, "cvs" ) )	// the directory called "CVS"
			continue;		// ignore it

		SanityCheckGroupDir(sDir+sGroupDirName);

		// Find all Song folders in this group directory
		CStringArray arraySongDirs;
		GetDirListing( sDir+sGroupDirName + "/*", arraySongDirs, true, true );
		SortCStringArray( arraySongDirs );

		LOG->Trace("Attempting to load %i songs from \"%s\"", arraySongDirs.size(), (sDir+sGroupDirName).c_str() );
		int loaded = 0;

		for( unsigned j=0; j< arraySongDirs.size(); j++ )	// for each song dir
		{
			CString sSongDirName = arraySongDirs[j];

			if( 0 == stricmp( Basename(sSongDirName), "cvs" ) )	// the directory called "CVS"
				continue;		// ignore it

			// this is a song directory.  Load a new song!
			if( ld ) {
				ld->SetText( ssprintf("Loading songs...\n%s\n%s",
					Basename(sGroupDirName).c_str(),
					Basename(sSongDirName).c_str()));
				ld->Paint();
			}

			Song* pNewSong = new Song;
			if( !pNewSong->LoadFromSongDir( sSongDirName ) ) 
			{
				// The song failed to load.
				delete pNewSong;
				continue;
			}
			
			m_pMachineSongs.push_back( pNewSong );	// We'll need this when unloading player songs
			m_pSongs.push_back( pNewSong );
			loaded++;

			// Keep track of how many songs we'll have for SORT_LIST
			if( stricmp(pNewSong->m_sListSortPosition,"") != 0 )
			{
				int iValue = atoi(pNewSong->m_sListSortPosition);

				if( iValue > m_iHighestListSortValue )
					m_iHighestListSortValue = iValue;
			}
		}

		LOG->Trace("Loaded %i songs from \"%s\"", loaded, (sDir+sGroupDirName).c_str() );

		/* Don't add the group name if we didn't load any songs in this group. */
		if(!loaded) continue;

		/* Add this group to the group array. */
		AddGroup(sDir, sGroupDirName);

		/* Cache and load the group banner. */
		BANNERCACHE->CacheBanner( GetGroupBannerPath(sGroupDirName) );

		/* Cache and load the group background. */
		BACKGROUNDCACHE->CacheBackground( GetGroupBackgroundPath(sGroupDirName) );
		
		/* Load the group sym links (if any)*/
		LoadGroupSymLinks(sDir, sGroupDirName);
	}
}

void SongManager::LoadPlayerSongs( PlayerNumber pn )
{
	LOG->Trace( "SongManager::LoadPlayerSongs()" );

	// Get our working directory
	CString sDir = PROFILEMAN->GetProfileDir( pn );

	// Make sure sDir has a trailing slash.
	if( sDir.Right(1) != "/" )
		sDir += "/";
	
	// We want the songs folder. Append appropriately.
	sDir = sDir + "Songs/";

	// Set this up beforehand, so we have a place to actually put these things.
	CString sGroupName;

	if( PROFILEMAN->IsUsingProfile(pn) )
	{
		Profile* p = PROFILEMAN->GetProfile( pn );
		CString sDisplayName = p->GetDisplayName();

		// No name set. Revert to "Player 1" format.
		if( !sDisplayName || sDisplayName == "NoName" )
			sDisplayName = ssprintf( "Player %d", pn+1 );
			
		sGroupName = sDisplayName + "\'s Songs";
	}
	else
	{
		sGroupName = ssprintf( "Player %d\'s Songs", pn+1 );
	}

	// Find all folders in the song directory.
	CStringArray arraySongRootDirs;
	GetDirListing( sDir + "/*", arraySongRootDirs, true, true );
	SortCStringArray( arraySongRootDirs );

	// Subdir song list, to load subdirectory songs from.
	CStringArray arraySongSubDirs;

	// Add any subdirectories to their own list.
	for( unsigned i=0; i < arraySongRootDirs.size(); i++ )
	{
		// Create a subdirectory container, for folder contents
		CStringArray arraySubDirs;

		// Find all song folders in this subgroup directory, and add them
		GetDirListing( arraySongRootDirs[i] + "/*", arraySubDirs, true, true );

		if( !arraySubDirs.size() ) // nothing here
			continue;

		// Add everything we find, for loading
		for( unsigned j=0; j < arraySubDirs.size(); j++ )
			arraySongSubDirs.push_back( arraySubDirs[j] );
	}

	SortCStringArray( arraySongSubDirs ); // organise what we just loaded

	/* We want subdirectories loaded first. Since the for() operator just	*
	 * goes through the vector, create a final vector and append both. I've	*
	 * heard this is faster than the vector insert() function. It's kind of *
	 * ugly, but we'll go with it.											*/

	CStringArray arraySongDirs;
	for( unsigned m=0; m < arraySongSubDirs.size(); m++ )
		arraySongDirs.push_back( arraySongSubDirs[m] );
	for( unsigned n=0; n < arraySongRootDirs.size(); n++ )
		arraySongDirs.push_back( arraySongRootDirs[n] );

	LOG->Trace("Attempting to load %i songs and %i subdirectory songs from \"%s\"",
		arraySongRootDirs.size(), arraySongSubDirs.size(), sDir.c_str() );
	int iSongsLoaded = 0;

	// And this is where my restructuring work pays off!

	// keep time so we don't run much longer than expected.
	RageTimer LoadTimer;

	for( unsigned j=0; j < arraySongDirs.size(); j++ )
	{
		if( LoadTimer.Ago() > PREFSMAN->m_fPlayerSongsLoadTimeout )

		{
			LOG->Warn( "Loading interrupted after %i songs. Timer reached %f, with a %f second limit.",
			iSongsLoaded, LoadTimer.Ago(), PREFSMAN->m_fPlayerSongsLoadTimeout );
			break;
		}

		// we want to stop right on the number, not after. "Greater than" is added as a safeguard.
		if( iSongsLoaded >= PREFSMAN->m_iPlayerSongsLoadLimit )
		{
			LOG->Warn( "Loading interrupted. Limit of %i songs was reached, after %f seconds.",
			PREFSMAN->m_iPlayerSongsLoadLimit, LoadTimer.Ago() );
			break;
		}

		CString sSongDirName = arraySongDirs[j];
		Song* pNewSong = new Song;
		
		// we need a custom loader for these songs.
		if( !pNewSong->LoadFromPlayerSongDir( sSongDirName, sGroupName ) )
		{
			// The song failed to load
			LOG->Warn("Loading %s from \"%s\" failed", sSongDirName.c_str(), arraySongDirs[j].c_str() );
			delete pNewSong;
			continue;
		}
		
		m_pSongs.push_back( pNewSong );
		iSongsLoaded++;
	}

	LOG->Trace( "%i songs loaded from %s in %f seconds.", iSongsLoaded, sDir.c_str(), LoadTimer.Ago() );

	// If we have content, make sure to add the group.
	if( iSongsLoaded )
	{
		AddGroup( sDir, sGroupName );

		/* Cache and load the group banner. */
		BANNERCACHE->CacheBanner( GetGroupBannerPath(sGroupName) );

		/* Cache and load the group background. */
		BACKGROUNDCACHE->CacheBackground( GetGroupBackgroundPath(sGroupName) );
	}
}

void SongManager::LoadGroupSymLinks(CString sDir, CString sGroupFolder)
{
	// Find all symlink files in this folder
	CStringArray arraySymLinks;
	GetDirListing( sDir+sGroupFolder+"/*.include", arraySymLinks, false );
	SortCStringArray( arraySymLinks );
	for( unsigned s=0; s< arraySymLinks.size(); s++ )	// for each symlink in this dir, add it in as a song.
	{
		MsdFile		msdF;
		msdF.ReadFile( sDir+sGroupFolder+"/"+arraySymLinks[s].c_str() );
		CString	sSymDestination = msdF.GetParam(0,1);	// Should only be 1 vale&param...period.
		
		Song* pNewSong = new Song;
		if( !pNewSong->LoadFromSongDir( sSymDestination ) )
			delete pNewSong; // The song failed to load.
		else
		{
			const vector<Steps*>& vpSteps = pNewSong->GetAllSteps();
			while( vpSteps.size() )
				pNewSong->RemoveSteps( vpSteps[0] );

			pNewSong->m_BackgroundChanges.clear();

			pNewSong->m_bIsSymLink = true;	// Very important so we don't double-parse later
			pNewSong->m_sGroupName = sGroupFolder;

			m_pSongs.push_back( pNewSong );
		}
	}
}

void SongManager::PreloadSongImages()
{
	ASSERT( TEXTUREMAN );

	bool bSkipBanners = false;
	bool bSkipBackgrounds = false;

	if( PREFSMAN->m_BannerCache != PrefsManager::BNCACHE_FULL )
		bSkipBanners = true;
		//return;

	if( PREFSMAN->m_BackgroundCache != PrefsManager::BGCACHE_FULL )
		bSkipBackgrounds = true;

	unsigned i;

	if( !bSkipBanners && !bSkipBackgrounds )
	{
		const vector<Song*> &songs = SONGMAN->GetAllSongs();

		for( i = 0; i < songs.size(); ++i )
		{
			if( !songs[i]->HasBanner() && !songs[i]->HasBackground() )
				continue;

			if( !bSkipBanners && songs[i]->HasBanner() )
			{
				const RageTextureID BNID = Sprite::SongBannerTexture( songs[i]->GetBannerPath() );
				TEXTUREMAN->PermanentTexture( BNID );
			}

			if( !bSkipBackgrounds && songs[i]->HasBackground() )
			{
				const RageTextureID BGID = Sprite::SongBackgroundSSMTexture( songs[i]->GetBackgroundPath() );
				TEXTUREMAN->PermanentTexture( BGID );
			}
		}
	}

	if( !bSkipBanners )
	{
		vector<Course*> courses;
		SONGMAN->GetAllCourses( courses, false );

		for( i = 0; i < courses.size(); ++i )
		{
			if( !courses[i]->HasBanner() )
				continue;

			const RageTextureID ID = Sprite::SongBannerTexture( courses[i]->m_sBannerPath );
			TEXTUREMAN->PermanentTexture( ID );
		}
	}
}

void SongManager::FreeSongs()
{
	m_sGroupNames.clear();
	m_sGroupBannerPaths.clear();
	m_sGroupBGPaths.clear();

	for( unsigned i=0; i<m_pSongs.size(); i++ )
	{
		SAFE_DELETE( m_pSongs[i] );
	}

	m_pSongs.clear();
	m_pMachineSongs.clear();
	m_pListSortSongs.clear();

	m_sGroupBannerPaths.clear();
	m_sGroupBGPaths.clear();

	for( int i = 0; i < NUM_PROFILE_SLOTS; ++i )
		m_pBestSongs[i].clear();

	m_pShuffledSongs.clear();
}

CString SongManager::GetGroupBannerPath( CString sGroupName )
{
	unsigned i;
	for(i = 0; i < m_sGroupNames.size(); ++i)
		if( sGroupName == m_sGroupNames[i] ) break;

	if( i == m_sGroupNames.size() )
		return "";

	return m_sGroupBannerPaths[i];
}

CString SongManager::GetGroupBackgroundPath( CString sGroupName )
{
	unsigned i;
	for(i = 0; i < m_sGroupNames.size(); ++i)
		if( sGroupName == m_sGroupNames[i] ) break;

	if( i == m_sGroupNames.size() )
		return "";

	return m_sGroupBGPaths[i];
}

void SongManager::GetGroupNames( CStringArray &AddTo )
{
	AddTo.insert(AddTo.end(), m_sGroupNames.begin(), m_sGroupNames.end() );
}

bool SongManager::DoesGroupExist( CString sGroupName )
{
	for( unsigned i = 0; i < m_sGroupNames.size(); ++i )
	{
		if( !m_sGroupNames[i].CompareNoCase(sGroupName) )
			return true;
	}

	return false;
}

RageColor SongManager::GetGroupColor( const CString &sGroupName )
{
	UpdateMetrics();

	// search for the group index
	unsigned i;
	for( i=0; i<m_sGroupNames.size(); i++ )
	{
		if( m_sGroupNames[i] == sGroupName )
			break;
	}
	ASSERT_M( i != m_sGroupNames.size(), sGroupName );	// this is not a valid group

	return g_vGroupColors[i%g_vGroupColors.size()];
}

RageColor SongManager::GetSongColor( const Song* pSong )
{
	ASSERT( pSong );

	// Update our metric here
	UpdateMetrics();

	/* XXX:
	 * Previously, this matched all notes, which set a song to "extra" if it
	 * had any 10-foot steps at all, even edits or doubles.
	 *
	 * For now, only look at notes for the current note type.  This means that
	 * if a song has 10-foot steps on Doubles, it'll only show up red in Doubles.
	 * That's not too bad, I think.  This will also change it in the song scroll,
	 * which is a little odd but harmless. 
	 *
	 * XXX: Ack.  This means this function can only be called when we have a style
	 * set up, which is too restrictive.  How to handle this?
	 */

	// Mike: 3.9+ will do a hybrid system; if the style is set, look only at the current note types. If not, then
	// set as extra if any steps match.

	const Style* pStyle = GAMESTATE->GetCurrentStyle();
	const vector<Steps*>& vpSteps = pSong->GetAllSteps();
	RageColor SongColor = GetGroupColor( pSong->m_sGroupName );
	
	for( unsigned i=0; i<vpSteps.size(); i++ )
	{
		const Steps* pSteps = vpSteps[i];

		// Only do this for beginner -> heavy steps
		switch( pSteps->GetDifficulty() )
		{
		case DIFFICULTY_CHALLENGE:
		case DIFFICULTY_EDIT:
			continue;
		}

		if( pStyle )
		{
			if( pSteps->m_StepsType != GAMESTATE->GetCurrentStyle()->m_StepsType )
				continue;
		}

		// If the difficulty is hidden, don't accidentally reveal that it's above this threshold
		if( pSteps->m_bHiddenDifficulty )
			continue;
	
		if( stricmp(pSong->MeterType(),"DDR X")==0 && pSteps->GetMeter() >= EXTRA_COLOR_METER_X )
			SongColor = EXTRA_COLOR;
		if( stricmp(pSong->MeterType(),"ITG")==0 && pSteps->GetMeter() >= EXTRA_COLOR_METER_ITG )
			SongColor = EXTRA_COLOR;
		if( stricmp(pSong->MeterType(),"DDR")==0 && pSteps->GetMeter() >= EXTRA_COLOR_METER )
			SongColor = EXTRA_COLOR;
	}

	// Allow users to set color via individual stepfiles
	CString sMenuColor = pSong->m_sMenuColor;

	if( stricmp(sMenuColor,"") != 0 )
	{
		sMenuColor.MakeLower();
		
		// Stock colors
		if( stricmp(sMenuColor,"white") == 0 )
			SongColor.FromString( "1,1,1,1" );
		else if( stricmp(sMenuColor,"black") == 0 )
			SongColor.FromString( "0,0,0,0" );
		else if( stricmp(sMenuColor,"brightred") == 0 )
			SongColor.FromString( "1,0,0,1" );
		else if( stricmp(sMenuColor,"brightgreen") == 0 )
			SongColor.FromString( "0,1,0,1" );
		else if( stricmp(sMenuColor,"brightblue") == 0 )
			SongColor.FromString( "0,0,1,1" );

		// Mixes of the color values
		else if( stricmp(sMenuColor,"red") == 0 )
			SongColor.FromString( "1,0.3,0.3,1" );
		else if( stricmp(sMenuColor,"green") == 0 )
			SongColor.FromString( "0.1,0.7,0.3,1" );
		else if( stricmp(sMenuColor,"blue") == 0 )
			SongColor.FromString( "0,0.4,0.8,1" );

		else if( stricmp(sMenuColor,"yellow") == 0 )
			SongColor.FromString( "0.9,0.9,0,1" );
		else if( stricmp(sMenuColor,"pink") == 0 )
			SongColor.FromString( "0.8,0.1,0.6,1" );
		else if( stricmp(sMenuColor,"purple") == 0 )
			SongColor.FromString( "0.6,0.4,0.8,1" );
		else if( stricmp(sMenuColor,"seagreen") == 0 )
			SongColor.FromString( "0,0.6,0.6,1" );
		else if( stricmp(sMenuColor,"cyan") == 0 )
			SongColor.FromString( "0,0.8,0.8,1" );
		else if( stricmp(sMenuColor,"orange") == 0 )
			SongColor.FromString( "0.8,0.6,0,1" );

		// DDR Songwheel colors
		else if( stricmp(sMenuColor,"ddrgreen") == 0 )
			SongColor.FromString( "0.22,0.92,0.22,0.81" );
		else if( stricmp(sMenuColor,"ddrblue") == 0 )
			SongColor.FromString( "0.31,0.67,0.98,0.95" );
		else if( stricmp(sMenuColor,"ddryellow") == 0 )
		{
			// SongColor.FromString( "0.92,0.92,0.09,0.84" ); // What the Extreme theme says
			SongColor.FromString( "0.92,0.92,0.09,0.99" );
		}
		else if( stricmp(sMenuColor,"ddrpurple") == 0 )
			SongColor.FromString( "0.67,0.22,0.91,0.8" );

		// Theme specified colors
		else if( stricmp(sMenuColor,"extra") == 0 || stricmp(sMenuColor,"boss") == 0 )
			SongColor = EXTRA_COLOR;
		else if( stricmp(sMenuColor,"group") == 0 )
			SongColor = GetGroupColor( pSong->m_sGroupName );

		// If they've manually specified or incorrectly specified
		else
		{
			// Fallback to ensure that, should a improper value be entered, the colour will default back to the
			// group color scheme
			if( !SongColor.FromString(sMenuColor) ) // This line loads the RageColor string
			{
				LOG->Warn( "The color value '%s' for song '%s' is invalid, and will be ignored.", sMenuColor.c_str(), pSong->m_sSongFileName.c_str() );
				SongColor = GetGroupColor( pSong->m_sGroupName );
			}
		}
	}

	return SongColor;
}

RageColor SongManager::GetDifficultyColor( Difficulty dc ) const
{
	switch( dc )
	{
	case DIFFICULTY_BEGINNER:	return BEGINNER_COLOR;
	case DIFFICULTY_EASY:		return EASY_COLOR;
	case DIFFICULTY_MEDIUM:		return MEDIUM_COLOR;
	case DIFFICULTY_HARD:		return HARD_COLOR;
	case DIFFICULTY_CHALLENGE:	return CHALLENGE_COLOR;
	case DIFFICULTY_EDIT:		return EDIT_COLOR;
	default:	ASSERT(0);	return EDIT_COLOR;
	}
}

static void GetSongsFromVector( const vector<Song*> &Songs, vector<Song*> &AddTo, CString sGroupName, int iMaxStages )
{
	AddTo.clear();

	for( unsigned i=0; i<Songs.size(); i++ )
		if( sGroupName==GROUP_ALL_MUSIC || sGroupName==Songs[i]->m_sGroupName )
			if( SongManager::GetNumStagesForSong(Songs[i]) <= iMaxStages )
				AddTo.push_back( Songs[i] );
}

void SongManager::GetSongs( vector<Song*> &AddTo, CString sGroupName, int iMaxStages ) const
{
	GetSongsFromVector( m_pSongs, AddTo, sGroupName, iMaxStages );
}

void SongManager::GetBestSongs( vector<Song*> &AddTo, CString sGroupName, int iMaxStages, ProfileSlot slot ) const
{
	GetSongsFromVector( m_pBestSongs[slot], AddTo, sGroupName, iMaxStages );
}

int SongManager::GetNumSongs() const
{
	return m_pSongs.size();
}

int SongManager::GetNumGroups() const
{
	return m_sGroupNames.size();
}

int SongManager::GetNumCourses() const
{
	return m_pCourses.size();
}

CString SongManager::ShortenGroupName( CString sLongGroupName )
{
	sLongGroupName.Replace( "Dance Dance Revolution", "DDR" );
	sLongGroupName.Replace( "dance dance revolution", "DDR" );
	sLongGroupName.Replace( "DANCE DANCE REVOLUTION", "DDR" );
	sLongGroupName.Replace( "Pump It Up", "PIU" );
	sLongGroupName.Replace( "pump it up", "PIU" );
	sLongGroupName.Replace( "PUMP IT UP", "PIU" );
	sLongGroupName.Replace( "ParaParaParadise", "PPP" );
	sLongGroupName.Replace( "paraparaparadise", "PPP" );
	sLongGroupName.Replace( "PARAPARAPARADISE", "PPP" );
	sLongGroupName.Replace( "Para Para Paradise", "PPP" );
	sLongGroupName.Replace( "para para paradise", "PPP" );
	sLongGroupName.Replace( "PARA PARA PARADISE", "PPP" );
	sLongGroupName.Replace( "Dancing Stage", "DS" );
	sLongGroupName.Replace( "dancing stage", "DS" );
	sLongGroupName.Replace( "DANCING STAGE", "DS" );
	sLongGroupName.Replace( "Ez2dancer", "EZ2" );
	sLongGroupName.Replace( "Ez 2 Dancer", "EZ2");
	sLongGroupName.Replace( "Technomotion", "TM");
	sLongGroupName.Replace( "Techno Motion", "TM");
	sLongGroupName.Replace( "Dance Station 3DDX", "3DDX");
	sLongGroupName.Replace( "DS3DDX", "3DDX");
	sLongGroupName.Replace( "BeatMania", "BM");
	sLongGroupName.Replace( "Beatmania", "BM");
	sLongGroupName.Replace( "BEATMANIA", "BM");
	sLongGroupName.Replace( "beatmania", "BM");
	return sLongGroupName;
}

int SongManager::GetNumStagesForSong( const Song* pSong )
{
	ASSERT( pSong );
	if( pSong->m_fMusicLengthSeconds > PREFSMAN->m_fMarathonVerSongSeconds )
		return 3;
	if( pSong->m_fMusicLengthSeconds > PREFSMAN->m_fLongVerSongSeconds )
		return 2;
	else
		return 1;
}

void SongManager::InitCoursesFromDisk( LoadingWindow *ld )
{
	unsigned i;

	LOG->Trace( "Loading courses." );

	//
	// Load courses from in Courses dir
	//
	CStringArray saCourseFiles;
	GetDirListing( COURSES_DIR "*.crs", saCourseFiles, false, true );
	for( i=0; i<saCourseFiles.size(); i++ )
	{
		Course* pCourse = new Course;
		pCourse->LoadFromCRSFile( saCourseFiles[i] );
		m_pCourses.push_back( pCourse );

		if( ld )
		{
			ld->SetText( ssprintf("Loading courses...\n%s\n%s",
				"Courses",
				Basename(saCourseFiles[i]).c_str()));
			ld->Paint();
		}

	}


	// Find all group directories in Courses dir
	CStringArray arrayGroupDirs;
	GetDirListing( COURSES_DIR "*", arrayGroupDirs, true );
	SortCStringArray( arrayGroupDirs );
	
	for( i=0; i< arrayGroupDirs.size(); i++ )	// for each dir in /Courses/
	{
		CString sGroupDirName = arrayGroupDirs[i];

		if( 0 == stricmp( sGroupDirName, "cvs" ) )	// the directory called "CVS"
			continue;		// ignore it

		// Find all CRS files in this group directory
		CStringArray arrayCoursePaths;
		GetDirListing( COURSES_DIR + sGroupDirName + "/*.crs", arrayCoursePaths, false, true );
		SortCStringArray( arrayCoursePaths );

		for( unsigned j=0; j<arrayCoursePaths.size(); j++ )
		{
			if( ld )
			{
				ld->SetText( ssprintf("Loading courses...\n%s\n%s",
					Basename(arrayGroupDirs[i]).c_str(),
					Basename(arrayCoursePaths[j]).c_str()));
				ld->Paint();
			}

			Course* pCourse = new Course;
			pCourse->LoadFromCRSFile( arrayCoursePaths[j] );
			m_pCourses.push_back( pCourse );
		}
	}
}
	
void SongManager::InitAutogenCourses()
{
	//
	// Create group courses for Endless and Nonstop
	//
	CStringArray saGroupNames;
	this->GetGroupNames( saGroupNames );
	Course* pCourse;
	for( unsigned g=0; g<saGroupNames.size(); g++ )	// foreach Group
	{
		CString sGroupName = saGroupNames[g];
		vector<Song*> apGroupSongs;
		GetSongs( apGroupSongs, sGroupName );

		// Generate random courses from each group.
		pCourse = new Course;
		pCourse->AutogenEndlessFromGroup( sGroupName, DIFFICULTY_MEDIUM );
		m_pCourses.push_back( pCourse );

		pCourse = new Course;
		pCourse->AutogenNonstopFromGroup( sGroupName, DIFFICULTY_MEDIUM );
		m_pCourses.push_back( pCourse );
	}
	
	vector<Song*> apCourseSongs = GetAllSongs();

	// Generate "All Songs" endless course.
	pCourse = new Course;
	pCourse->AutogenEndlessFromGroup( "", DIFFICULTY_MEDIUM );
	m_pCourses.push_back( pCourse );

	/* Generate Oni courses from artists.  Only create courses if we have at least
	 * four songs from an artist; create 3- and 4-song courses. */
	{
		/* We normally sort by translit artist.  However, display artist is more
		 * consistent.  For example, transliterated Japanese names are alternately
		 * spelled given- and family-name first, but display titles are more consistent. */
		vector<Song*> apSongs = this->GetAllSongs();
		SongUtil::SortSongPointerArrayByDisplayArtist( apSongs );

		CString sCurArtist = "";
		CString sCurArtistTranslit = "";
		int iCurArtistCount = 0;

		vector<Song *> aSongs;
		unsigned i = 0;
		do {
			CString sArtist = i >= apSongs.size()? CString(""): apSongs[i]->GetDisplayArtist();
			CString sTranslitArtist = i >= apSongs.size()? CString(""): apSongs[i]->GetTranslitArtist();
			if( i < apSongs.size() && !sCurArtist.CompareNoCase(sArtist) )
			{
				aSongs.push_back( apSongs[i] );
				++iCurArtistCount;
				continue;
			}

			/* Different artist, or we're at the end.  If we have enough entries for
			 * the last artist, add it.  Skip blanks and "Unknown artist". */
			if( iCurArtistCount >= 3 && sCurArtistTranslit != "" &&
				sCurArtistTranslit.CompareNoCase("Unknown artist") &&
				sCurArtist.CompareNoCase("Unknown artist") )
			{
				pCourse = new Course;
				pCourse->AutogenOniFromArtist( sCurArtist, sCurArtistTranslit, aSongs, DIFFICULTY_HARD );
				m_pCourses.push_back( pCourse );
			}

			aSongs.clear();
			
			if( i < apSongs.size() )
			{
				sCurArtist = sArtist;
				sCurArtistTranslit = sTranslitArtist;
				iCurArtistCount = 1;
				aSongs.push_back( apSongs[i] );
			}
		} while( i++ < apSongs.size() );
	}
}


void SongManager::FreeCourses()
{
	for( unsigned i=0; i<m_pCourses.size(); i++ )
		delete m_pCourses[i];
	m_pCourses.clear();

	for( int i = 0; i < NUM_PROFILE_SLOTS; ++i )
		m_pBestCourses[i].clear();
	m_pShuffledCourses.clear();
}

// Called periodically to wipe out cached NoteData.  This is called when we change screens.
void SongManager::Cleanup()
{
	for( unsigned i=0; i<m_pSongs.size(); i++ )
	{
		Song* pSong = m_pSongs[i];
		const vector<Steps*>& vpSteps = pSong->GetAllSteps();
		for( unsigned n=0; n<vpSteps.size(); n++ )
		{
			Steps* pSteps = vpSteps[n];
			pSteps->Compress();
		}
	}
}

/* Flush all Song*, Steps* and Course* caches.  This is called on reload, and when
 * any of those are removed or changed.  This doesn't touch GAMESTATE and StageStats
 * pointers, which are updated explicitly in Song::RevertFromDisk. */
void SongManager::Invalidate( Song *pStaleSong )
{
	//
	// Save list of all old Course and Trail pointers
	//
	map<Course*,CourseID> mapOldCourseToCourseID;
	typedef pair<TrailID,Course*> TrailIDAndCourse;
	map<Trail*,TrailIDAndCourse> mapOldTrailToTrailIDAndCourse;
	FOREACH_CONST( Course*, this->m_pCourses, pCourse )
	{
		CourseID id;
		id.FromCourse( *pCourse );
		mapOldCourseToCourseID[*pCourse] = id;
		vector<Trail *> Trails;
		(*pCourse)->GetAllCachedTrails( Trails );
		FOREACH_CONST( Trail*, Trails, pTrail )
		{
			TrailID id;
			id.FromTrail( *pTrail );
			mapOldTrailToTrailIDAndCourse[*pTrail] = TrailIDAndCourse(id, *pCourse);
		}
	}

	// It's a real pain to selectively invalidate only those Courses with 
	// dependencies on the stale Song.  So, instead, just reload all Courses.
	// It doesn't take very long.
	FreeCourses();
	InitCoursesFromDisk( NULL );
	InitAutogenCourses();

	// invalidate cache
	StepsID::Invalidate( pStaleSong );

#define CONVERT_COURSE_POINTER( pCourse ) { \
	CourseID id = mapOldCourseToCourseID[pCourse]; /* this will always succeed */ \
	pCourse = id.ToCourse(); }

	/* Ugly: We need the course pointer to restore a trail pointer, and both have
	 * been invalidated.  We need to go through our mapping, and update the course
	 * pointers, so we can use that to update trail pointers.  */
	{
		map<Trail*,TrailIDAndCourse>::iterator it;
		for( it = mapOldTrailToTrailIDAndCourse.begin(); it != mapOldTrailToTrailIDAndCourse.end(); ++it )
		{
			TrailIDAndCourse &tidc = it->second;
			CONVERT_COURSE_POINTER( tidc.second );
		}
	}

	CONVERT_COURSE_POINTER( GAMESTATE->m_pCurCourse );
	CONVERT_COURSE_POINTER( GAMESTATE->m_pPreferredCourse );

#define CONVERT_TRAIL_POINTER( pTrail ) { \
	if( pTrail != NULL ) { \
		map<Trail*,TrailIDAndCourse>::iterator it; \
		it = mapOldTrailToTrailIDAndCourse.find(pTrail); \
		ASSERT_M( it != mapOldTrailToTrailIDAndCourse.end(), ssprintf("%p", pTrail) ); \
		const TrailIDAndCourse &tidc = it->second; \
		const TrailID &id = tidc.first; \
		const Course *pCourse = tidc.second; \
		pTrail = id.ToTrail( pCourse, true ); \
	} \
}

	FOREACH_PlayerNumber( pn )
	{
		CONVERT_TRAIL_POINTER( GAMESTATE->m_pCurTrail[pn] );
	}
}

/* If bAllowNotesLoss is true, any global notes pointers which no longer exist
 * (or exist but couldn't be matched) will be set to NULL.  This is used when
 * reverting out of the editor.  If false, this is unexpected and will assert.
 * This is used when reverting out of gameplay, in which case we may have StageStats,
 * etc. which may cause hard-to-trace crashes down the line if we set them to NULL. */
void SongManager::RevertFromDisk( Song *pSong, bool bAllowNotesLoss )
{
	/* Reverting from disk is brittle, and touches a lot of tricky and rarely-
	 * used code paths.  If it's ever used during a game, log it. */
	LOG->MapLog( "RevertFromDisk", "Reverted \"%s\" from disk", pSong->GetTranslitMainTitle().c_str() );

	// Ugly:  When we re-load the song, the Steps* will change.
	// Fix GAMESTATE->m_CurSteps, g_CurStageStats, g_vPlayedStageStats[] after reloading.
	/* XXX: This is very brittle.  However, we must know about all globals uses of Steps*,
	 * so we can check to make sure we didn't lose any steps which are referenced ... */


	//
	// Save list of all old Steps pointers for the song
	//
	map<Steps*,StepsID> mapOldStepsToStepsID;
	FOREACH_CONST( Steps*, pSong->GetAllSteps(), pSteps )
	{
		StepsID id;
		id.FromSteps( *pSteps );
		mapOldStepsToStepsID[*pSteps] = id;
	}


	//
	// Reload the song
	//
	const CString dir = pSong->GetSongDir();
	FILEMAN->FlushDirCache( dir );

	/* Erase existing data and reload. */
	pSong->Reset();
	const bool OldVal = PREFSMAN->m_bFastLoad;
	PREFSMAN->m_bFastLoad = false;
	pSong->LoadFromSongDir( dir );	
	/* XXX: reload edits? */
	PREFSMAN->m_bFastLoad = OldVal;


	/* Courses cache Steps pointers.  On the off chance that this isn't the last
	 * thing this screen does, clear that cache. */
	/* TODO: Don't make Song depend on SongManager.  This is breaking 
	 * encapsulation and placing confusing limitation on what can be done in 
	 * SONGMAN->Invalidate(). -Chris */
	this->Invalidate( pSong );
	StepsID::Invalidate( pSong );



#define CONVERT_STEPS_POINTER( pSteps ) { \
	if( pSteps != NULL ) { \
		map<Steps*,StepsID>::iterator it = mapOldStepsToStepsID.find(pSteps); \
		if( it != mapOldStepsToStepsID.end() ) \
			pSteps = it->second.ToSteps( pSong, bAllowNotesLoss ); \
	} \
}


	FOREACH_PlayerNumber( p )
	{
		CONVERT_STEPS_POINTER( GAMESTATE->m_pCurSteps[p] );

		FOREACH( Steps*, g_CurStageStats.vpSteps[p], pSteps )
			CONVERT_STEPS_POINTER( *pSteps );

		FOREACH( StageStats, g_vPlayedStageStats, ss )
			FOREACH( Steps*, ss->vpSteps[p], pSteps )
				CONVERT_STEPS_POINTER( *pSteps );
	}
}

void SongManager::RegenerateNonFixedCourses()
{
	for( unsigned i=0; i < m_pCourses.size(); i++ )
		m_pCourses[i]->RegenerateNonFixedTrails();
}

void SongManager::SetPreferences()
{
	for( unsigned int i=0; i<m_pSongs.size(); i++ )
	{
		/* PREFSMAN->m_bAutogenSteps may have changed. */
		m_pSongs[i]->RemoveAutoGenNotes();
		m_pSongs[i]->AddAutoGenNotes();
	}
}

void SongManager::GetAllCourses( vector<Course*> &AddTo, bool bIncludeAutogen )
{
	for( unsigned i=0; i<m_pCourses.size(); i++ )
		if( bIncludeAutogen || !m_pCourses[i]->m_bIsAutogen )
			AddTo.push_back( m_pCourses[i] );
}

void SongManager::GetNonstopCourses( vector<Course*> &AddTo, bool bIncludeAutogen )
{
	for( unsigned i=0; i<m_pCourses.size(); i++ )
		if( m_pCourses[i]->IsNonstop() )
			if( bIncludeAutogen || !m_pCourses[i]->m_bIsAutogen )
				AddTo.push_back( m_pCourses[i] );
}

void SongManager::GetOniCourses( vector<Course*> &AddTo, bool bIncludeAutogen )
{
	for( unsigned i=0; i<m_pCourses.size(); i++ )
		if( m_pCourses[i]->IsOni() )
			if( bIncludeAutogen || !m_pCourses[i]->m_bIsAutogen )
				AddTo.push_back( m_pCourses[i] );
}

void SongManager::GetEndlessCourses( vector<Course*> &AddTo, bool bIncludeAutogen )
{
	for( unsigned i=0; i<m_pCourses.size(); i++ )
		if( m_pCourses[i]->IsEndless() )
			if( bIncludeAutogen || !m_pCourses[i]->m_bIsAutogen )
				AddTo.push_back( m_pCourses[i] );
}

bool SongManager::GetExtraStageInfoFromCourse( bool bExtra2, CString sPreferredGroup,
								   Song*& pSongOut, Steps*& pStepsOut, PlayerOptions& po_out, SongOptions& so_out )
{
	const CString sExtension = ".crs";
	const CString sCourseSuffix = (bExtra2 ? "extra2" : "extra1") + sExtension;
	CString sCourseGroupSpecificFilesPath = SONGS_DIR + sPreferredGroup + "/" + sCourseSuffix;
	CString sCourseOverallFilesPath = SONGS_DIR + sCourseSuffix;
	CString sCoursePath;

	// Couldn't find course in DWI path or alternative song folders
	if( !DoesFileExist(sCourseGroupSpecificFilesPath) ) // Not in the group folder
	{
		if( !DoesFileExist(sCourseOverallFilesPath) ) // And also not in the song directory...
			return false;
		else
			sCoursePath = sCourseOverallFilesPath; // If the overall file exists, then we're using it!
	}
	else
		sCoursePath = sCourseGroupSpecificFilesPath; // If the group file exists, then we're using it!

	Course course;
	course.LoadFromCRSFile( sCoursePath );

	if( course.GetEstimatedNumStages() <= 0 )
		return false;

	Trail *pTrail = course.GetTrail( GAMESTATE->GetCurrentStyle()->m_StepsType );
	if( pTrail->m_vEntries.empty() )
		return false;

	po_out.Init();
	po_out.FromString( pTrail->m_vEntries[0].Modifiers );

	// Check to see that the machine's default noteskin is being applied, unless otherwise stated
	// Check to see if there was a noteskin specified in the modifiers by looking at each modifier
	bool bNoteSkinMatched = false;
	
	CStringArray sPlayerOptions;
	split( pTrail->m_vEntries[0].Modifiers, ",", sPlayerOptions, true );

	// Look at each specified modifier
	for( unsigned i=0; i < sPlayerOptions.size(); i++ )
	{
		bNoteSkinMatched = NOTESKIN->DoesNoteSkinExistNoCase( sPlayerOptions[i] );

		if( bNoteSkinMatched )
			break;
	}

	// If we had a match, it's already been taken care of. If there wasn't a match, though, then go to the
	// machine's default noteskin
	if( !bNoteSkinMatched )
	{
		bool bExists = false;

		CString sNoteSkin = "";
		vector<CString> sDefaults;
		split( PREFSMAN->m_sDefaultModifiers, ",", sDefaults, true);

		// Look at each specified modifier
		for( unsigned i=0; i < sDefaults.size(); i++ )
		{
			bExists = NOTESKIN->DoesNoteSkinExistNoCase( sDefaults[i] );

			if( bExists )
			{
				sNoteSkin = sDefaults[i];
				break;
			}
		}

		// If noteskin was matched, load that noteskin. If not, load the overall default noteskin,
		// not the one that we have set as the machine's default
		if( bExists )
			po_out.m_sNoteSkin = sNoteSkin;
		else
			po_out.m_sNoteSkin = "default";
	}

	so_out.Init();
	so_out.FromString( pTrail->m_vEntries[0].Modifiers );

	pSongOut = pTrail->m_vEntries[0].pSong;
	pStepsOut = pTrail->m_vEntries[0].pSteps;
	return true;
}

// Return true if n1 < n2
bool CompareNotesPointersForExtra(const Steps *n1, const Steps *n2)
{
	// Equate CHALLENGE to HARD.
	Difficulty d1 = min(n1->GetDifficulty(), DIFFICULTY_HARD);
	Difficulty d2 = min(n2->GetDifficulty(), DIFFICULTY_HARD);

	if(d1 < d2) return true;
	if(d1 > d2) return false;
	// n1 difficulty == n2 difficulty

	if(StepsUtil::CompareNotesPointersByMeter(n1,n2)) return true;
	if(StepsUtil::CompareNotesPointersByMeter(n2,n1)) return false;
	// n1 meter == n2 meter

	return StepsUtil::CompareNotesPointersByRadarValues(n1,n2);
}

void SongManager::GetExtraStageInfo( bool bExtra2, const Style *sd, 
								   Song*& pSongOut, Steps*& pStepsOut, PlayerOptions& po_out, SongOptions& so_out )
{
	CString sGroup = GAMESTATE->m_sPreferredGroup;
	if( sGroup == GROUP_ALL_MUSIC )
	{
		if( GAMESTATE->m_pCurSong == NULL )
		{
			// This normally shouldn't happen, but it's helpful to permit it for testing.
			LOG->Warn( "GetExtraStageInfo() called in GROUP_ALL_MUSIC, but GAMESTATE->m_pCurSong == NULL" );
			GAMESTATE->m_pCurSong = SONGMAN->GetRandomSong();
		}
		sGroup = GAMESTATE->m_pCurSong->m_sGroupName;
	}

	ASSERT_M( sGroup != "", ssprintf("%p '%s' '%s'",
		GAMESTATE->m_pCurSong,
		GAMESTATE->m_pCurSong? GAMESTATE->m_pCurSong->GetSongDir().c_str():"",
		GAMESTATE->m_pCurSong? GAMESTATE->m_pCurSong->m_sGroupName.c_str():"") );

	if( GetExtraStageInfoFromCourse(bExtra2, sGroup, pSongOut, pStepsOut, po_out, so_out) )
		return;
	
	// Choose a hard song for the extra stage
	Song*	pExtra1Song = NULL;		// the absolute hardest Song and Steps.  Use this for extra stage 1.
	Steps*	pExtra1Notes = NULL;
	Song*	pExtra2Song = NULL;		// a medium-hard Song and Steps.  Use this for extra stage 2.
	Steps*	pExtra2Notes = NULL;
	
	vector<Song*> apSongs;
	SONGMAN->GetSongs( apSongs, sGroup );
	for( unsigned s=0; s<apSongs.size(); s++ )	// foreach song
	{
		Song* pSong = apSongs[s];

		vector<Steps*> apSteps;
		pSong->GetSteps( apSteps, sd->m_StepsType );
		for( unsigned n=0; n<apSteps.size(); n++ )	// foreach Steps
		{
			Steps* pSteps = apSteps[n];

			if( pExtra1Notes == NULL || CompareNotesPointersForExtra(pExtra1Notes,pSteps) )	// pSteps is harder than pHardestNotes
			{
				if( pSteps->GetDifficulty() == DIFFICULTY_CHALLENGE && PREFSMAN->m_bOniExtraStage1 )
				{
					pExtra1Song = pSong;
					pExtra1Notes = pSteps;
				}
				else if( pSteps->GetDifficulty() != DIFFICULTY_CHALLENGE )
				{
					pExtra1Song = pSong;
					pExtra1Notes = pSteps;
				}
			}

			// for extra 2, we don't want to choose the hardest notes possible.  So, we'll disregard Steps with meter > 9
			if(	bExtra2 && pSteps->GetMeter() > 9 )	
				continue;	// skip

			if( pExtra2Notes == NULL  ||  CompareNotesPointersForExtra(pExtra2Notes,pSteps) )	// pSteps is harder than pHardestNotes
			{
				if( pSteps->GetDifficulty() == DIFFICULTY_CHALLENGE && PREFSMAN->m_bOniExtraStage2 )
				{
					pExtra2Song = pSong;
					pExtra2Notes = pSteps;
				}
				else if( pSteps->GetDifficulty() != DIFFICULTY_CHALLENGE )
				{
					pExtra2Song = pSong;
					pExtra2Notes = pSteps;
				}
			}
		}
	}

	if( pExtra2Song == NULL && pExtra1Song != NULL )
	{
		pExtra2Song = pExtra1Song;
		pExtra2Notes = pExtra1Notes;
	}

	// If there are any notes at all that match this StepsType, everything should be filled out.
	// Also, it's guaranteed that there is at least one Steps that matches the StepsType because the player
	// had to play something before reaching the extra stage!
	ASSERT( pExtra2Song && pExtra1Song && pExtra2Notes && pExtra1Notes );

	pSongOut = (bExtra2 ? pExtra2Song : pExtra1Song);
	pStepsOut = (bExtra2 ? pExtra2Notes : pExtra1Notes);


	po_out.Init();
	so_out.Init();
	po_out.m_fScrolls[PlayerOptions::SCROLL_REVERSE] = 1;
	po_out.m_fScrollSpeed = 1.5f;
	so_out.m_DrainType = (bExtra2 ? SongOptions::DRAIN_SUDDEN_DEATH : SongOptions::DRAIN_NO_RECOVER);
	po_out.m_fDark = PREFSMAN->m_bDarkExtraStage; // Look at the preferences to see if Dark should be used

	// Check to see if we can find the machine's default noteskin in the directory
	bool bExists = false;

	CString sNoteSkin = "";
	vector<CString> sDefaults;
	split( PREFSMAN->m_sDefaultModifiers, ",", sDefaults, true);

	// Look at each specified modifier
	for( unsigned i=0; i < sDefaults.size(); i++ )
	{
		bExists = NOTESKIN->DoesNoteSkinExistNoCase( sDefaults[i] );

		if( bExists )
		{
			sNoteSkin = sDefaults[i];
			break;
		}
	}

	// If noteskin was matched, load that noteskin. If not, load the overall default noteskin,
	// not the one that we have set as the machine's default
	if( bExists )
		po_out.m_sNoteSkin = sNoteSkin;
	else
		po_out.m_sNoteSkin = "default";
}

Song* SongManager::GetRandomSong()
{
	if( m_pShuffledSongs.empty() )
		return NULL;

	static int i = 0;
	i++;
	wrap( i, m_pShuffledSongs.size() );

	return m_pShuffledSongs[ i ];
}

Course* SongManager::GetRandomCourse()
{
	if( m_pShuffledCourses.empty() )
		return NULL;

	static int i = 0;
	i++;
	wrap( i, m_pShuffledCourses.size() );

	return m_pShuffledCourses[ i ];
}

Song* SongManager::GetSongFromDir( CString sDir )
{
	if( sDir.Right(1) != "/" )
		sDir += "/";

	sDir.Replace( '\\', '/' );

	for( unsigned int i=0; i<m_pSongs.size(); i++ )
		if( sDir.CompareNoCase(m_pSongs[i]->GetSongDir()) == 0 )
			return m_pSongs[i];

	return NULL;
}

Course* SongManager::GetCourseFromPath( CString sPath )
{
	if( sPath == "" )
		return NULL;

	for( unsigned int i=0; i<m_pCourses.size(); i++ )
		if( sPath.CompareNoCase(m_pCourses[i]->m_sPath) == 0 )
			return m_pCourses[i];

	return NULL;
}

Course* SongManager::GetCourseFromName( CString sName )
{
	if( sName == "" )
		return NULL;

	for( unsigned int i=0; i<m_pCourses.size(); i++ )
		if( sName.CompareNoCase(m_pCourses[i]->GetFullDisplayTitle()) == 0 )
			return m_pCourses[i];

	return NULL;
}


/*
 * GetSongDir() contains a path to the song, possibly a full path, eg:
 * Songs\Group\SongName                   or 
 * My Other Song Folder\Group\SongName    or
 * c:\Corny J-pop\Group\SongName
 *
 * Most course group names are "Group\SongName", so we want to
 * match against the last two elements. Let's also support
 * "SongName" alone, since the group is only important when it's
 * potentially ambiguous.
 *
 * Let's *not* support "Songs\Group\SongName" in course files.
 * That's probably a common error, but that would result in
 * course files floating around that only work for people who put
 * songs in "Songs"; we don't want that.
 */

Song *SongManager::FindSong( CString sPath )
{
	sPath.Replace( '\\', '/' );
	CStringArray bits;
	split( sPath, "/", bits );

	if( bits.size() == 1 )
		return FindSong( "", bits[0] );
	else if( bits.size() == 2 )
		return FindSong( bits[0], bits[1] );

	return NULL;	
}

Song *SongManager::FindSong( CString sGroup, CString sSong )
{
	// foreach song
	for( unsigned i = 0; i < m_pSongs.size(); i++ )
	{
		if( m_pSongs[i]->Matches(sGroup, sSong) )
			return m_pSongs[i];
	}

	return NULL;	
}

Course *SongManager::FindCourse( CString sName )
{
	for( unsigned i = 0; i < m_pCourses.size(); i++ )
	{
		if( !sName.CompareNoCase(m_pCourses[i]->GetFullDisplayTitle()) )
			return m_pCourses[i];
	}

	return NULL;
}

void SongManager::UpdateBest()
{
	// update players best
	for( int i = 0; i < NUM_PROFILE_SLOTS; ++i )
	{
		vector<Song*> &Best = m_pBestSongs[i];
		Best = m_pSongs;

		for ( unsigned j=0; j < Best.size() ; ++j )
		{
			bool bFiltered = false;

			// Filter out hidden songs.
			if( PREFSMAN->m_bEventMode && !PREFSMAN->m_bEventIgnoreSelectable )
			{ 
				if( Best[j]->GetDisplayed() != Song::SHOW_ALWAYS )
					bFiltered = true;
			}
			else if( !PREFSMAN->m_bEventMode )
			{
				if( Best[j]->GetDisplayed() != Song::SHOW_ALWAYS )
					bFiltered = true;
			}

			// Filter out locked songs.
			if( PREFSMAN->m_bEventMode && !PREFSMAN->m_bEventIgnoreUnlock )
			{ 
				if( UNLOCKMAN->SongIsLocked(Best[j]) )
					bFiltered = true;
			}
			else if( !PREFSMAN->m_bEventMode )
			{
				if( UNLOCKMAN->SongIsLocked(Best[j]) )
					bFiltered = true;
			}

			if( !bFiltered )
				continue;

			// Remove it
			swap( Best[j], Best.back() );
			Best.erase( Best.end()-1 );
		}

		SongUtil::SortSongPointerArrayByNumPlays( m_pBestSongs[i], (ProfileSlot) i, true );

		m_pBestCourses[i] = m_pCourses;
		CourseUtil::SortCoursePointerArrayByNumPlays( m_pBestCourses[i], (ProfileSlot) i, true );
	}
}

void SongManager::UpdateShuffled()
{
	// update shuffled
	m_pShuffledSongs = m_pSongs;
	random_shuffle( m_pShuffledSongs.begin(), m_pShuffledSongs.end() );

	m_pShuffledCourses = m_pCourses;
	random_shuffle( m_pShuffledCourses.begin(), m_pShuffledCourses.end() );
}

void SongManager::SortSongs()
{
	SongUtil::SortSongPointerArrayByTitle( m_pSongs );
}

void SongManager::UpdateRankingCourses()
{
	/*  Updating the ranking courses data is fairly expensive
	 *  since it involves comparing strings.  Do so sparingly.
	 */
	CStringArray RankingCourses;

	split( THEME->GetMetric("ScreenRanking","CoursesToShow"),",", RankingCourses);

	for(unsigned i=0; i < m_pCourses.size(); i++)
	{
		if (m_pCourses[i]->GetEstimatedNumStages() > 7)
			m_pCourses[i]->m_SortOrder_Ranking = 3;
		else
			m_pCourses[i]->m_SortOrder_Ranking = 2;
		
		for(unsigned j = 0; j < RankingCourses.size(); j++)
			if (!RankingCourses[j].CompareNoCase(m_pCourses[i]->m_sPath))
				m_pCourses[i]->m_SortOrder_Ranking = 1;
	}
}

void SongManager::LoadAllFromProfiles()
{
	FOREACH_ProfileSlot( s )
	{
		if( !PROFILEMAN->IsUsingProfile(s) )
			continue;

		CString sProfileDir = PROFILEMAN->GetProfileDir( s );
		if( sProfileDir.empty() )
			continue;	// skip
		//
		// Load all .edit files.
		//
		{
			CString sEditsDir = sProfileDir+"Edits/";

			CStringArray asEditsFilesWithPath;
			GetDirListing( sEditsDir+"*.edit", asEditsFilesWithPath, false, true );

			unsigned size = min( asEditsFilesWithPath.size(), (unsigned)MAX_EDITS_PER_PROFILE );

			for( unsigned i=0; i<size; i++ )
			{
				CString fn = asEditsFilesWithPath[i];

				int iBytes = FILEMAN->GetFileSizeInBytes( fn );
				if( iBytes > MAX_EDIT_SIZE_BYTES )
				{
					LOG->Warn( "The file '%s' is unreasonably large.  It won't be loaded.", fn.c_str() );
					continue;
				}

				SMLoader::LoadEdit( fn, (ProfileSlot) s );
			}
		}
	}
}

void SongManager::FreeAllLoadedFromProfiles()
{
	for( unsigned s=0; s<m_pSongs.size(); s++ )
	{
		Song* pSong = m_pSongs[s];
		pSong->FreeAllLoadedFromProfiles();
	}
}

void SongManager::FreeAllPlayerSongs()
{
	//XXX: Ugly! Disgustingly ugly! Find a solution soon...
	bool bPlayerSongs = false;

	for( unsigned s=0; s<m_pSongs.size(); s++ )
	{
		Song* pSong = m_pSongs[s];

		if( pSong->IsPlayerSong() )
		{
			bPlayerSongs = true;
			break;			// We found a player song, so we have to reload anyways. Stop looking.
		}
	}

	if( bPlayerSongs )
	{
		m_pSongs.clear();

		for( unsigned i=0; i < m_pMachineSongs.size(); i++ )
			m_pSongs.push_back( m_pMachineSongs[i] );
	}
}

static bool CheckPointer( const Song *p )
{
	const vector<Song*> &songs = SONGMAN->GetAllSongs();
	for( unsigned i = 0; i < songs.size(); ++i )
		if( songs[i] == p )
			return true;
	return false;
}

void SongManager::InitRandomAttacks( LoadingWindow *ld )
{
	LOG->Trace( "Loading Random Attacks from file" );

	if( ld )
	{
		ld->SetText( ssprintf("Loading random attacks from file..."));
		ld->Paint();
	}
	
	if( !IsAFile(ATTACKS_PATH) )
		LOG->Trace( "RandomAttacks.dat was not found" );
	else
	{
		MsdFile msd;

		if( !msd.ReadFile( ATTACKS_PATH ) )
		{
			LOG->Warn( "Error opening file '%s' for reading: %s.", ATTACKS_PATH, msd.GetError().c_str() );
		}
		else
		{
			unsigned i;

			for( i=0; i<msd.GetNumValues(); i++ )
			{
				int iNumParams = msd.GetNumParams(i);
				const MsdFile::value_t &sParams = msd.GetValue(i);
				CString sType = sParams[0];
				CString sAttack = sParams[1];

				if( iNumParams > 2 )
				{
					LOG->Warn( "Got \"%s:%s\" tag with too many parameters", sType.c_str(), sAttack.c_str() );
					continue;
				}

				if( stricmp(sType,"ATTACK") != 0 )
				{
					LOG->Warn( "Got \"%s:%s\" tag with wrong declaration", sType.c_str(), sAttack.c_str() );
					continue;
				}

				// Check to make sure only one attack has been specified
				CStringArray sAttackCheck;

				split( sAttack, ",", sAttackCheck, false );

				if( sAttackCheck.size() > 1 )
				{
					LOG->Warn( "Attack \"%s\" has more than one modifier; must only be one modifier specified", sAttack.c_str() );
					continue;
				}

				// Drop the attack parameter into the GameState vector
				GAMESTATE->m_RandomAttacks.push_back( sAttack );
				LOG->Trace( "Random Attack \"%s\" loaded", sAttack.c_str() );
			}

			LOG->Trace( "Done loading Random Attacks from file" );
		}
	}
}

void SongManager::LoadListSortSongs( LoadingWindow *ld )
{
	LOG->Trace( "Loading List Sort order" );
	LOG->Trace( "Highest value found in #LISTSORT tag: %d.", m_iHighestListSortValue );

	if( ld )
	{
		ld->SetText( ssprintf("Loading list sort..."));
		ld->Paint();
	}

	m_pListSortSongs.clear();

	m_bListSortExists = false;
	int iLoadedSongs = 0;

	for( int i=0; i < m_iHighestListSortValue; i++ )
	{
		bool bFoundForThisValueOfI = false;
		CString sFoundOnSong = "";

		// Go right back to the start of the vector
		m_pMachineSongs.begin();

		// We're only looking at machine songs for the sort list, not player songs
		for( unsigned j=0; j < m_pMachineSongs.size(); j++ )
		{
			if( (i+1 == atoi(m_pMachineSongs[j]->m_sListSortPosition)) && !bFoundForThisValueOfI )
			{
				m_pListSortSongs.push_back( m_pMachineSongs[j] );
				iLoadedSongs++;

				bFoundForThisValueOfI = true;
				sFoundOnSong = m_pMachineSongs[j]->GetSongDir();
			}
			else if ( (i+1 == atoi(m_pMachineSongs[j]->m_sListSortPosition)) && bFoundForThisValueOfI )
			{
				LOG->Warn( "Already found List Sort song %d in '%s', but '%s' is also set to this position in the list sort. Will ignore second entry.",
							i+1, sFoundOnSong.c_str(), m_pMachineSongs[j]->GetSongDir().c_str() );
			}
		}

		if( !bFoundForThisValueOfI )
			LOG->Warn( "No song was found for List Sort song %d, but there should have been.", i+1 );
	}

	if( m_pListSortSongs.size() > 0 )
		m_bListSortExists = true;

	LOG->Trace( " Done loading List Sort order, %d songs will be displayed in this sort order.", iLoadedSongs );
}

#include "LuaFunctions.h"
#define LuaFunction_Song( func, call ) \
int LuaFunc_##func( lua_State *L ) { \
	REQ_ARGS( #func, 1 ); \
	REQ_ARG( #func, 1, lightuserdata ); \
	const Song *p = (const Song *) (lua_touserdata( L, -1 )); \
	LUA_ASSERT( CheckPointer(p), ssprintf("%p is not a valid song", p) ); \
	LUA_RETURN( call ); \
} \
LuaFunction( func ); /* register it */

LuaFunction_Str( Song, SONGMAN->FindSong( str ) );

LuaFunction_Song( SongFullDisplayTitle, p->GetFullDisplayTitle() );

static bool CheckPointer( const Steps *p )
{
	const vector<Song*> &songs = SONGMAN->GetAllSongs();
	for( unsigned i = 0; i < songs.size(); ++i )
	{
		const vector<Steps*>& vpSteps = songs[i]->GetAllSteps();
		for( unsigned j = 0; j < vpSteps.size(); ++j )
			if( vpSteps[j] == p )
				return true;
	}
	return false;
}

#define LuaFunction_Steps( func, call ) \
int LuaFunc_##func( lua_State *L ) { \
	REQ_ARGS( #func, 1 ); \
	REQ_ARG( #func, 1, lightuserdata ); \
	const Steps *p = (const Steps *) (lua_touserdata( L, -1 )); \
	LUA_ASSERT( CheckPointer(p), ssprintf("%p is not a valid steps", p) ); \
	LUA_RETURN( call ); \
} \
LuaFunction( func ); /* register it */
LuaFunction_Steps( StepsMeter, p->GetMeter() );

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
