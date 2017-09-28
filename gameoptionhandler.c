#include "gameoptionhandler.h"

#include <tari/animation.h>
#include <tari/wrapper.h>
#include <tari/input.h>
#include <tari/optionhandler.h>
#include <tari/texthandler.h>
#include <tari/math.h>
#include <tari/screeneffect.h>
#include <tari/mugenanimationhandler.h>

#include "gamescreen.h"
#include "titlescreen.h"
#include "player.h"
#include "level.h"

static struct {
	TextureData mWhiteTexture;
	int mBlackBGAnimationID;

	int mIsActive;

	int mPauseTextID;
	int mResumeOptionID;
	int mRetryOptionID;
	int mExitOptionID;
} gData;

static void loadGameOptionHandler(void* tData) {
	(void)tData;
	
	instantiateActor(getOptionHandlerBlueprint());
	setOptionButtonA();
	setOptionButtonStart();
	setOptionTextSize(20);
	setOptionTextBreakSize(-5);

	gData.mWhiteTexture = loadTexture("$/rd/effects/white.pkg");

	gData.mIsActive = 0;
}

static void setOptionsInactive() {
	resumeWrapper();
	unpauseMugenAnimationHandler();

	removeHandledAnimation(gData.mBlackBGAnimationID);
	removeHandledText(gData.mPauseTextID);
	removeOption(gData.mResumeOptionID);
	removeOption(gData.mRetryOptionID);
	removeOption(gData.mExitOptionID);

	gData.mIsActive = 0;
}

static void pickResumeOption(void* tData) {
	(void)tData;
	setOptionsInactive();
}

static void goToStartOfGame(void* tCaller) {
	(void)tCaller;
	resetPlayerState();
	setLevelToStart();
	setNewScreen(&GameScreen);
}

static void goToTitleScreen(void* tCaller) {
	(void)tCaller;
	setNewScreen(&TitleScreen);
}

static void pickRetryOption(void* tData) {
	(void)tData;
	resumeWrapper();
	addFadeOut(30, goToStartOfGame, NULL);
}

static void pickExitOption(void* tData) {
	(void)tData;
	resumeWrapper();
	addFadeOut(30, goToTitleScreen, NULL);
}

static void setOptionsActive() {
	pauseWrapper();
	pauseMugenAnimationHandler();
	
	gData.mBlackBGAnimationID = playOneFrameAnimationLoop(makePosition(0, 0, 90), &gData.mWhiteTexture);
	setAnimationSize(gData.mBlackBGAnimationID, makePosition(640, 480, 1), makePosition(0, 0, 0));
	setAnimationTransparency(gData.mBlackBGAnimationID, 0.7);
	setAnimationColor(gData.mBlackBGAnimationID, 0, 0, 0);

	gData.mPauseTextID = addHandledText(makePosition(200, 100, 91), "PAUSE", 0, COLOR_WHITE, makePosition(40, 40, 1), makePosition(-5, -5, 0), makePosition(INF, INF, INF), INF);
	gData.mResumeOptionID = addOption(makePosition(120, 160, 91), "RESUME SAVING THE UNIVERSE", pickResumeOption, NULL);
	gData.mRetryOptionID = addOption(makePosition(120, 190, 91), "RETRY FROM FIRST STAGE", pickRetryOption, NULL);
	gData.mExitOptionID = addOption(makePosition(120, 220, 91), "EXIT TO TITLE SCREEN", pickExitOption, NULL);

	gData.mIsActive = 1;
}

static void updateGameOptionHandler(void* tData) {
	(void)tData;

	if (isWrapperPaused() || gData.mIsActive) return;

	if (hasPressedStartFlank()) {
		setOptionsActive();
	}
}

ActorBlueprint GameOptionHandler = {
	.mLoad = loadGameOptionHandler,
	.mUpdate = updateGameOptionHandler,
};