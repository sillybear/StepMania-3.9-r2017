#include "global.h"
#include "PrefsManager.h"
#include "IniFile.h"
#include "GameState.h"
#include "RageDisplay.h"
#include "RageUtil.h"
#include "arch/arch.h" /* for default driver specs */
#include "RageSoundReader_Resample.h" /* for ResampleQuality */
#include "RageFile.h"
#include "ProductInfo.h"
#include "GameConstantsAndTypes.h"
#include "Foreach.h"
#include "Preference.h"

#define DEFAULTS_INI_PATH	"Data/Defaults.ini"		// these can be overridden
#define STEPMANIA_INI_PATH	"Data/StepMania.ini"		// overlay on Defaults.ini, contains the user's choices
#define STATIC_INI_PATH		"Data/Static.ini"		// overlay on the 2 above, can't be overridden

PrefsManager*	PREFSMAN = NULL;	// global and accessable from anywhere in our program

const float DEFAULT_SOUND_VOLUME = 1.00f;
const CString DEFAULT_LIGHTS_DRIVER = "Null";


//
// For self-registering prefs
//
vector<IPreference*> *g_pvpSubscribers = NULL;

void Subscribe( IPreference *p )
{
	// TRICKY: If we make this a global vector instead of a global pointer,
	// then we'd have to be careful that the static constructors of all
	// Preferences are called before the vector constructor.  It's
	// too tricky to enfore that, so we'll allocate the vector ourself
	// so that the compiler can't possibly call the vector constructor
	// after we've already added to the vector.
	if( g_pvpSubscribers == NULL )
		g_pvpSubscribers = new vector<IPreference*>;
	g_pvpSubscribers->push_back( p );
}

bool g_bAutoRestart = false;

PrefsManager::PrefsManager()
{
	Init();
	ReadGlobalPrefsFromDisk();
}

void PrefsManager::Init()
{
#ifdef DEBUG
	m_bWindowed = true;
#else
	m_bWindowed = false;
#endif

	m_bCatalogXML = true;
	m_iDisplayWidth = 640;
	m_iDisplayHeight = 480;
	m_iDisplayColorDepth = 16;
	m_iTextureColorDepth = 16;		// default to 16 for better preformance on slower cards
	m_iMovieColorDepth = 16;
	m_iMaxTextureResolution = 2048;
	m_iRefreshRate = REFRESH_DEFAULT;
	m_bOnlyDedicatedMenuButtons = false;
	m_bCelShadeModels = false;		// Work-In-Progress.. disable by default.
	m_fConstantUpdateDeltaSeconds = 0;

#ifdef DEBUG
	m_bShowStats = true;
#else
	m_bShowStats = false;
#endif
	m_bShowBanners = true ;
	m_bShowBackgrounds = true;
	m_BackgroundMode = BGMODE_ANIMATIONS;
	m_iNumBackgrounds = 8;
	m_bShowDanger = true;
	m_fBGBrightness = 0.8f;
	m_bMenuTimer = true;
	m_iNumArcadeStages = 3;
	m_bEventMode = false;
	m_bEventIgnoreSelectable = false;
	m_bEventIgnoreUnlock = false;
	m_bAutoPlay = false;
	m_fJudgeWindowScale = 1.0f;
	m_fJudgeWindowAdd = 0;
	m_fLifeDifficultyScale = 1.0f;

	m_bNoSavePlusFeatures = true;
	m_ConvertRatingsScaleDown = MTSCALE_OFF;

	m_bPositiveAnnouncerOnly = false;
	m_bEvalExtraAnnouncer = true;
	m_bGameExtraAnnouncer = true;

	m_fDebounceTime = 0.01f;

	m_bNonstopUsesExtremeScoring = false;

	m_bDanceRaveShufflesNotes = true;

	m_bRollTapTimingIsAffectedByJudge = true;
	m_fJudgeWindowSecondsMarvelous =		0.0225f;
	m_fJudgeWindowSecondsPerfect =			0.045f;
	m_fJudgeWindowSecondsGreat =			0.090f;
	m_fJudgeWindowSecondsGood =			0.135f;
	m_fJudgeWindowSecondsBoo =			0.180f;
	m_fJudgeWindowSecondsHidden =			0.045f;	// same as perfect
	m_fJudgeWindowSecondsHold =			0.250f;	// allow enough time to take foot off and put back on
	m_fJudgeWindowSecondsRoll =			0.300f;	// same judgment for HOLD_OK, but roll time is different - Mark
	m_fJudgeWindowSecondsMine =			0.090f;	// same as great
	m_fJudgeWindowSecondsAttack =			0.135f;

	// Life Meter, Normal Play
	m_fLifePercentInitialValue =			+0.500f;
	m_fLifeDeltaPercentChangeMarvelous =		+0.008f;
	m_fLifeDeltaPercentChangePerfect =		+0.008f;
	m_fLifeDeltaPercentChangeGreat =		+0.004f;
	m_fLifeDeltaPercentChangeGood =			+0.000f;
	m_fLifeDeltaPercentChangeBoo =			-0.040f;
	m_fLifeDeltaPercentChangeMiss =			-0.080f;
	m_fLifeDeltaPercentChangeHidden =		+0.250f;
	m_fLifeDeltaPercentChangeMissHidden =		-0.000f;
	m_fLifeDeltaPercentChangeHitMine =		-0.160f;
	m_fLifeDeltaPercentChangeHoldOK =		+0.008f;
	m_fLifeDeltaPercentChangeHoldNG =		-0.080f;
	m_fLifeDeltaPercentChangeRollOK =		+0.008f;
	m_fLifeDeltaPercentChangeRollNG =		-0.080f;

	// Life Meter, No Recover
	m_fLifePercentInitialValueNR =			+1.000f;
	m_fLifeDeltaPercentChangeMarvelousNR =		+0.000f;
	m_fLifeDeltaPercentChangePerfectNR =		+0.000f;
	m_fLifeDeltaPercentChangeGreatNR =		+0.000f;
	m_fLifeDeltaPercentChangeGoodNR =		+0.000f;
	m_fLifeDeltaPercentChangeBooNR =		-0.040f;
	m_fLifeDeltaPercentChangeMissNR =		-0.080f;
	m_fLifeDeltaPercentChangeHiddenNR =		+0.000f;
	m_fLifeDeltaPercentChangeMissHiddenNR =		-0.000f;
	m_fLifeDeltaPercentChangeHitMineNR =		-0.160f;
	m_fLifeDeltaPercentChangeHoldOKNR =		+0.000f;
	m_fLifeDeltaPercentChangeHoldNGNR =		-0.080f;
	m_fLifeDeltaPercentChangeRollOKNR =		+0.000f;
	m_fLifeDeltaPercentChangeRollNGNR =		-0.080f;

	// Life Meter, Sudden Death
	m_fLifePercentInitialValueSD =			+1.000f;
	m_fLifeDeltaPercentChangeMarvelousSD =		+0.000f;
	m_fLifeDeltaPercentChangePerfectSD =		+0.000f;
	m_fLifeDeltaPercentChangeGreatSD =		+0.000f;
	m_fLifeDeltaPercentChangeGoodSD =		-1.000f;
	m_fLifeDeltaPercentChangeBooSD =		-1.000f;
	m_fLifeDeltaPercentChangeMissSD =		-1.000f;
	m_fLifeDeltaPercentChangeHiddenSD =		+0.000f;
	m_fLifeDeltaPercentChangeMissHiddenSD =		-0.000f;
	m_fLifeDeltaPercentChangeHitMineSD =		-1.000f;
	m_fLifeDeltaPercentChangeHoldOKSD =		+0.000f;
	m_fLifeDeltaPercentChangeHoldNGSD =		-1.000f;
	m_fLifeDeltaPercentChangeRollOKSD =		+0.000f;
	m_fLifeDeltaPercentChangeRollNGSD =		-1.000f;

	m_fTugMeterPercentChangeMarvelous =		+0.010f;
	m_fTugMeterPercentChangePerfect =		+0.008f;
	m_fTugMeterPercentChangeGreat =			+0.004f;
	m_fTugMeterPercentChangeGood =			+0.000f;
	m_fTugMeterPercentChangeBoo =			-0.010f;
	m_fTugMeterPercentChangeMiss =			-0.020f;
	m_fTugMeterPercentChangeHidden =		+0.250f;
	m_fTugMeterPercentChangeMissHidden =		-0.000f;
	m_fTugMeterPercentChangeHitMine =		-0.040f;
	m_fTugMeterPercentChangeHoldOK =		+0.008f;
	m_fTugMeterPercentChangeHoldNG =		-0.020f;
	m_fTugMeterPercentChangeRollOK =		+0.008f;
	m_fTugMeterPercentChangeRollNG =		-0.020f;

	m_fSuperMeterPercentChangeMarvelous =		+0.05f;
	m_fSuperMeterPercentChangePerfect =		+0.04f;
	m_fSuperMeterPercentChangeGreat =		+0.02f;
	m_fSuperMeterPercentChangeGood =		+0.00f;
	m_fSuperMeterPercentChangeBoo =			-0.00f;
	m_fSuperMeterPercentChangeHidden =		+0.20f;
	m_fSuperMeterPercentChangeMissHidden =		-0.00f;
	m_fSuperMeterPercentChangeMiss =		-0.20f;
	m_fSuperMeterPercentChangeHitMine =		-0.40f;
	m_fSuperMeterPercentChangeHoldOK =		+0.04f;
	m_fSuperMeterPercentChangeHoldNG =		-0.20f;
	m_fSuperMeterPercentChangeRollOK =		+0.04f;
	m_fSuperMeterPercentChangeRollNG =		-0.20f;
	m_bMercifulSuperMeter = true;

	m_iRegenComboAfterFail = 10; // cumulative
	m_iRegenComboAfterMiss = 5; // cumulative
	m_iMaxRegenComboAfterFail = 10;
	m_iMaxRegenComboAfterMiss = 10;
	m_bTwoPlayerRecovery = true;
	m_bMercifulDrain = true;
	m_bMinimum1FullSongInCourses = false;

	m_bFadeVideoBackgrounds = false;

	m_bPlayAttackSounds = true;
	m_bPlayMineSound = true;

	m_fRandomAttackLength = 5.0f;
	m_fTimeBetweenRandomAttacks = 2.0f;
	m_fAttackMinesLength = 7.0f;
	
	m_iPercentScoreWeightMarvelous = 3;
	m_iPercentScoreWeightPerfect = 2;
	m_iPercentScoreWeightGreat = 1;
	m_iPercentScoreWeightGood = 0;
	m_iPercentScoreWeightBoo = 0;
	m_iPercentScoreWeightMiss = 0;
	m_iPercentScoreWeightHoldOK = 3;
	m_iPercentScoreWeightHoldNG = 0;
	m_iPercentScoreWeightHitMine = -2;
	m_iPercentScoreWeightRollOK = 3;
	m_iPercentScoreWeightRollNG = 0;

	m_iGradeWeightMarvelous = 2;
	m_iGradeWeightPerfect = 2;
	m_iGradeWeightGreat = 1;
	m_iGradeWeightGood = 0;
	m_iGradeWeightBoo = -4;
	m_iGradeWeightMiss = -8;
	m_iGradeWeightHitMine = -8;
	m_iGradeWeightHoldOK = 6;
	m_iGradeWeightHoldNG = 0;
	m_iGradeWeightRollOK = 6;
	m_iGradeWeightRollNG = 0;
	m_iNumGradeTiersUsed = 7;

	for( int i=0; i<NUM_GRADE_TIERS; i++ )
		m_fGradePercent[i] = 0;

	// Because the default mode is GM_DDR, we'll use it's values for the default
	m_fGradePercent[GRADE_TIER_1] = 1.0f;	// AAAA
	m_fGradePercent[GRADE_TIER_2] = 1.0f;	// AAA
	m_fGradePercent[GRADE_TIER_3] = 0.93f;	// AA
	m_fGradePercent[GRADE_TIER_4] = 0.80f;	// A
	m_fGradePercent[GRADE_TIER_5] = 0.65f;	// B
	m_fGradePercent[GRADE_TIER_6] = 0.45f;	// C
	m_fGradePercent[GRADE_TIER_7] = -99999;	// D
	m_bGradeTier02IsAllPerfects = true;
	m_bGradeTier02RequiresNoMiss = false;
	m_bGradeTier02RequiresFC = false;
	m_bGradeTier03RequiresNoMiss = false;
	m_bGradeTier03RequiresFC = false;

	m_bDelayedEscape = true;
	m_bInstructions = true;
	m_bShowDontDie = true;
	m_bShowSelectGroup = true;
	m_bShowNative = true;
	m_bArcadeOptionsNavigation = false;
	m_bSoloSingle = false;
	m_bDelayedTextureDelete = true;
	m_bTexturePreload = false;
	m_bDelayedScreenLoad = false;
	m_bDelayedModelDelete = false;
	m_BannerCache = BNCACHE_LOW_RES;
	m_bPalettedBannerCache = false;
	m_BackgroundCache = BGCACHE_LOW_RES;
	m_bPalettedBackgroundCache = false;
	m_bFastLoad = true;
	m_bFastLoadExistingCacheOnly = false;
	m_MusicWheelUsesSections = ALWAYS;
	m_iMusicWheelSwitchSpeed = 10;
	m_bEasterEggs = true;
	m_iMarvelousTiming = 2;
	m_iCoinMode = COIN_HOME;
	m_iCoinsPerCredit = 1;
	m_Premium = NO_PREMIUM;
	m_bDelayedCreditsReconcile = false;
	m_iBoostAppPriority = -1;
	m_bSmoothLines = false;
	m_ShowSongOptions = YES;
	m_bDancePointsForOni = false;
	m_bPercentageScoring = false;
	m_fMinPercentageForMachineSongHighScore = 0.5f;
	m_fMinPercentageForMachineCourseHighScore = 0.001f;	// don't save course scores with 0 percentage
	m_bDisqualification = false;
	m_bShowLyrics = true;
	m_bAutogenSteps = true;
	m_bAutogenGroupCourses = true;
	m_bBreakComboToGetItem = false;
	m_bLockCourseDifficulties = true;
	m_ShowDancingCharacters = CO_RANDOM;

	m_bUseUnlockSystem = false;
	m_bUnlockUsesMachineProfileStats = true;
	m_bUnlockUsesPlayerProfileStats = false;

	m_bFirstRun = true;
	m_bAutoMapOnJoyChange = true;
	m_fGlobalOffsetSeconds = 0;
	m_bShowBeginnerHelper = false;
	m_bEndlessBreakEnabled = true;
	m_iEndlessNumStagesUntilBreak = 5;
	m_iEndlessBreakLength = 5;
	m_bDisableScreenSaver = true;

	// set to 0 so people aren't shocked at first
	m_iProgressiveLifebar = 0;
	m_iProgressiveNonstopLifebar = 0;
	m_iProgressiveStageLifebar = 0;

	// DDR Extreme style extra stage support.
	// Default off so people used to the current behavior (or those with extra stage CRS files)
	// don't get it changed around on them.
	m_bLockExtraStageDiff = true;
	m_bPickExtraStage = false;
	m_bAlwaysAllowExtraStage2 = false;
	m_bPickModsForExtraStage = false;
	m_bDarkExtraStage = true;
	m_bOniExtraStage1 = true;
	m_bOniExtraStage2 = true;

	m_bComboContinuesBetweenSongs = false;

	// default to old sort order
	m_iCourseSortOrder = COURSE_SORT_SONGS;
	m_bMoveRandomToEnd = false;
	m_bSubSortByNumSteps = false;
	m_iScoringType = SCORING_MAX2;

	m_iGetRankingName = RANKING_ON;

	m_bUseLongSongs = true;
	m_bUseMarathonSongs = true;

	m_fLongVerSongSeconds = 60*2.5f;	// Dynamite Rave is 2:55
	m_fMarathonVerSongSeconds = 60*5.f;

	/* I'd rather get occasional people asking for support for this even though it's
	 * already here than lots of people asking why songs aren't being displayed. */
	m_bHiddenSongs = false;
	m_bVsync = true;
	m_sLanguage = "";	// ThemeManager will deal with this invalid language

	m_iCenterImageTranslateX = 0;
	m_iCenterImageTranslateY = 0;
	m_fCenterImageScaleX = 1;
	m_fCenterImageScaleY = 1;

	m_iAttractSoundFrequency = 1;
	m_bAllowExtraStage = true;
	m_bHideDefaultNoteSkin = false;
	m_iMaxHighScoresPerListForMachine = 10;
	m_iMaxHighScoresPerListForPlayer = 3;
	m_fPadStickSeconds = 0;
	m_bForceMipMaps = false;
	m_bTrilinearFiltering = false;
	m_bAnisotropicFiltering = false;
	g_bAutoRestart = false;
	m_bSignProfileData = false;

	m_bEditorShowBGChangesPlay = false;
	m_bEditShiftSelector = true;

	/* XXX: Set these defaults for individual consoles using VideoCardDefaults.ini. */
	m_bPAL = false;
#ifndef _XBOX
	m_bInterlaced = false;
#endif

	m_sSoundDrivers = "";	// default
	/* Number of frames to write ahead; usually 44100 frames per second.
	 * (Number of millisec would be more flexible, but it's more useful to
	 * specify numbers directly.) This is purely a troubleshooting option
	 * and is not honored by all sound drivers. */
	m_iSoundWriteAhead = 0;
	m_iSoundDevice = "";
	m_fSoundVolume = DEFAULT_SOUND_VOLUME;
	m_iSoundResampleQuality = RageSoundReader_Resample::RESAMP_NORMAL;

	m_sMovieDrivers = DEFAULT_MOVIE_DRIVER_LIST;

	// StepMania.cpp sets these on first run:
	m_sVideoRenderers = "";
#if defined(WIN32)
	m_iLastSeenMemory = 0;
#endif

	m_sLightsDriver = DEFAULT_LIGHTS_DRIVER;
	m_sLightsStepsDifficulty = "medium";
//CHANGE: Several lights options - Mark
	m_fLightsFalloffSeconds = 0.08f;
	m_bLightsFalloffUsesBPM = false;
	m_iLightsIgnoreHolds = 0;
	m_bBlinkGameplayButtonLightsOnNote = false;
	m_bAllowUnacceleratedRenderer = false;
	m_bThreadedInput = true;
	m_bThreadedMovieDecode = true;
	m_bScreenTestMode = false;
	m_sMachineName = "NoName";
	m_sIgnoredMessageWindows = "";

	m_sCoursesToShowRanking = "";

	m_bLogToDisk = true;
	m_bForceLogFlush = false;
#ifdef DEBUG
	m_bShowLogOutput = true;
#else
	m_bShowLogOutput = false;
#endif
	m_bTimestamping = false;
	m_bLogSkips = false;
	m_bLogCheckpoints = false;
	m_bShowLoadingWindow = true;

	m_bMemoryCards = false;
	
	// player song stuff
	m_bPlayerSongs = true;
	m_bPlayerSongsAllowBanners = true;
	m_bPlayerSongsAllowBackgrounds = true;
	m_bPlayerSongsAllowCDTitles = true;
	m_bPlayerSongsAllowLyrics = true;
	m_bPlayerSongsAllowBGChanges = true;
	m_fPlayerSongsLoadTimeout = 5.0f;
	m_fPlayerSongsLengthLimitSeconds = 150.0f;
	m_iPlayerSongsLoadLimit = 25;

	FOREACH_PlayerNumber( p )
	{
		m_iMemoryCardUsbBus[p] = -1;
		m_iMemoryCardUsbPort[p] = -1;
		m_iMemoryCardUsbLevel[p] = -1;
	}

	m_sMemoryCardProfileSubdir = PRODUCT_NAME;
	m_iProductID = 1;


	FOREACH_CONST( IPreference*, *g_pvpSubscribers, p ) (*p)->LoadDefault();
}

PrefsManager::~PrefsManager()
{
}

void PrefsManager::ReadGlobalPrefsFromDisk()
{
	ReadPrefsFromFile( DEFAULTS_INI_PATH );
	ReadPrefsFromFile( STEPMANIA_INI_PATH );
	ReadPrefsFromFile( STATIC_INI_PATH );
}

void PrefsManager::ResetToFactoryDefaults()
{
	// clobber the users prefs by initing then applying defaults
	Init();
	m_bFirstRun = false;
	ReadPrefsFromFile( DEFAULTS_INI_PATH );
	ReadPrefsFromFile( STATIC_INI_PATH );
	
	SaveGlobalPrefsToDisk();
}

void PrefsManager::ReadPrefsFromFile( CString sIni )
{
	IniFile ini;
	if( !ini.ReadFile(sIni) )
		return;

	ini.GetValue( "Options", "UseCatalogXML",				m_bCatalogXML );
	ini.GetValue( "Options", "Windowed",					m_bWindowed );
	ini.GetValue( "Options", "Interlaced",					m_bInterlaced );
	ini.GetValue( "Options", "PAL",						m_bPAL );
	ini.GetValue( "Options", "CelShadeModels",				m_bCelShadeModels );
	ini.GetValue( "Options", "ConstantUpdateDeltaSeconds",			m_fConstantUpdateDeltaSeconds );
	ini.GetValue( "Options", "DisplayWidth",				m_iDisplayWidth );
	ini.GetValue( "Options", "DisplayHeight",				m_iDisplayHeight );
	ini.GetValue( "Options", "DisplayColorDepth",				m_iDisplayColorDepth );
	ini.GetValue( "Options", "TextureColorDepth",				m_iTextureColorDepth );
	ini.GetValue( "Options", "MovieColorDepth",				m_iMovieColorDepth );
	ini.GetValue( "Options", "MaxTextureResolution",			m_iMaxTextureResolution );
	ini.GetValue( "Options", "RefreshRate",					m_iRefreshRate );
	ini.GetValue( "Options", "UseDedicatedMenuButtons",			m_bOnlyDedicatedMenuButtons );
	ini.GetValue( "Options", "ShowStats",					m_bShowStats );
	ini.GetValue( "Options", "ShowBanners",					m_bShowBanners );
	ini.GetValue( "Options", "ShowBackgroundsInSSM",			m_bShowBackgrounds);
	ini.GetValue( "Options", "BackgroundMode",				(int&)m_BackgroundMode );
	ini.GetValue( "Options", "NumBackgrounds",				m_iNumBackgrounds);
	ini.GetValue( "Options", "ShowDanger",					m_bShowDanger );
	ini.GetValue( "Options", "BGBrightness",				m_fBGBrightness );
	ini.GetValue( "Options", "MenuTimer",					m_bMenuTimer );
	ini.GetValue( "Options", "NumArcadeStages",				m_iNumArcadeStages );
	ini.GetValue( "Options", "EventMode",					m_bEventMode );
	ini.GetValue( "Options", "EventModeIgnoresSelectable",			m_bEventIgnoreSelectable );
	ini.GetValue( "Options", "EventModeIgnoresUnlock",			m_bEventIgnoreUnlock );
	ini.GetValue( "Options", "AutoPlay",					m_bAutoPlay );
	ini.GetValue( "Options", "RollTapTimingIsAffectedByJudge",		m_bRollTapTimingIsAffectedByJudge );
	ini.GetValue( "Options", "JudgeWindowScale",				m_fJudgeWindowScale );
	ini.GetValue( "Options", "JudgeWindowAdd",				m_fJudgeWindowAdd );
	ini.GetValue( "Options", "JudgeWindowSecondsMarvelous",			m_fJudgeWindowSecondsMarvelous );
	ini.GetValue( "Options", "JudgeWindowSecondsPerfect",			m_fJudgeWindowSecondsPerfect );
	ini.GetValue( "Options", "JudgeWindowSecondsGreat",			m_fJudgeWindowSecondsGreat );
	ini.GetValue( "Options", "JudgeWindowSecondsGood",			m_fJudgeWindowSecondsGood );
	ini.GetValue( "Options", "JudgeWindowSecondsBoo",			m_fJudgeWindowSecondsBoo );
	ini.GetValue( "Options", "JudgeWindowSecondsHold",			m_fJudgeWindowSecondsHold );
	ini.GetValue( "Options", "JudgeWindowSecondsRoll",			m_fJudgeWindowSecondsRoll );
	ini.GetValue( "Options", "JudgeWindowSecondsMine",			m_fJudgeWindowSecondsMine );
	ini.GetValue( "Options", "JudgeWindowSecondsAttack",			m_fJudgeWindowSecondsAttack );
	ini.GetValue( "Options", "JudgeWindowSecondsHidden",			m_fJudgeWindowSecondsHidden );
	ini.GetValue( "Options", "LifeDifficultyScale",				m_fLifeDifficultyScale );

	ini.GetValue( "Options", "PositiveAnnouncerOnly",			m_bPositiveAnnouncerOnly );
	ini.GetValue( "Options", "AnnouncerInExtraStageGameplay",		m_bGameExtraAnnouncer );
	ini.GetValue( "Options", "AnnouncerInExtraStageEvaluation",		m_bEvalExtraAnnouncer );

	ini.GetValue( "Options", "DebounceTime",				m_fDebounceTime );

	ini.GetValue( "Options", "NonstopUsesExtremeScoring",			m_bNonstopUsesExtremeScoring );
	ini.GetValue( "Options", "EditModeDoesntSavePlusFeatures",		m_bNoSavePlusFeatures );

	ini.GetValue( "Options", "ScaleMeterRatings",				(int&)m_ConvertRatingsScaleDown );

	// Life Meter, Normal Play
	ini.GetValue( "Options", "LifePercentInitialValue",			m_fLifePercentInitialValue );
	ini.GetValue( "Options", "LifeDeltaPercentChangeMarvelous",		m_fLifeDeltaPercentChangeMarvelous );
	ini.GetValue( "Options", "LifeDeltaPercentChangePerfect",		m_fLifeDeltaPercentChangePerfect );
	ini.GetValue( "Options", "LifeDeltaPercentChangeGreat",			m_fLifeDeltaPercentChangeGreat );
	ini.GetValue( "Options", "LifeDeltaPercentChangeGood",			m_fLifeDeltaPercentChangeGood );
	ini.GetValue( "Options", "LifeDeltaPercentChangeBoo",			m_fLifeDeltaPercentChangeBoo );
	ini.GetValue( "Options", "LifeDeltaPercentChangeMiss",			m_fLifeDeltaPercentChangeMiss );
	ini.GetValue( "Options", "LifeDeltaPercentChangeHidden",		m_fLifeDeltaPercentChangeHidden );
	ini.GetValue( "Options", "LifeDeltaPercentChangeMissHidden",		m_fLifeDeltaPercentChangeMissHidden );
	ini.GetValue( "Options", "LifeDeltaPercentChangeHitMine",		m_fLifeDeltaPercentChangeHitMine );
	ini.GetValue( "Options", "LifeDeltaPercentChangeHoldOK",		m_fLifeDeltaPercentChangeHoldOK );
	ini.GetValue( "Options", "LifeDeltaPercentChangeHoldNG",		m_fLifeDeltaPercentChangeHoldNG );
	ini.GetValue( "Options", "LifeDeltaPercentChangeRollOK",		m_fLifeDeltaPercentChangeRollOK );
	ini.GetValue( "Options", "LifeDeltaPercentChangeRollNG",		m_fLifeDeltaPercentChangeRollNG );

	// Life Meter, No Recover
	ini.GetValue( "Options", "LifePercentNoRecoverInitialValue",		m_fLifePercentInitialValueNR );
	ini.GetValue( "Options", "LifeDeltaPercentNoRecoverChangeMarvelous",	m_fLifeDeltaPercentChangeMarvelousNR );
	ini.GetValue( "Options", "LifeDeltaPercentNoRecoverChangePerfect",	m_fLifeDeltaPercentChangePerfectNR );
	ini.GetValue( "Options", "LifeDeltaPercentNoRecoverChangeGreat",	m_fLifeDeltaPercentChangeGreatNR );
	ini.GetValue( "Options", "LifeDeltaPercentNoRecoverChangeGood",		m_fLifeDeltaPercentChangeGoodNR );
	ini.GetValue( "Options", "LifeDeltaPercentNoRecoverChangeBoo",		m_fLifeDeltaPercentChangeBooNR );
	ini.GetValue( "Options", "LifeDeltaPercentNoRecoverChangeMiss",		m_fLifeDeltaPercentChangeMissNR );
	ini.GetValue( "Options", "LifeDeltaPercentNoRecoverChangeHidden",	m_fLifeDeltaPercentChangeHiddenNR );
	ini.GetValue( "Options", "LifeDeltaPercentNoRecoverChangeMissHidden",	m_fLifeDeltaPercentChangeMissHiddenNR );
	ini.GetValue( "Options", "LifeDeltaPercentNoRecoverChangeHitMine",	m_fLifeDeltaPercentChangeHitMineNR );
	ini.GetValue( "Options", "LifeDeltaPercentNoRecoverChangeHoldOK",	m_fLifeDeltaPercentChangeHoldOKNR );
	ini.GetValue( "Options", "LifeDeltaPercentNoRecoverChangeHoldNG",	m_fLifeDeltaPercentChangeHoldNGNR );
	ini.GetValue( "Options", "LifeDeltaPercentNoRecoverChangeRollOK",	m_fLifeDeltaPercentChangeRollOKNR );
	ini.GetValue( "Options", "LifeDeltaPercentNoRecoverChangeRollNG",	m_fLifeDeltaPercentChangeRollNGNR );

	// Life Meter, Sudden Death
	ini.GetValue( "Options", "LifePercentSuddenDeathInitialValue",			m_fLifePercentInitialValueSD );
	ini.GetValue( "Options", "LifeDeltaPercentSuddenDeathChangeMarvelous",		m_fLifeDeltaPercentChangeMarvelousSD );
	ini.GetValue( "Options", "LifeDeltaPercentSuddenDeathChangePerfect",		m_fLifeDeltaPercentChangePerfectSD );
	ini.GetValue( "Options", "LifeDeltaPercentSuddenDeathChangeGreat",		m_fLifeDeltaPercentChangeGreatSD );
	ini.GetValue( "Options", "LifeDeltaPercentSuddenDeathChangeGood",		m_fLifeDeltaPercentChangeGoodSD );
	ini.GetValue( "Options", "LifeDeltaPercentSuddenDeathChangeBoo",		m_fLifeDeltaPercentChangeBooSD );
	ini.GetValue( "Options", "LifeDeltaPercentSuddenDeathChangeMiss",		m_fLifeDeltaPercentChangeMissSD );
	ini.GetValue( "Options", "LifeDeltaPercentSuddenDeathChangeHidden",		m_fLifeDeltaPercentChangeHiddenSD );
	ini.GetValue( "Options", "LifeDeltaPercentSuddenDeathChangeMissHidden",		m_fLifeDeltaPercentChangeMissHiddenSD );
	ini.GetValue( "Options", "LifeDeltaPercentSuddenDeathChangeHitMine",		m_fLifeDeltaPercentChangeHitMineSD );
	ini.GetValue( "Options", "LifeDeltaPercentSuddenDeathChangeHoldOK",		m_fLifeDeltaPercentChangeHoldOKSD );
	ini.GetValue( "Options", "LifeDeltaPercentSuddenDeathChangeHoldNG",		m_fLifeDeltaPercentChangeHoldNGSD );
	ini.GetValue( "Options", "LifeDeltaPercentSuddenDeathChangeRollOK",		m_fLifeDeltaPercentChangeRollOKSD );
	ini.GetValue( "Options", "LifeDeltaPercentSuddenDeathChangeRollNG",		m_fLifeDeltaPercentChangeRollNGSD );

	ini.GetValue( "Options", "TugMeterPercentChangeMarvelous",		m_fTugMeterPercentChangeMarvelous );
	ini.GetValue( "Options", "TugMeterPercentChangePerfect",		m_fTugMeterPercentChangePerfect );
	ini.GetValue( "Options", "TugMeterPercentChangeGreat",			m_fTugMeterPercentChangeGreat );
	ini.GetValue( "Options", "TugMeterPercentChangeGood",			m_fTugMeterPercentChangeGood );
	ini.GetValue( "Options", "TugMeterPercentChangeBoo",			m_fTugMeterPercentChangeBoo );
	ini.GetValue( "Options", "TugMeterPercentChangeMiss",			m_fTugMeterPercentChangeMiss );
	ini.GetValue( "Options", "TugMeterPercentChangeHidden",			m_fTugMeterPercentChangeHidden );
	ini.GetValue( "Options", "TugMeterPercentChangeMissHidden",		m_fTugMeterPercentChangeMissHidden );
	ini.GetValue( "Options", "TugMeterPercentChangeHitMine",		m_fTugMeterPercentChangeHitMine );
	ini.GetValue( "Options", "TugMeterPercentChangeHoldOK",			m_fTugMeterPercentChangeHoldOK );
	ini.GetValue( "Options", "TugMeterPercentChangeHoldNG",			m_fTugMeterPercentChangeHoldNG );
	ini.GetValue( "Options", "TugMeterPercentChangeRollOK",			m_fTugMeterPercentChangeRollOK );
	ini.GetValue( "Options", "TugMeterPercentChangeRollNG",			m_fTugMeterPercentChangeRollNG );

	ini.GetValue( "Options", "SuperMeterPercentChangeMarvelous",		m_fSuperMeterPercentChangeMarvelous );
	ini.GetValue( "Options", "SuperMeterPercentChangePerfect",		m_fSuperMeterPercentChangePerfect );
	ini.GetValue( "Options", "SuperMeterPercentChangeGreat",		m_fSuperMeterPercentChangeGreat );
	ini.GetValue( "Options", "SuperMeterPercentChangeGood",			m_fSuperMeterPercentChangeGood );
	ini.GetValue( "Options", "SuperMeterPercentChangeBoo",			m_fSuperMeterPercentChangeBoo );
	ini.GetValue( "Options", "SuperMeterPercentChangeMiss",			m_fSuperMeterPercentChangeMiss );
	ini.GetValue( "Options", "SuperMeterPercentChangeHidden",		m_fSuperMeterPercentChangeHidden );
	ini.GetValue( "Options", "SuperMeterPercentChangeMissHidden",		m_fSuperMeterPercentChangeMissHidden );
	ini.GetValue( "Options", "SuperMeterPercentChangeHitMine",		m_fSuperMeterPercentChangeHitMine );
	ini.GetValue( "Options", "SuperMeterPercentChangeHoldOK",		m_fSuperMeterPercentChangeHoldOK );
	ini.GetValue( "Options", "SuperMeterPercentChangeHoldNG",		m_fSuperMeterPercentChangeHoldNG );
	ini.GetValue( "Options", "SuperMeterPercentChangeRollOK",		m_fSuperMeterPercentChangeRollOK );
	ini.GetValue( "Options", "SuperMeterPercentChangeRollNG",		m_fSuperMeterPercentChangeRollNG );
	ini.GetValue( "Options", "MercifulSuperMeter",				m_bMercifulSuperMeter );

	ini.GetValue( "Options", "RegenComboAfterFail",				m_iRegenComboAfterFail );
	ini.GetValue( "Options", "RegenComboAfterMiss",				m_iRegenComboAfterMiss );
	ini.GetValue( "Options", "MaxRegenComboAfterFail",			m_iMaxRegenComboAfterFail );
	ini.GetValue( "Options", "MaxRegenComboAfterMiss",			m_iMaxRegenComboAfterMiss );
	ini.GetValue( "Options", "TwoPlayerRecovery",				m_bTwoPlayerRecovery );
	ini.GetValue( "Options", "MercifulDrain",				m_bMercifulDrain );
	ini.GetValue( "Options", "Minimum1FullSongInCourses",			m_bMinimum1FullSongInCourses );

	ini.GetValue( "Options", "FadeBackgroundsOnTransition",			m_bFadeVideoBackgrounds );

	ini.GetValue( "Options", "PlayAttackSounds",				m_bPlayAttackSounds );
	ini.GetValue( "Options", "PlayMineSound",				m_bPlayMineSound );
	ini.GetValue( "Options", "RandomAttacksLength",				m_fRandomAttackLength );
	ini.GetValue( "Options", "TimeBetweenRandomAttacks",			m_fTimeBetweenRandomAttacks );
	ini.GetValue( "Options", "AttackMinesLength",				m_fAttackMinesLength );

	ini.GetValue( "Options", "ShuffleNotesInDanceRave",			m_bDanceRaveShufflesNotes );

	ini.GetValue( "Options", "PercentScoreWeightMarvelous",			m_iPercentScoreWeightMarvelous );
	ini.GetValue( "Options", "PercentScoreWeightPerfect",			m_iPercentScoreWeightPerfect );
	ini.GetValue( "Options", "PercentScoreWeightGreat",			m_iPercentScoreWeightGreat );
	ini.GetValue( "Options", "PercentScoreWeightGood",			m_iPercentScoreWeightGood );
	ini.GetValue( "Options", "PercentScoreWeightBoo",			m_iPercentScoreWeightBoo );
	ini.GetValue( "Options", "PercentScoreWeightMiss",			m_iPercentScoreWeightMiss );
	ini.GetValue( "Options", "PercentScoreWeightHoldOK",			m_iPercentScoreWeightHoldOK );
	ini.GetValue( "Options", "PercentScoreWeightHoldNG",			m_iPercentScoreWeightHoldNG );
	ini.GetValue( "Options", "PercentScoreWeightRollOK",			m_iPercentScoreWeightRollOK );
	ini.GetValue( "Options", "PercentScoreWeightRollNG",			m_iPercentScoreWeightRollNG );
	ini.GetValue( "Options", "PercentScoreWeightHitMine",			m_iPercentScoreWeightHitMine );

	//ini.GetValue( "Options", "GradeMode",					m_GradeMode );
	ini.GetValue( "Options", "GradeWeightMarvelous",			m_iGradeWeightMarvelous );
	ini.GetValue( "Options", "GradeWeightPerfect",				m_iGradeWeightPerfect );
	ini.GetValue( "Options", "GradeWeightGreat",				m_iGradeWeightGreat );
	ini.GetValue( "Options", "GradeWeightGood",				m_iGradeWeightGood );
	ini.GetValue( "Options", "GradeWeightBoo",				m_iGradeWeightBoo );
	ini.GetValue( "Options", "GradeWeightMiss",				m_iGradeWeightMiss );
	ini.GetValue( "Options", "GradeWeightHitMine",				m_iGradeWeightHitMine );
	ini.GetValue( "Options", "GradeWeightHoldOK",				m_iGradeWeightHoldOK );
	ini.GetValue( "Options", "GradeWeightHoldNG",				m_iGradeWeightHoldNG );
	ini.GetValue( "Options", "GradeWeightRollOK",				m_iGradeWeightRollOK );
	ini.GetValue( "Options", "GradeWeightRollNG",				m_iGradeWeightRollNG );

	ini.GetValue( "Options", "NumGradeTiersUsed",				m_iNumGradeTiersUsed );
	for( int g=0; g<NUM_GRADE_TIERS; g++ )
	{
		Grade grade = (Grade)g;
		CString s = GradeToString( grade );
		ini.GetValue( "Options", "GradePercent"+s,			m_fGradePercent[g] );
	}

	ini.GetValue( "Options", "GradeTier02IsAllPerfects",			m_bGradeTier02IsAllPerfects );
	ini.GetValue( "Options", "GradeTier02RequiresNoMiss",			m_bGradeTier02RequiresNoMiss );
	ini.GetValue( "Options", "GradeTier02RequiresFC",			m_bGradeTier02RequiresFC );
	ini.GetValue( "Options", "GradeTier03RequiresNoMiss",			m_bGradeTier03RequiresNoMiss );
	ini.GetValue( "Options", "GradeTier03RequiresFC",			m_bGradeTier03RequiresFC );

	ini.GetValue( "Options", "DelayedEscape",				m_bDelayedEscape );
	ini.GetValue( "Options", "HiddenSongs",					m_bHiddenSongs );
	ini.GetValue( "Options", "Vsync",					m_bVsync );
	ini.GetValue( "Options", "HowToPlay",					m_bInstructions );
	ini.GetValue( "Options", "Caution",					m_bShowDontDie );
	ini.GetValue( "Options", "ShowSelectGroup",				m_bShowSelectGroup );
	ini.GetValue( "Options", "ShowNative",					m_bShowNative );
	ini.GetValue( "Options", "ArcadeOptionsNavigation",			m_bArcadeOptionsNavigation );
	ini.GetValue( "Options", "DelayedTextureDelete",			m_bDelayedTextureDelete );
	ini.GetValue( "Options", "TexturePreload",				m_bTexturePreload );
	ini.GetValue( "Options", "DelayedScreenLoad",				m_bDelayedScreenLoad );
	ini.GetValue( "Options", "DelayedModelDelete",				m_bDelayedModelDelete );
	ini.GetValue( "Options", "BannerCache",					(int&)m_BannerCache );
	ini.GetValue( "Options", "PalettedBannerCache",				m_bPalettedBannerCache );
	ini.GetValue( "Options", "BackgroundCache",				(int&)m_BackgroundCache );
	ini.GetValue( "Options", "PalettedBackgroundCache",			m_bPalettedBackgroundCache );
	ini.GetValue( "Options", "FastLoad",					m_bFastLoad );
	ini.GetValue( "Options", "FastLoadExisitingCacheOnly",			m_bFastLoadExistingCacheOnly );
	ini.GetValue( "Options", "MusicWheelUsesSections",			(int&)m_MusicWheelUsesSections );
	ini.GetValue( "Options", "MusicWheelSwitchSpeed",			m_iMusicWheelSwitchSpeed );
	ini.GetValue( "Options", "SoundDrivers",				m_sSoundDrivers );
	ini.GetValue( "Options", "SoundWriteAhead",				m_iSoundWriteAhead );
	ini.GetValue( "Options", "SoundDevice",					m_iSoundDevice );
	ini.GetValue( "Options", "MovieDrivers",				m_sMovieDrivers );
	ini.GetValue( "Options", "EasterEggs",					m_bEasterEggs );
	ini.GetValue( "Options", "MarvelousTiming",				(int&)m_iMarvelousTiming );
	ini.GetValue( "Options", "SoundVolume",					m_fSoundVolume );
	ini.GetValue( "Options", "LightsDriver",				m_sLightsDriver );
	ini.GetValue( "Options", "SoundResampleQuality",			m_iSoundResampleQuality );
	ini.GetValue( "Options", "CoinMode",					m_iCoinMode );
	ini.GetValue( "Options", "CoinsPerCredit",				m_iCoinsPerCredit );

	m_iCoinsPerCredit = max(m_iCoinsPerCredit, 1);

	ini.GetValue( "Options", "Premium",					(int&)m_Premium );
	ini.GetValue( "Options", "DelayedCreditsReconcile",			m_bDelayedCreditsReconcile );
	ini.GetValue( "Options", "BoostAppPriority",				m_iBoostAppPriority );

	ini.GetValue( "Options", "LockExtraStageDifficulty",			m_bLockExtraStageDiff );
	ini.GetValue( "Options", "PickExtraStage",				m_bPickExtraStage );
	ini.GetValue( "Options", "AlwaysAllowExtraStage2",			m_bAlwaysAllowExtraStage2 );
	ini.GetValue( "Options", "PickModsForExtraStage",			m_bPickModsForExtraStage );
	ini.GetValue( "Options", "DarkExtraStage",				m_bDarkExtraStage );
	ini.GetValue( "Options", "OniExtraStage1",				m_bOniExtraStage1 );
	ini.GetValue( "Options", "OniExtraStage2",				m_bOniExtraStage2 );

	ini.GetValue( "Options", "ComboContinuesBetweenSongs",			m_bComboContinuesBetweenSongs );
	ini.GetValue( "Options", "UseLongSongs",				m_bUseLongSongs );
	ini.GetValue( "Options", "UseMarathonSongs",				m_bUseMarathonSongs );
	ini.GetValue( "Options", "LongVerSeconds",				m_fLongVerSongSeconds );
	ini.GetValue( "Options", "MarathonVerSeconds",				m_fMarathonVerSongSeconds );
	ini.GetValue( "Options", "ShowSongOptions",				(int&)m_ShowSongOptions );
	ini.GetValue( "Options", "LightsStepsDifficulty",			m_sLightsStepsDifficulty );
	ini.GetValue( "Options", "BlinkGameplayButtonLightsOnNote",		m_bBlinkGameplayButtonLightsOnNote );

	//CHANGE: Add lights stuff - Mark
	ini.GetValue( "Options", "LightsFalloffSeconds",			m_fLightsFalloffSeconds );	
	ini.GetValue( "Options", "LightsFalloffUsesBPM",			m_bLightsFalloffUsesBPM );	
	ini.GetValue( "Options", "LightsIgnoreHolds",				m_iLightsIgnoreHolds );

	ini.GetValue( "Options", "AllowUnacceleratedRenderer",			m_bAllowUnacceleratedRenderer );
	ini.GetValue( "Options", "ThreadedInput",				m_bThreadedInput );
	ini.GetValue( "Options", "ThreadedMovieDecode",				m_bThreadedMovieDecode );
	ini.GetValue( "Options", "ScreenTestMode",				m_bScreenTestMode );
	ini.GetValue( "Options", "MachineName",					m_sMachineName );
	ini.GetValue( "Options", "IgnoredMessageWindows",			m_sIgnoredMessageWindows );
	ini.GetValue( "Options", "SoloSingle",					m_bSoloSingle );
	ini.GetValue( "Options", "DancePointsForOni",				m_bDancePointsForOni );
	ini.GetValue( "Options", "PercentageScoring",				m_bPercentageScoring );
	ini.GetValue( "Options", "MinPercentageForMachineSongHighScore",	m_fMinPercentageForMachineSongHighScore );
	ini.GetValue( "Options", "MinPercentageForMachineCourseHighScore",	m_fMinPercentageForMachineCourseHighScore );
	ini.GetValue( "Options", "Disqualification",				m_bDisqualification );
	ini.GetValue( "Options", "ShowLyrics",					m_bShowLyrics );
	ini.GetValue( "Options", "AutogenSteps",				m_bAutogenSteps );
	ini.GetValue( "Options", "AutogenGroupCourses",				m_bAutogenGroupCourses );
	ini.GetValue( "Options", "BreakComboToGetItem",				m_bBreakComboToGetItem );
	ini.GetValue( "Options", "LockCourseDifficulties",			m_bLockCourseDifficulties );
	ini.GetValue( "Options", "ShowDancingCharacters",			(int&)m_ShowDancingCharacters );

	ini.GetValue( "Options", "CourseSortOrder",				(int&)m_iCourseSortOrder );
	ini.GetValue( "Options", "MoveRandomToEnd",				m_bMoveRandomToEnd );
	ini.GetValue( "Options", "SubSortByNumSteps",				m_bSubSortByNumSteps );

	ini.GetValue( "Options", "ScoringType",					(int&)m_iScoringType );

	ini.GetValue( "Options", "ProgressiveLifebar",				m_iProgressiveLifebar );
	ini.GetValue( "Options", "ProgressiveNonstopLifebar", 			m_iProgressiveNonstopLifebar );
	ini.GetValue( "Options", "ProgressiveStageLifebar",			m_iProgressiveStageLifebar );

	ini.GetValue( "Options", "UseUnlockSystem",				m_bUseUnlockSystem );
	ini.GetValue( "Options", "UnlockUsesMachineProfileStats",		m_bUnlockUsesMachineProfileStats );
	ini.GetValue( "Options", "UnlockUsesPlayerProfileStats",		m_bUnlockUsesPlayerProfileStats );

	ini.GetValue( "Options", "FirstRun",					m_bFirstRun );
	ini.GetValue( "Options", "AutoMapJoysticks",				m_bAutoMapOnJoyChange );
	ini.GetValue( "Options", "VideoRenderers",				m_sVideoRenderers );
	ini.GetValue( "Options", "LastSeenVideoDriver",				m_sLastSeenVideoDriver );
	ini.GetValue( "Options", "LastSeenInputDevices",			m_sLastSeenInputDevices );
#if defined(WIN32)
	ini.GetValue( "Options", "LastSeenMemory",				m_iLastSeenMemory );
#endif
	ini.GetValue( "Options", "CoursesToShowRanking",			m_sCoursesToShowRanking );
	ini.GetValue( "Options", "GetRankingName",				(int&)m_iGetRankingName);
	ini.GetValue( "Options", "SmoothLines",					m_bSmoothLines );
	ini.GetValue( "Options", "GlobalOffsetSeconds",				m_fGlobalOffsetSeconds );
	ini.GetValue( "Options", "ShowBeginnerHelper",				m_bShowBeginnerHelper );
	ini.GetValue( "Options", "Language",					m_sLanguage );
	ini.GetValue( "Options", "EndlessBreakEnabled",				m_bEndlessBreakEnabled );
	ini.GetValue( "Options", "EndlessStagesUntilBreak",			m_iEndlessNumStagesUntilBreak );
	ini.GetValue( "Options", "EndlessBreakLength",				m_iEndlessBreakLength );
	ini.GetValue( "Options", "DisableScreenSaver",				m_bDisableScreenSaver );

	ini.GetValue( "Options", "MemoryCardProfileSubdir",			m_sMemoryCardProfileSubdir );
	ini.GetValue( "Options", "ProductID",					m_iProductID );
	ini.GetValue( "Options", "MemoryCards",					m_bMemoryCards );
	FOREACH_PlayerNumber( p )
	{
		ini.GetValue( "Options", ssprintf("DefaultLocalProfileIDP%d",p+1),	m_sDefaultLocalProfileID[p] );
		ini.GetValue( "Options", ssprintf("MemoryCardOsMountPointP%d",p+1),	m_sMemoryCardOsMountPoint[p] );
		FixSlashesInPlace( m_sMemoryCardOsMountPoint[p] );
		ini.GetValue( "Options", ssprintf("MemoryCardUsbBusP%d",p+1),		m_iMemoryCardUsbBus[p] );
		ini.GetValue( "Options", ssprintf("MemoryCardUsbPortP%d",p+1),		m_iMemoryCardUsbPort[p] );
		ini.GetValue( "Options", ssprintf("MemoryCardUsbLevelP%d",p+1),		m_iMemoryCardUsbLevel[p] );
	}

	// Player Songs!
	ini.GetValue( "Options", "PlayerSongs",					m_bPlayerSongs );
	ini.GetValue( "Options", "PlayerSongsUseBanners",			m_bPlayerSongsAllowBanners );
	ini.GetValue( "Options", "PlayerSongsUseBackgrounds",			m_bPlayerSongsAllowBackgrounds );
	ini.GetValue( "Options", "PlayerSongsUseCDTitles",			m_bPlayerSongsAllowCDTitles );
	ini.GetValue( "Options", "PlayerSongsUseLyrics",			m_bPlayerSongsAllowLyrics );
	ini.GetValue( "Options", "PlayerSongsUseBGChanges",			m_bPlayerSongsAllowBGChanges );
	ini.GetValue( "Options", "PlayerSongsLoadTimeout",			m_fPlayerSongsLoadTimeout );
	ini.GetValue( "Options", "PlayerSongsLengthLimitInSeconds",		m_fPlayerSongsLengthLimitSeconds );
	ini.GetValue( "Options", "PlayerSongsLoadLimit",			m_iPlayerSongsLoadLimit );

	ini.GetValue( "Options", "CenterImageTranslateX",			m_iCenterImageTranslateX );
	ini.GetValue( "Options", "CenterImageTranslateY",			m_iCenterImageTranslateY );
	ini.GetValue( "Options", "CenterImageScaleX",				m_fCenterImageScaleX );
	ini.GetValue( "Options", "CenterImageScaleY",				m_fCenterImageScaleY );
	ini.GetValue( "Options", "AttractSoundFrequency",			m_iAttractSoundFrequency );
	ini.GetValue( "Options", "AllowExtraStage",				m_bAllowExtraStage );
	ini.GetValue( "Options", "HideDefaultNoteSkin",				m_bHideDefaultNoteSkin );
	ini.GetValue( "Options", "MaxHighScoresPerListForMachine",		m_iMaxHighScoresPerListForMachine );
	ini.GetValue( "Options", "MaxHighScoresPerListForPlayer",		m_iMaxHighScoresPerListForPlayer );
	ini.GetValue( "Options", "PadStickSeconds",				m_fPadStickSeconds );
	ini.GetValue( "Options", "ForceMipMaps",				m_bForceMipMaps );
	ini.GetValue( "Options", "TrilinearFiltering",				m_bTrilinearFiltering );
	ini.GetValue( "Options", "AnisotropicFiltering",			m_bAnisotropicFiltering );
	ini.GetValue( "Options", "AutoRestart",					g_bAutoRestart );
	ini.GetValue( "Options", "SignProfileData",				m_bSignProfileData );

	ini.GetValue( "Editor", "ShowBGChangesPlay",				m_bEditorShowBGChangesPlay );
	ini.GetValue( "Editor",	"ShiftIsUsedAsAreaSelector",			m_bEditShiftSelector );

	ini.GetValue( "Options", "AdditionalSongFolders",			m_sAdditionalSongFolders );
	ini.GetValue( "Options", "AdditionalFolders",				m_sAdditionalFolders );
	FixSlashesInPlace(m_sAdditionalSongFolders);
	FixSlashesInPlace(m_sAdditionalFolders);

	ini.GetValue( "Debug", "LogToDisk",					m_bLogToDisk );
	ini.GetValue( "Debug", "ForceLogFlush",					m_bForceLogFlush );
	ini.GetValue( "Debug", "ShowLogOutput",					m_bShowLogOutput );
	ini.GetValue( "Debug", "Timestamping",					m_bTimestamping );
	ini.GetValue( "Debug", "LogSkips",					m_bLogSkips );
	ini.GetValue( "Debug", "LogCheckpoints",				m_bLogCheckpoints );
	ini.GetValue( "Debug", "ShowLoadingWindow",				m_bShowLoadingWindow );


	FOREACH( IPreference*, *g_pvpSubscribers, p ) (*p)->ReadFrom( ini );
}

void PrefsManager::SaveGlobalPrefsToDisk() const
{
	IniFile ini;

	ini.SetValue( "Options", "UseCatalogXML",				m_bCatalogXML );
	ini.SetValue( "Options", "Windowed",					m_bWindowed );
	ini.SetValue( "Options", "CelShadeModels",				m_bCelShadeModels );
	ini.SetValue( "Options", "ConstantUpdateDeltaSeconds",			m_fConstantUpdateDeltaSeconds );
	ini.SetValue( "Options", "DisplayWidth",				m_iDisplayWidth );
	ini.SetValue( "Options", "DisplayHeight",				m_iDisplayHeight );
	ini.SetValue( "Options", "DisplayColorDepth",				m_iDisplayColorDepth );
	ini.SetValue( "Options", "TextureColorDepth",				m_iTextureColorDepth );
	ini.SetValue( "Options", "MovieColorDepth",				m_iMovieColorDepth );
	ini.SetValue( "Options", "MaxTextureResolution",			m_iMaxTextureResolution );
	ini.SetValue( "Options", "RefreshRate",					m_iRefreshRate );
	ini.SetValue( "Options", "UseDedicatedMenuButtons",			m_bOnlyDedicatedMenuButtons );
	ini.SetValue( "Options", "ShowStats",					m_bShowStats );
	ini.SetValue( "Options", "ShowBanners",					m_bShowBanners );
	ini.SetValue( "Options", "ShowBackgroundsInSSM",			m_bShowBackgrounds);
	ini.SetValue( "Options", "BackgroundMode",				m_BackgroundMode);
	ini.SetValue( "Options", "NumBackgrounds",				m_iNumBackgrounds);
	ini.SetValue( "Options", "ShowDanger",					m_bShowDanger );
	ini.SetValue( "Options", "BGBrightness",				m_fBGBrightness );
	ini.SetValue( "Options", "MenuTimer",					m_bMenuTimer );
	ini.SetValue( "Options", "NumArcadeStages",				m_iNumArcadeStages );
	ini.SetValue( "Options", "EventMode",					m_bEventMode );
	ini.SetValue( "Options", "EventModeIgnoresSelectable",			m_bEventIgnoreSelectable );
	ini.SetValue( "Options", "EventModeIgnoresUnlock",			m_bEventIgnoreUnlock );
	ini.SetValue( "Options", "AutoPlay",					m_bAutoPlay );
	ini.SetValue( "Options", "RollTapTimingIsAffectedByJudge",		m_bRollTapTimingIsAffectedByJudge );
	ini.SetValue( "Options", "JudgeWindowScale",				m_fJudgeWindowScale );
	ini.SetValue( "Options", "JudgeWindowAdd",				m_fJudgeWindowAdd );
	ini.SetValue( "Options", "JudgeWindowSecondsMarvelous",			m_fJudgeWindowSecondsMarvelous );
	ini.SetValue( "Options", "JudgeWindowSecondsPerfect",			m_fJudgeWindowSecondsPerfect );
	ini.SetValue( "Options", "JudgeWindowSecondsGreat",			m_fJudgeWindowSecondsGreat );
	ini.SetValue( "Options", "JudgeWindowSecondsGood",			m_fJudgeWindowSecondsGood );
	ini.SetValue( "Options", "JudgeWindowSecondsBoo",			m_fJudgeWindowSecondsBoo );
	ini.SetValue( "Options", "JudgeWindowSecondsHold",			m_fJudgeWindowSecondsHold );
	ini.SetValue( "Options", "JudgeWindowSecondsRoll",			m_fJudgeWindowSecondsRoll );
	ini.SetValue( "Options", "JudgeWindowSecondsMine",			m_fJudgeWindowSecondsMine );
	ini.SetValue( "Options", "JudgeWindowSecondsAttack",			m_fJudgeWindowSecondsAttack );
	ini.SetValue( "Options", "JudgeWindowSecondsHidden",			m_fJudgeWindowSecondsHidden );
	ini.SetValue( "Options", "LifeDifficultyScale",				m_fLifeDifficultyScale );

	ini.SetValue( "Options", "PositiveAnnouncerOnly",			m_bPositiveAnnouncerOnly );
	ini.SetValue( "Options", "AnnouncerInExtraStageGameplay",		m_bGameExtraAnnouncer );
	ini.SetValue( "Options", "AnnouncerInExtraStageEvaluation",		m_bEvalExtraAnnouncer );

	ini.SetValue( "Options", "DebounceTime",				m_fDebounceTime );

	ini.SetValue( "Options", "NonstopUsesExtremeScoring",			m_bNonstopUsesExtremeScoring );
	ini.SetValue( "Options", "EditModeDoesntSavePlusFeatures",		m_bNoSavePlusFeatures );

	ini.SetValue( "Options", "ScaleMeterRatings",				(int&)m_ConvertRatingsScaleDown );

	// Life Meter, Normal Play
	ini.SetValue( "Options", "LifePercentInitialValue",			m_fLifePercentInitialValue );
	ini.SetValue( "Options", "LifeDeltaPercentChangeMarvelous",		m_fLifeDeltaPercentChangeMarvelous );
	ini.SetValue( "Options", "LifeDeltaPercentChangePerfect",		m_fLifeDeltaPercentChangePerfect );
	ini.SetValue( "Options", "LifeDeltaPercentChangeGreat",			m_fLifeDeltaPercentChangeGreat );
	ini.SetValue( "Options", "LifeDeltaPercentChangeGood",			m_fLifeDeltaPercentChangeGood );
	ini.SetValue( "Options", "LifeDeltaPercentChangeBoo",			m_fLifeDeltaPercentChangeBoo );
	ini.SetValue( "Options", "LifeDeltaPercentChangeMiss",			m_fLifeDeltaPercentChangeMiss );
	ini.SetValue( "Options", "LifeDeltaPercentChangeHidden",		m_fLifeDeltaPercentChangeHidden );
	ini.SetValue( "Options", "LifeDeltaPercentChangeMissHidden",		m_fLifeDeltaPercentChangeMissHidden );
	ini.SetValue( "Options", "LifeDeltaPercentChangeHitMine",		m_fLifeDeltaPercentChangeHitMine );
	ini.SetValue( "Options", "LifeDeltaPercentChangeHoldOK",		m_fLifeDeltaPercentChangeHoldOK );
	ini.SetValue( "Options", "LifeDeltaPercentChangeHoldNG",		m_fLifeDeltaPercentChangeHoldNG );
	ini.SetValue( "Options", "LifeDeltaPercentChangeRollOK",		m_fLifeDeltaPercentChangeRollOK );
	ini.SetValue( "Options", "LifeDeltaPercentChangeRollNG",		m_fLifeDeltaPercentChangeRollNG );

	// Life Meter, No Recover
	ini.SetValue( "Options", "LifePercentNoRecoverInitialValue",		m_fLifePercentInitialValueNR );
	ini.SetValue( "Options", "LifeDeltaPercentNoRecoverChangeMarvelous",	m_fLifeDeltaPercentChangeMarvelousNR );
	ini.SetValue( "Options", "LifeDeltaPercentNoRecoverChangePerfect",	m_fLifeDeltaPercentChangePerfectNR );
	ini.SetValue( "Options", "LifeDeltaPercentNoRecoverChangeGreat",	m_fLifeDeltaPercentChangeGreatNR );
	ini.SetValue( "Options", "LifeDeltaPercentNoRecoverChangeGood",		m_fLifeDeltaPercentChangeGoodNR );
	ini.SetValue( "Options", "LifeDeltaPercentNoRecoverChangeBoo",		m_fLifeDeltaPercentChangeBooNR );
	ini.SetValue( "Options", "LifeDeltaPercentNoRecoverChangeMiss",		m_fLifeDeltaPercentChangeMissNR );
	ini.SetValue( "Options", "LifeDeltaPercentNoRecoverChangeHidden",	m_fLifeDeltaPercentChangeHiddenNR );
	ini.SetValue( "Options", "LifeDeltaPercentNoRecoverChangeMissHidden",	m_fLifeDeltaPercentChangeMissHiddenNR );
	ini.SetValue( "Options", "LifeDeltaPercentNoRecoverChangeHitMine",	m_fLifeDeltaPercentChangeHitMineNR );
	ini.SetValue( "Options", "LifeDeltaPercentNoRecoverChangeHoldOK",	m_fLifeDeltaPercentChangeHoldOKNR );
	ini.SetValue( "Options", "LifeDeltaPercentNoRecoverChangeHoldNG",	m_fLifeDeltaPercentChangeHoldNGNR );
	ini.SetValue( "Options", "LifeDeltaPercentNoRecoverChangeRollOK",	m_fLifeDeltaPercentChangeRollOKNR );
	ini.SetValue( "Options", "LifeDeltaPercentNoRecoverChangeRollNG",	m_fLifeDeltaPercentChangeRollNGNR );

	// Life Meter, Sudden Death
	ini.SetValue( "Options", "LifePercentSuddenDeathInitialValue",			m_fLifePercentInitialValueSD );
	ini.SetValue( "Options", "LifeDeltaPercentSuddenDeathChangeMarvelous",		m_fLifeDeltaPercentChangeMarvelousSD );
	ini.SetValue( "Options", "LifeDeltaPercentSuddenDeathChangePerfect",		m_fLifeDeltaPercentChangePerfectSD );
	ini.SetValue( "Options", "LifeDeltaPercentSuddenDeathChangeGreat",		m_fLifeDeltaPercentChangeGreatSD );
	ini.SetValue( "Options", "LifeDeltaPercentSuddenDeathChangeGood",		m_fLifeDeltaPercentChangeGoodSD );
	ini.SetValue( "Options", "LifeDeltaPercentSuddenDeathChangeBoo",		m_fLifeDeltaPercentChangeBooSD );
	ini.SetValue( "Options", "LifeDeltaPercentSuddenDeathChangeMiss",		m_fLifeDeltaPercentChangeMissSD );
	ini.SetValue( "Options", "LifeDeltaPercentSuddenDeathChangeHidden",		m_fLifeDeltaPercentChangeHiddenSD );
	ini.SetValue( "Options", "LifeDeltaPercentSuddenDeathChangeMissHidden",		m_fLifeDeltaPercentChangeMissHiddenSD );
	ini.SetValue( "Options", "LifeDeltaPercentSuddenDeathChangeHitMine",		m_fLifeDeltaPercentChangeHitMineSD );
	ini.SetValue( "Options", "LifeDeltaPercentSuddenDeathChangeHoldOK",		m_fLifeDeltaPercentChangeHoldOKSD );
	ini.SetValue( "Options", "LifeDeltaPercentSuddenDeathChangeHoldNG",		m_fLifeDeltaPercentChangeHoldNGSD );
	ini.SetValue( "Options", "LifeDeltaPercentSuddenDeathChangeRollOK",		m_fLifeDeltaPercentChangeRollOKSD );
	ini.SetValue( "Options", "LifeDeltaPercentSuddenDeathChangeRollNG",		m_fLifeDeltaPercentChangeRollNGSD );

	ini.SetValue( "Options", "TugMeterPercentChangeMarvelous",		m_fTugMeterPercentChangeMarvelous );
	ini.SetValue( "Options", "TugMeterPercentChangePerfect",		m_fTugMeterPercentChangePerfect );
	ini.SetValue( "Options", "TugMeterPercentChangeGreat",			m_fTugMeterPercentChangeGreat );
	ini.SetValue( "Options", "TugMeterPercentChangeGood",			m_fTugMeterPercentChangeGood );
	ini.SetValue( "Options", "TugMeterPercentChangeBoo",			m_fTugMeterPercentChangeBoo );
	ini.SetValue( "Options", "TugMeterPercentChangeMiss",			m_fTugMeterPercentChangeMiss );
	ini.SetValue( "Options", "TugMeterPercentChangeHidden",			m_fTugMeterPercentChangeHidden );
	ini.SetValue( "Options", "TugMeterPercentChangeMissHidden",		m_fTugMeterPercentChangeMissHidden );
	ini.SetValue( "Options", "TugMeterPercentChangeHitMine",		m_fTugMeterPercentChangeHitMine );
	ini.SetValue( "Options", "TugMeterPercentChangeHoldOK",			m_fTugMeterPercentChangeHoldOK );
	ini.SetValue( "Options", "TugMeterPercentChangeHoldNG",			m_fTugMeterPercentChangeHoldNG );
	ini.SetValue( "Options", "TugMeterPercentChangeRollOK",			m_fTugMeterPercentChangeRollOK );
	ini.SetValue( "Options", "TugMeterPercentChangeRollNG",			m_fTugMeterPercentChangeRollNG );

	ini.SetValue( "Options", "SuperMeterPercentChangeMarvelous",		m_fSuperMeterPercentChangeMarvelous );
	ini.SetValue( "Options", "SuperMeterPercentChangePerfect",		m_fSuperMeterPercentChangePerfect );
	ini.SetValue( "Options", "SuperMeterPercentChangeGreat",		m_fSuperMeterPercentChangeGreat );
	ini.SetValue( "Options", "SuperMeterPercentChangeGood",			m_fSuperMeterPercentChangeGood );
	ini.SetValue( "Options", "SuperMeterPercentChangeBoo",			m_fSuperMeterPercentChangeBoo );
	ini.SetValue( "Options", "SuperMeterPercentChangeMiss",			m_fSuperMeterPercentChangeMiss );
	ini.SetValue( "Options", "SuperMeterPercentChangeHidden",		m_fSuperMeterPercentChangeHidden );
	ini.SetValue( "Options", "SuperMeterPercentChangeMissHidden",		m_fSuperMeterPercentChangeMissHidden );
	ini.SetValue( "Options", "SuperMeterPercentChangeHitMine",		m_fSuperMeterPercentChangeHitMine );
	ini.SetValue( "Options", "SuperMeterPercentChangeHoldOK",		m_fSuperMeterPercentChangeHoldOK );
	ini.SetValue( "Options", "SuperMeterPercentChangeHoldNG",		m_fSuperMeterPercentChangeHoldNG );
	ini.SetValue( "Options", "SuperMeterPercentChangeRollOK",		m_fSuperMeterPercentChangeRollOK );
	ini.SetValue( "Options", "SuperMeterPercentChangeRollNG",		m_fSuperMeterPercentChangeRollNG );
	ini.SetValue( "Options", "MercifulSuperMeter",				m_bMercifulSuperMeter );

	ini.SetValue( "Options", "RegenComboAfterFail",				m_iRegenComboAfterFail );
	ini.SetValue( "Options", "RegenComboAfterMiss",				m_iRegenComboAfterMiss );
	ini.SetValue( "Options", "MaxRegenComboAfterFail",			m_iMaxRegenComboAfterFail );
	ini.SetValue( "Options", "MaxRegenComboAfterMiss",			m_iMaxRegenComboAfterMiss );
	ini.SetValue( "Options", "TwoPlayerRecovery",				m_bTwoPlayerRecovery );
	ini.SetValue( "Options", "MercifulDrain",				m_bMercifulDrain );
	ini.SetValue( "Options", "Minimum1FullSongInCourses",			m_bMinimum1FullSongInCourses );

	ini.SetValue( "Options", "FadeBackgroundsOnTransition",			m_bFadeVideoBackgrounds );

	ini.SetValue( "Options", "PlayAttackSounds",				m_bPlayAttackSounds );
	ini.SetValue( "Options", "PlayMineSound",				m_bPlayMineSound );

	ini.SetValue( "Options", "RandomAttacksLength",				m_fRandomAttackLength );
	ini.SetValue( "Options", "TimeBetweenRandomAttacks",			m_fTimeBetweenRandomAttacks );
	ini.SetValue( "Options", "AttackMinesLength",				m_fAttackMinesLength );

	ini.SetValue( "Options", "ShuffleNotesInDanceRave",			m_bDanceRaveShufflesNotes );

	ini.SetValue( "Options", "PercentScoreWeightMarvelous",			m_iPercentScoreWeightMarvelous );
	ini.SetValue( "Options", "PercentScoreWeightPerfect",			m_iPercentScoreWeightPerfect );
	ini.SetValue( "Options", "PercentScoreWeightGreat",			m_iPercentScoreWeightGreat );
	ini.SetValue( "Options", "PercentScoreWeightGood",			m_iPercentScoreWeightGood );
	ini.SetValue( "Options", "PercentScoreWeightBoo",			m_iPercentScoreWeightBoo );
	ini.SetValue( "Options", "PercentScoreWeightMiss",			m_iPercentScoreWeightMiss );
	ini.SetValue( "Options", "PercentScoreWeightHoldOK",			m_iPercentScoreWeightHoldOK );
	ini.SetValue( "Options", "PercentScoreWeightHoldNG",			m_iPercentScoreWeightHoldNG );
	ini.SetValue( "Options", "PercentScoreWeightRollOK",			m_iPercentScoreWeightRollOK );
	ini.SetValue( "Options", "PercentScoreWeightRollNG",			m_iPercentScoreWeightRollNG );
	ini.SetValue( "Options", "PercentScoreWeightHitMine",			m_iPercentScoreWeightHitMine );

	//ini.SetValue( "Options", "GradeMode",					m_GradeMode );
	ini.SetValue( "Options", "GradeWeightMarvelous",			m_iGradeWeightMarvelous );
	ini.SetValue( "Options", "GradeWeightPerfect",				m_iGradeWeightPerfect );
	ini.SetValue( "Options", "GradeWeightGreat",				m_iGradeWeightGreat );
	ini.SetValue( "Options", "GradeWeightGood",				m_iGradeWeightGood );
	ini.SetValue( "Options", "GradeWeightBoo",				m_iGradeWeightBoo );
	ini.SetValue( "Options", "GradeWeightMiss",				m_iGradeWeightMiss );
	ini.SetValue( "Options", "GradeWeightHitMine",				m_iGradeWeightHitMine );
	ini.SetValue( "Options", "GradeWeightHoldOK",				m_iGradeWeightHoldOK );
	ini.SetValue( "Options", "GradeWeightHoldNG",				m_iGradeWeightHoldNG );
	ini.SetValue( "Options", "GradeWeightRollOK",				m_iGradeWeightRollOK );
	ini.SetValue( "Options", "GradeWeightRollNG",				m_iGradeWeightRollNG );
	
	ini.SetValue( "Options", "NumGradeTiersUsed",				m_iNumGradeTiersUsed );
	for( int g=0; g<NUM_GRADE_TIERS; g++ )
	{
		Grade grade = (Grade)g;
		CString s = GradeToString( grade );
		ini.SetValue( "Options", "GradePercent"+s,			m_fGradePercent[g] );
	}
	ini.SetValue( "Options", "GradeTier02IsAllPerfects",			m_bGradeTier02IsAllPerfects );
	ini.SetValue( "Options", "GradeTier02RequiresNoMiss",			m_bGradeTier02RequiresNoMiss );
	ini.SetValue( "Options", "GradeTier02RequiresFC",			m_bGradeTier02RequiresFC );
	ini.SetValue( "Options", "GradeTier03RequiresNoMiss",			m_bGradeTier03RequiresNoMiss );
	ini.SetValue( "Options", "GradeTier03RequiresFC",			m_bGradeTier03RequiresFC );

	ini.SetValue( "Options", "DelayedEscape",				m_bDelayedEscape );
	ini.SetValue( "Options", "HiddenSongs",					m_bHiddenSongs );
	ini.SetValue( "Options", "Vsync",					m_bVsync );
	ini.SetValue( "Options", "Interlaced",					m_bInterlaced );
	ini.SetValue( "Options", "PAL",						m_bPAL );
	ini.SetValue( "Options", "HowToPlay",					m_bInstructions );
	ini.SetValue( "Options", "Caution",					m_bShowDontDie );
	ini.SetValue( "Options", "ShowSelectGroup",				m_bShowSelectGroup );
	ini.SetValue( "Options", "ShowNative",					m_bShowNative );
	ini.SetValue( "Options", "ArcadeOptionsNavigation",			m_bArcadeOptionsNavigation );
	ini.SetValue( "Options", "DelayedTextureDelete",			m_bDelayedTextureDelete );
	ini.SetValue( "Options", "TexturePreload",				m_bTexturePreload );
	ini.SetValue( "Options", "DelayedScreenLoad",				m_bDelayedScreenLoad );
	ini.SetValue( "Options", "DelayedModelDelete",				m_bDelayedModelDelete );
	ini.SetValue( "Options", "BannerCache",					m_BannerCache );
	ini.SetValue( "Options", "PalettedBannerCache",				m_bPalettedBannerCache );
	ini.SetValue( "Options", "BackgroundCache",				m_BackgroundCache );
	ini.SetValue( "Options", "PalettedBackgroundCache",			m_bPalettedBackgroundCache );
	ini.SetValue( "Options", "FastLoad",					m_bFastLoad );
	ini.SetValue( "Options", "FastLoadExisitingCacheOnly",			m_bFastLoadExistingCacheOnly );
	ini.SetValue( "Options", "MusicWheelUsesSections",			m_MusicWheelUsesSections );
	ini.SetValue( "Options", "MusicWheelSwitchSpeed",			m_iMusicWheelSwitchSpeed );
	ini.SetValue( "Options", "EasterEggs",					m_bEasterEggs );
	ini.SetValue( "Options", "MarvelousTiming",				m_iMarvelousTiming );
	ini.SetValue( "Options", "SoundResampleQuality",			m_iSoundResampleQuality );
	ini.SetValue( "Options", "CoinMode",					m_iCoinMode );
	ini.SetValue( "Options", "CoinsPerCredit",				m_iCoinsPerCredit );
	ini.SetValue( "Options", "Premium",					m_Premium );
	ini.SetValue( "Options", "DelayedCreditsReconcile",			m_bDelayedCreditsReconcile );
	ini.SetValue( "Options", "BoostAppPriority",				m_iBoostAppPriority );

	ini.SetValue( "Options", "LockExtraStageDifficulty",			m_bLockExtraStageDiff );
	ini.SetValue( "Options", "PickExtraStage",				m_bPickExtraStage );
	ini.SetValue( "Options", "AlwaysAllowExtraStage2",			m_bAlwaysAllowExtraStage2 );
	ini.SetValue( "Options", "PickModsForExtraStage",			m_bPickModsForExtraStage );
	ini.SetValue( "Options", "DarkExtraStage",				m_bDarkExtraStage );
	ini.SetValue( "Options", "OniExtraStage1",				m_bOniExtraStage1 );
	ini.SetValue( "Options", "OniExtraStage2",				m_bOniExtraStage2 );

	ini.SetValue( "Options", "ComboContinuesBetweenSongs",			m_bComboContinuesBetweenSongs );
	ini.SetValue( "Options", "UseLongSongs",				m_bUseLongSongs );
	ini.SetValue( "Options", "UseMarathonSongs",				m_bUseMarathonSongs );
	ini.SetValue( "Options", "LongVerSeconds",				m_fLongVerSongSeconds );
	ini.SetValue( "Options", "MarathonVerSeconds",				m_fMarathonVerSongSeconds );
	ini.SetValue( "Options", "ShowSongOptions",				m_ShowSongOptions );
	ini.SetValue( "Options", "LightsStepsDifficulty",			m_sLightsStepsDifficulty );
//CHANGE: Add lights stuff. - Mark
	ini.SetValue( "Options", "LightsFalloffSeconds",			m_fLightsFalloffSeconds );	
	ini.SetValue( "Options", "LightsFalloffUsesBPM",			m_bLightsFalloffUsesBPM );	
	ini.SetValue( "Options", "LightsIgnoreHolds",				m_iLightsIgnoreHolds );

	ini.SetValue( "Options", "BlinkGameplayButtonLightsOnNote",		m_bBlinkGameplayButtonLightsOnNote );
	ini.SetValue( "Options", "AllowUnacceleratedRenderer",			m_bAllowUnacceleratedRenderer );
	ini.SetValue( "Options", "ThreadedInput",				m_bThreadedInput );
	ini.SetValue( "Options", "ThreadedMovieDecode",				m_bThreadedMovieDecode );
	ini.SetValue( "Options", "ScreenTestMode",				m_bScreenTestMode );
	ini.SetValue( "Options", "MachineName",					m_sMachineName );
	ini.SetValue( "Options", "IgnoredMessageWindows",			m_sIgnoredMessageWindows );
	ini.SetValue( "Options", "SoloSingle",					m_bSoloSingle );
	ini.SetValue( "Options", "DancePointsForOni",				m_bDancePointsForOni );
	ini.SetValue( "Options", "PercentageScoring",				m_bPercentageScoring );
	ini.SetValue( "Options", "MinPercentageForMachineSongHighScore",	m_fMinPercentageForMachineSongHighScore );
	ini.SetValue( "Options", "MinPercentageForMachineCourseHighScore",	m_fMinPercentageForMachineCourseHighScore );
	ini.SetValue( "Options", "Disqualification",				m_bDisqualification );
	ini.SetValue( "Options", "ShowLyrics",					m_bShowLyrics );
	ini.SetValue( "Options", "AutogenSteps",				m_bAutogenSteps );
	ini.SetValue( "Options", "AutogenGroupCourses",				m_bAutogenGroupCourses );
	ini.SetValue( "Options", "BreakComboToGetItem",				m_bBreakComboToGetItem );
	ini.SetValue( "Options", "LockCourseDifficulties",			m_bLockCourseDifficulties );
	ini.SetValue( "Options", "ShowDancingCharacters",			m_ShowDancingCharacters );

	ini.SetValue( "Options", "UseUnlockSystem",				m_bUseUnlockSystem );
	ini.SetValue( "Options", "UnlockUsesMachineProfileStats",		m_bUnlockUsesMachineProfileStats );
	ini.SetValue( "Options", "UnlockUsesPlayerProfileStats",		m_bUnlockUsesPlayerProfileStats );

	ini.SetValue( "Options", "FirstRun",					m_bFirstRun );
	ini.SetValue( "Options", "AutoMapJoysticks",				m_bAutoMapOnJoyChange );
	ini.SetValue( "Options", "VideoRenderers",				m_sVideoRenderers );
	ini.SetValue( "Options", "LastSeenVideoDriver",				m_sLastSeenVideoDriver );
	ini.SetValue( "Options", "LastSeenInputDevices",			m_sLastSeenInputDevices );

#if defined(WIN32)
	ini.SetValue( "Options", "LastSeenMemory",				m_iLastSeenMemory );
#endif

	ini.SetValue( "Options", "CoursesToShowRanking",			m_sCoursesToShowRanking );
	ini.SetValue( "Options", "GetRankingName",				m_iGetRankingName);
	ini.SetValue( "Options", "SmoothLines",					m_bSmoothLines );
	ini.SetValue( "Options", "GlobalOffsetSeconds",				m_fGlobalOffsetSeconds );

	ini.SetValue( "Options", "CourseSortOrder",				m_iCourseSortOrder );
	ini.SetValue( "Options", "MoveRandomToEnd",				m_bMoveRandomToEnd );
	ini.SetValue( "Options", "SubSortByNumSteps",				m_bSubSortByNumSteps );

	ini.SetValue( "Options", "ScoringType",					m_iScoringType );

	ini.SetValue( "Options", "ProgressiveLifebar",				m_iProgressiveLifebar );
	ini.SetValue( "Options", "ProgressiveStageLifebar",			m_iProgressiveStageLifebar );
	ini.SetValue( "Options", "ProgressiveNonstopLifebar",			m_iProgressiveNonstopLifebar );
	ini.SetValue( "Options", "ShowBeginnerHelper",				m_bShowBeginnerHelper );
	ini.SetValue( "Options", "Language",					m_sLanguage );
	ini.SetValue( "Options", "EndlessBreakEnabled",				m_bEndlessBreakEnabled );
	ini.SetValue( "Options", "EndlessStagesUntilBreak",			m_iEndlessNumStagesUntilBreak );
	ini.SetValue( "Options", "EndlessBreakLength",				m_iEndlessBreakLength );
	ini.SetValue( "Options", "DisableScreenSaver",				m_bDisableScreenSaver );

	ini.SetValue( "Options", "MemoryCardProfileSubdir",			m_sMemoryCardProfileSubdir );
	ini.SetValue( "Options", "ProductID",					m_iProductID );
	ini.SetValue( "Options", "MemoryCards",					m_bMemoryCards );
	FOREACH_PlayerNumber( p )
	{
		ini.SetValue( "Options", ssprintf("DefaultLocalProfileIDP%d",p+1),	m_sDefaultLocalProfileID[p] );

		// For some reason, the mount path in windows gets mucked up. If you specify C:\ in the file, when 
		// it saves, it'll save as C:/. Very annoying and can break mount path data! This fixes that.
		if( m_sMemoryCardOsMountPoint[p].Right(1) == "/" )
		{
			CStringArray sHolder;
			CString sTemp;
						
			split( m_sMemoryCardOsMountPoint[p], "/", sHolder, false );

			for( unsigned j = 0; j < (sHolder.size() - 1); j++ )
			{
				sTemp += sHolder[j];
			}

			sTemp += "\\"; // Remember to escape characters!

			ini.SetValue( "Options", ssprintf("MemoryCardOsMountPointP%d",p+1),	sTemp );
		}
		else
		{
			ini.SetValue( "Options", ssprintf("MemoryCardOsMountPointP%d",p+1),	m_sMemoryCardOsMountPoint[p] );
		}

		ini.SetValue( "Options", ssprintf("MemoryCardUsbBusP%d",p+1),		m_iMemoryCardUsbBus[p] );
		ini.SetValue( "Options", ssprintf("MemoryCardUsbPortP%d",p+1),		m_iMemoryCardUsbPort[p] );
		ini.SetValue( "Options", ssprintf("MemoryCardUsbLevelP%d",p+1),		m_iMemoryCardUsbLevel[p] );
	}

	// Player Songs!
	ini.SetValue( "Options", "PlayerSongs",					m_bPlayerSongs );
	ini.SetValue( "Options", "PlayerSongsUseBanners",			m_bPlayerSongsAllowBanners );
	ini.SetValue( "Options", "PlayerSongsUseBackgrounds",			m_bPlayerSongsAllowBackgrounds );
	ini.SetValue( "Options", "PlayerSongsUseCDTitles",			m_bPlayerSongsAllowCDTitles );
	ini.SetValue( "Options", "PlayerSongsUseLyrics",			m_bPlayerSongsAllowLyrics );
	ini.SetValue( "Options", "PlayerSongsUseBGChanges",			m_bPlayerSongsAllowBGChanges );
	ini.SetValue( "Options", "PlayerSongsLoadTimeout",			m_fPlayerSongsLoadTimeout );
	ini.SetValue( "Options", "PlayerSongsLengthLimitInSeconds",		m_fPlayerSongsLengthLimitSeconds );
	ini.SetValue( "Options", "PlayerSongsLoadLimit",			m_iPlayerSongsLoadLimit );

	ini.SetValue( "Options", "CenterImageTranslateX",			m_iCenterImageTranslateX );
	ini.SetValue( "Options", "CenterImageTranslateY",			m_iCenterImageTranslateY );
	ini.SetValue( "Options", "CenterImageScaleX",				m_fCenterImageScaleX );
	ini.SetValue( "Options", "CenterImageScaleY",				m_fCenterImageScaleY );
	ini.SetValue( "Options", "AttractSoundFrequency",			m_iAttractSoundFrequency );
	ini.SetValue( "Options", "AllowExtraStage",				m_bAllowExtraStage );
	ini.SetValue( "Options", "HideDefaultNoteSkin",				m_bHideDefaultNoteSkin );
	ini.SetValue( "Options", "MaxHighScoresPerListForMachine",		m_iMaxHighScoresPerListForMachine );
	ini.SetValue( "Options", "MaxHighScoresPerListForPlayer",		m_iMaxHighScoresPerListForPlayer );
	ini.SetValue( "Options", "PadStickSeconds",				m_fPadStickSeconds );
	ini.SetValue( "Options", "ForceMipMaps",				m_bForceMipMaps );
	ini.SetValue( "Options", "TrilinearFiltering",				m_bTrilinearFiltering );
	ini.SetValue( "Options", "AnisotropicFiltering",			m_bAnisotropicFiltering );
	ini.SetValue( "Options", "AutoRestart",					g_bAutoRestart );
	ini.SetValue( "Options", "SignProfileData",				m_bSignProfileData );
	
	ini.SetValue( "Options", "SoundWriteAhead",				m_iSoundWriteAhead );
	ini.SetValue( "Options", "SoundDevice",					m_iSoundDevice );

	ini.SetValue( "Editor", "ShowBGChangesPlay",				m_bEditorShowBGChangesPlay );
	ini.SetValue( "Editor",	"ShiftIsUsedAsAreaSelector",			m_bEditShiftSelector );

	/* Only write these if they aren't the default.  This ensures that we can change
	 * the default and have it take effect for everyone (except people who
	 * tweaked this value). */
	if(m_sSoundDrivers != DEFAULT_SOUND_DRIVER_LIST)
		ini.SetValue ( "Options", "SoundDrivers",			m_sSoundDrivers );
	if(m_fSoundVolume != DEFAULT_SOUND_VOLUME)
		ini.SetValue( "Options", "SoundVolume",				m_fSoundVolume );
	if(m_sLightsDriver != DEFAULT_LIGHTS_DRIVER)
		ini.SetValue( "Options", "LightsDriver",			m_sLightsDriver );
	if(m_sMovieDrivers != DEFAULT_MOVIE_DRIVER_LIST)
		ini.SetValue ( "Options", "MovieDrivers",			m_sMovieDrivers );

	ini.SetValue( "Options", "AdditionalSongFolders", 			m_sAdditionalSongFolders);
	ini.SetValue( "Options", "AdditionalFolders", 				m_sAdditionalFolders);

	ini.SetValue( "Debug", "LogToDisk",					m_bLogToDisk );
	ini.SetValue( "Debug", "ForceLogFlush",					m_bForceLogFlush );
	ini.SetValue( "Debug", "ShowLogOutput",					m_bShowLogOutput );
	ini.SetValue( "Debug", "Timestamping",					m_bTimestamping );
	ini.SetValue( "Debug", "LogSkips",					m_bLogSkips );
	ini.SetValue( "Debug", "LogCheckpoints",				m_bLogCheckpoints );
	ini.SetValue( "Debug", "ShowLoadingWindow",				m_bShowLoadingWindow );

	FOREACH_CONST( IPreference*, *g_pvpSubscribers, p ) (*p)->WriteTo( ini );

	ini.WriteFile( STEPMANIA_INI_PATH );
}

CString PrefsManager::GetSoundDrivers()
{
	if( m_sSoundDrivers.empty() )
		return (CString)DEFAULT_SOUND_DRIVER_LIST;
	else
		return m_sSoundDrivers;
}

int PrefsManager::GetCoinMode()
{
	if( m_bEventMode && m_iCoinMode == COIN_PAY )
		return COIN_FREE; 
	else 
		return m_iCoinMode; 
}

PrefsManager::Premium	PrefsManager::GetPremium() 
{ 
	if(m_bEventMode) 
		return NO_PREMIUM; 
	else 
		return m_Premium; 
}


/*
 * (c) 2001-2004 Chris Danford, Chris Gomez
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
