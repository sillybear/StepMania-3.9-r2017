/* PrefsManager - Holds user-chosen preferences that are saved between sessions. */

#ifndef PREFSMANAGER_H
#define PREFSMANAGER_H

#include "PlayerNumber.h"
#include "Grade.h"	// for NUM_GRADE_TIERS

class IPreference;
class IniFile;

class PrefsManager
{
public:
	PrefsManager();
	~PrefsManager();

	void Init();

	// GameOptions (ARE saved between sessions)
	bool			m_bCatalogXML;
	bool			m_bWindowed;
	int			m_iDisplayWidth;
	int			m_iDisplayHeight;
	int			m_iDisplayColorDepth;
	int			m_iTextureColorDepth;
	int			m_iMovieColorDepth;
	int			m_iMaxTextureResolution;
	int			m_iRefreshRate;
	bool			m_bShowStats;
	bool			m_bShowBanners;
	bool			m_bShowBackgrounds;
	enum BackgroundModes { BGMODE_OFF, BGMODE_ANIMATIONS, BGMODE_MOVIEVIS, BGMODE_RANDOMMOVIES } m_BackgroundMode;
	int			m_iNumBackgrounds;
	float			m_fBGBrightness;
	bool			m_bHiddenSongs;
	bool			m_bVsync;
	bool			m_bInterlaced;
	bool			m_bPAL;
	bool			m_bDelayedTextureDelete;
	bool			m_bTexturePreload;
	bool			m_bDelayedScreenLoad;
	bool			m_bDelayedModelDelete;
	enum BannerCacheMode { BNCACHE_OFF, BNCACHE_LOW_RES, BNCACHE_FULL };
	BannerCacheMode		m_BannerCache;
	bool			m_bPalettedBannerCache;
	enum BGCacheMode { BGCACHE_OFF, BGCACHE_LOW_RES, BGCACHE_FULL };
	BGCacheMode		m_BackgroundCache;
	bool			m_bPalettedBackgroundCache;
	bool			m_bFastLoad;
	bool			m_bFastLoadExistingCacheOnly;

	// Used for loading off player profiles
	bool			m_bPlayerSongs;
	bool			m_bPlayerSongsAllowBanners;
	bool			m_bPlayerSongsAllowBackgrounds;
	bool			m_bPlayerSongsAllowCDTitles;
	bool			m_bPlayerSongsAllowLyrics;
	bool			m_bPlayerSongsAllowBGChanges;
	float			m_fPlayerSongsLoadTimeout;
	float			m_fPlayerSongsLengthLimitSeconds;
	int			m_iPlayerSongsLoadLimit;

	// Used for announcer
	bool			m_bPositiveAnnouncerOnly;
	bool			m_bEvalExtraAnnouncer;
	bool			m_bGameExtraAnnouncer;

	// New feature for arcade machines/flaky pads
	float			m_fDebounceTime;

	// New feature for DDR X difficulty scale
	enum MeterTypeScale { MTSCALE_OFF, MTSCALE_ITG, MTSCALE_DDRX, MTSCALE_ALL };
	MeterTypeScale		m_ConvertRatingsScaleDown;

	bool			m_bNonstopUsesExtremeScoring;
	bool			m_bNoSavePlusFeatures;

	// Use for playing sounds
	bool			m_bPlayAttackSounds;
	bool			m_bPlayMineSound;

	// Used for Attack Mods
	float			m_fRandomAttackLength;
	float			m_fTimeBetweenRandomAttacks;
	float			m_fAttackMinesLength;

	bool			m_bDanceRaveShufflesNotes;

	bool			m_bOnlyDedicatedMenuButtons;
	bool			m_bMenuTimer;
	bool			m_bShowDanger;

	bool			m_bRollTapTimingIsAffectedByJudge;
	float			m_fJudgeWindowScale;
	float			m_fJudgeWindowAdd;		// this is useful for compensating for changes in sampling rate between devices
	float			m_fJudgeWindowSecondsMarvelous;
	float			m_fJudgeWindowSecondsPerfect;
	float			m_fJudgeWindowSecondsGreat;
	float			m_fJudgeWindowSecondsGood;
	float			m_fJudgeWindowSecondsBoo;
	float			m_fJudgeWindowSecondsHidden;
	float			m_fJudgeWindowSecondsHold;
	float			m_fJudgeWindowSecondsRoll;	// time from a hit until a roll drops. - Mark
	float			m_fJudgeWindowSecondsMine;
	float			m_fJudgeWindowSecondsAttack;

	float			m_fLifeDifficultyScale;

	// Life Meter, Normal Play
	float			m_fLifePercentInitialValue;
	float			m_fLifeDeltaPercentChangeMarvelous;
	float			m_fLifeDeltaPercentChangePerfect;
	float			m_fLifeDeltaPercentChangeGreat;
	float			m_fLifeDeltaPercentChangeGood;
	float			m_fLifeDeltaPercentChangeBoo;
	float			m_fLifeDeltaPercentChangeMiss;
	float			m_fLifeDeltaPercentChangeHidden;
	float			m_fLifeDeltaPercentChangeMissHidden;
	float			m_fLifeDeltaPercentChangeHitMine;
	float			m_fLifeDeltaPercentChangeHoldOK;
	float			m_fLifeDeltaPercentChangeHoldNG;
	float			m_fLifeDeltaPercentChangeRollOK;
	float			m_fLifeDeltaPercentChangeRollNG;

	// Life Meter, No Recover
	float			m_fLifePercentInitialValueNR;
	float			m_fLifeDeltaPercentChangeMarvelousNR;
	float			m_fLifeDeltaPercentChangePerfectNR;
	float			m_fLifeDeltaPercentChangeGreatNR;
	float			m_fLifeDeltaPercentChangeGoodNR;
	float			m_fLifeDeltaPercentChangeBooNR;
	float			m_fLifeDeltaPercentChangeMissNR;
	float			m_fLifeDeltaPercentChangeHiddenNR;
	float			m_fLifeDeltaPercentChangeMissHiddenNR;
	float			m_fLifeDeltaPercentChangeHitMineNR;
	float			m_fLifeDeltaPercentChangeHoldOKNR;
	float			m_fLifeDeltaPercentChangeHoldNGNR;
	float			m_fLifeDeltaPercentChangeRollOKNR;
	float			m_fLifeDeltaPercentChangeRollNGNR;

	// Life Meter, Sudden Death
	float			m_fLifePercentInitialValueSD;
	float			m_fLifeDeltaPercentChangeMarvelousSD;
	float			m_fLifeDeltaPercentChangePerfectSD;
	float			m_fLifeDeltaPercentChangeGreatSD;
	float			m_fLifeDeltaPercentChangeGoodSD;
	float			m_fLifeDeltaPercentChangeBooSD;
	float			m_fLifeDeltaPercentChangeMissSD;
	float			m_fLifeDeltaPercentChangeHiddenSD;
	float			m_fLifeDeltaPercentChangeMissHiddenSD;
	float			m_fLifeDeltaPercentChangeHitMineSD;
	float			m_fLifeDeltaPercentChangeHoldOKSD;
	float			m_fLifeDeltaPercentChangeHoldNGSD;
	float			m_fLifeDeltaPercentChangeRollOKSD;
	float			m_fLifeDeltaPercentChangeRollNGSD;

	// tug meter used in rave
	float			m_fTugMeterPercentChangeMarvelous;
	float			m_fTugMeterPercentChangePerfect;
	float			m_fTugMeterPercentChangeGreat;
	float			m_fTugMeterPercentChangeGood;
	float			m_fTugMeterPercentChangeBoo;
	float			m_fTugMeterPercentChangeMiss;
	float			m_fTugMeterPercentChangeHitMine;
	float			m_fTugMeterPercentChangeHidden;
	float			m_fTugMeterPercentChangeMissHidden;
	float			m_fTugMeterPercentChangeHoldOK;
	float			m_fTugMeterPercentChangeHoldNG;
	float			m_fTugMeterPercentChangeRollOK;
	float			m_fTugMeterPercentChangeRollNG;

	// Should we fade the backgrounds on random movies?
	bool			m_bFadeVideoBackgrounds;
	
	// Whoever added these: Please add a comment saying what they do. -Chris
	int			m_iRegenComboAfterFail;
	int			m_iRegenComboAfterMiss;
	int			m_iMaxRegenComboAfterFail;
	int			m_iMaxRegenComboAfterMiss;
	bool			m_bTwoPlayerRecovery;
	bool			m_bMercifulDrain;	// negative life deltas are scaled by the players life percentage
	bool			m_bMinimum1FullSongInCourses;	// FEoS for 1st song, FailImmediate thereafter

	// percent score (the number that is shown on the screen and saved to memory card)
	int			m_iPercentScoreWeightMarvelous;
	int			m_iPercentScoreWeightPerfect;
	int			m_iPercentScoreWeightGreat;
	int			m_iPercentScoreWeightGood;
	int			m_iPercentScoreWeightBoo;
	int			m_iPercentScoreWeightMiss;
	int			m_iPercentScoreWeightHitMine;
	int			m_iPercentScoreWeightHoldOK;
	int			m_iPercentScoreWeightHoldNG;
	int			m_iPercentScoreWeightRollOK;
	int			m_iPercentScoreWeightRollNG;

	int			m_iGradeWeightMarvelous;
	int			m_iGradeWeightPerfect;
	int			m_iGradeWeightGreat;
	int			m_iGradeWeightGood;
	int			m_iGradeWeightBoo;
	int			m_iGradeWeightMiss;
	int			m_iGradeWeightHitMine;
	int			m_iGradeWeightHoldOK;
	int			m_iGradeWeightHoldNG;
	int			m_iGradeWeightRollOK;
	int			m_iGradeWeightRollNG;

	int			m_iNumGradeTiersUsed;
	float			m_fGradePercent[NUM_GRADE_TIERS];	// the minimum percent necessary achieve a grade
	bool			m_bGradeTier02IsAllPerfects;		// DDR special case. If true, m_fGradePercentTier[GRADE_TIER_2] is ignored
	bool			m_bGradeTier02RequiresNoMiss;		// PIU special case
	bool			m_bGradeTier02RequiresFC;
	bool			m_bGradeTier03RequiresNoMiss;		// PIU special case
	bool			m_bGradeTier03RequiresFC;		// DDR Supernova special case

	// ScoreKeeperRave
	float			m_fSuperMeterPercentChangeMarvelous;
	float			m_fSuperMeterPercentChangePerfect;
	float			m_fSuperMeterPercentChangeGreat;
	float			m_fSuperMeterPercentChangeGood;
	float			m_fSuperMeterPercentChangeBoo;
	float			m_fSuperMeterPercentChangeHidden;
	float			m_fSuperMeterPercentChangeMissHidden;
	float			m_fSuperMeterPercentChangeMiss;
	float			m_fSuperMeterPercentChangeHitMine;
	float			m_fSuperMeterPercentChangeHoldOK;
	float			m_fSuperMeterPercentChangeHoldNG;
	float			m_fSuperMeterPercentChangeRollOK;
	float			m_fSuperMeterPercentChangeRollNG;
	bool			m_bMercifulSuperMeter;	// negative super deltas are scaled by the players life percentage

	bool			m_bAutoPlay;
	bool			m_bDelayedEscape;
	bool			m_bInstructions, m_bShowDontDie, m_bShowSelectGroup;
	bool			m_bShowNative;
	bool			m_bArcadeOptionsNavigation;
	enum MusicWheelUsesSections { NEVER, ALWAYS, ABC_ONLY } m_MusicWheelUsesSections;
	int			m_iMusicWheelSwitchSpeed;
	bool			m_bEasterEggs;
	int 			m_iMarvelousTiming;
	bool			m_bEventMode;
	bool			m_bEventIgnoreSelectable;
	bool			m_bEventIgnoreUnlock;
	int			m_iCoinsPerCredit;
	int			m_iNumArcadeStages;

	// Options Screens for courses
	bool			m_bCoursePlayerOptions;
	bool			m_bCourseSongOptions;

	// These options have weird interactions depending on m_bEventMode, so wrap them.
	enum Premium { NO_PREMIUM, DOUBLES_PREMIUM, JOINT_PREMIUM };
	Premium			m_Premium;
	int			m_iCoinMode;
	int GetCoinMode();
	Premium	GetPremium();

	bool			m_bDelayedCreditsReconcile;

	bool			m_bLockExtraStageDiff;
	bool			m_bPickExtraStage;
	bool			m_bAlwaysAllowExtraStage2;
	bool			m_bPickModsForExtraStage;
	bool			m_bDarkExtraStage;
	bool			m_bOniExtraStage1;
	bool			m_bOniExtraStage2;

	bool			m_bComboContinuesBetweenSongs;
	bool			m_bUseLongSongs;
	bool			m_bUseMarathonSongs;
	float			m_fLongVerSongSeconds;
	float			m_fMarathonVerSongSeconds;
	enum Maybe { ASK = -1, NO = 0, YES = 1 };
	Maybe			m_ShowSongOptions;
	bool			m_bSoloSingle;
	bool			m_bDancePointsForOni;	//DDR-Extreme style dance points instead of max2 percent
	bool			m_bPercentageScoring;
	float			m_fMinPercentageForMachineSongHighScore;
	float			m_fMinPercentageForMachineCourseHighScore;
	bool			m_bDisqualification;
	bool			m_bShowLyrics;
	bool			m_bAutogenSteps;
	bool			m_bAutogenGroupCourses;
	bool			m_bBreakComboToGetItem;
	bool			m_bLockCourseDifficulties;
	enum CharacterOption { CO_OFF = 0, CO_RANDOM = 1, CO_SELECT = 2};
	CharacterOption	m_ShowDancingCharacters;

	bool			m_bUseUnlockSystem;
	bool			m_bUnlockUsesMachineProfileStats;
	bool			m_bUnlockUsesPlayerProfileStats;

	bool			m_bFirstRun;
	bool			m_bAutoMapOnJoyChange;
	float			m_fGlobalOffsetSeconds;
	int			m_iProgressiveLifebar;
	int			m_iProgressiveStageLifebar;
	int			m_iProgressiveNonstopLifebar;
	bool			m_bShowBeginnerHelper;
	bool			m_bEndlessBreakEnabled;
	int			m_iEndlessNumStagesUntilBreak;
	int			m_iEndlessBreakLength;
	bool			m_bDisableScreenSaver;
	CString			m_sLanguage;
	CString			m_sMemoryCardProfileSubdir;	// the directory on a memory card to look in for a profile
	int			m_iProductID;			// Saved in HighScore to track what software version a score came from.
	CString			m_sDefaultLocalProfileID[NUM_PLAYERS];
	bool			m_bMemoryCards;
	CString			m_sMemoryCardOsMountPoint[NUM_PLAYERS];	// if set, always use the device that mounts to this point
	int			m_iMemoryCardUsbBus[NUM_PLAYERS];	// look for this bus when assigning cards.  -1 = match any
	int			m_iMemoryCardUsbPort[NUM_PLAYERS];	// look for this port when assigning cards.  -1 = match any
	int			m_iMemoryCardUsbLevel[NUM_PLAYERS];	// look for this level when assigning cards.  -1 = match any
	int			m_iCenterImageTranslateX;
	int			m_iCenterImageTranslateY;
	float			m_fCenterImageScaleX;
	float			m_fCenterImageScaleY;
	int			m_iAttractSoundFrequency;	// 0 = never, 1 = every time
	bool			m_bAllowExtraStage;
	bool			m_bHideDefaultNoteSkin;
	int			m_iMaxHighScoresPerListForMachine;
	int			m_iMaxHighScoresPerListForPlayer;
	bool			m_bCelShadeModels;

	/* experimental: force a specific update rate.  This prevents big 
	 * animation jumps on frame skips. */
	float			m_fConstantUpdateDeltaSeconds;	// 0 to disable

	// Number of seconds it takes for a button on the controller to release
	// after pressed.
	float			m_fPadStickSeconds;

	// Useful for non 4:3 displays and resolutions < 640x480 where texels don't
	// map directly to pixels.
	bool			m_bForceMipMaps;
	bool			m_bTrilinearFiltering;		// has no effect without mipmaps on
	bool			m_bAnisotropicFiltering;	// has no effect without mipmaps on.  Not mutually exclusive with trilinear.

	// If true, then signatures created when writing profile data 
	// and verified when reading profile data.  Leave this false if 
	// you want to use a profile on different machines that don't 
	// have the same key, or else the profile's data will be discarded.
	bool			m_bSignProfileData;
	
	/* Editor prefs: */
	bool			m_bEditorShowBGChangesPlay;
	bool			m_bEditShiftSelector;

	// course ranking
	enum CourseSortOrders { COURSE_SORT_SONGS, COURSE_SORT_METER, COURSE_SORT_METER_SUM, COURSE_SORT_RANK } m_iCourseSortOrder;
	bool			m_bMoveRandomToEnd;
	bool			m_bSubSortByNumSteps;	
	enum GetRankingName { RANKING_OFF, RANKING_ON, RANKING_LIST } m_iGetRankingName;

	// scoring type; SCORING_MAX2 should always be first
	enum ScoringTypes { SCORING_MAX2, SCORING_5TH, SCORING_NOVA, SCORING_NOVA2, SCORING_HYBRID, SCORING_PIU, SCORING_CUSTOM, } m_iScoringType;

	/* 0 = no; 1 = yes; -1 = auto (do whatever is appropriate for the arch). */
	int			m_iBoostAppPriority;

	CString			m_sAdditionalSongFolders;
	CString			m_sAdditionalFolders;

	CString			m_sLastSeenVideoDriver;
	CString			m_sLastSeenInputDevices;
#if defined(WIN32)
	int			m_iLastSeenMemory;
#endif
	CString			m_sVideoRenderers;
	bool			m_bSmoothLines;
private:
	CString			m_sSoundDrivers;
public:
	int			m_iSoundWriteAhead;
	CString			m_iSoundDevice;
	float			m_fSoundVolume;
	int			m_iSoundResampleQuality;
	CString			m_sMovieDrivers;

//CHANGE: Include LightsFalloffSeconds, LightsFalloffUsesBPM and LightsIgnoreHolds - Mark
	CString			m_sLightsDriver;
	CString			m_sLightsStepsDifficulty;
	float			m_fLightsFalloffSeconds;
	bool			m_bLightsFalloffUsesBPM;
	int			m_iLightsIgnoreHolds;
	bool			m_bBlinkGameplayButtonLightsOnNote;

	bool			m_bAllowUnacceleratedRenderer;
	bool			m_bThreadedInput;
	bool			m_bThreadedMovieDecode;
	bool			m_bScreenTestMode;
	CString			m_sMachineName;

	CString			m_sIgnoredMessageWindows;

	CString			m_sCoursesToShowRanking;

	/* Debug: */
	bool			m_bLogToDisk;
	bool			m_bForceLogFlush;
	bool			m_bShowLogOutput;
	bool			m_bTimestamping;
	bool			m_bLogSkips;
	bool			m_bLogCheckpoints;
	bool			m_bShowLoadingWindow;

	/* Game-specific prefs: */
	CString			m_sDefaultModifiers;


	// wrappers
	CString GetSoundDrivers();


	void ReadGlobalPrefsFromDisk();
	void SaveGlobalPrefsToDisk() const;

	void ResetToFactoryDefaults();

protected:
	void ReadPrefsFromFile( CString sIni );

};


//
// For self-registering prefs
//
void Subscribe( IPreference *p );


/* This is global, because it can be accessed by crash handlers and error handlers
 * that are run after PREFSMAN shuts down (and probably don't want to deref tht
 * pointer anyway). */
extern bool			g_bAutoRestart;

extern PrefsManager*	PREFSMAN;	// global and accessable from anywhere in our program

#endif

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
