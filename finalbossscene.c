#include "finalbossscene.h"

#include <tari/animation.h>
#include <tari/tweening.h>
#include <tari/texthandler.h>
#include <tari/math.h>
#include <tari/timer.h>

#include "shothandler.h"

static struct {
	TextureData mWhiteTexture;

	int mBlackBackgroundID;
	int mHelpTextID;

	int mIsShowing;
	int mHasBeenShown;
} gData;

static void loadSceneHandler(void* tData) {
	(void)tData;

	gData.mWhiteTexture = loadTexture("$/rd/effects/white.pkg");

	gData.mHasBeenShown = 0;
	gData.mIsShowing = 0;
}

static void removeHelpTextBG(void* tCaller) {
	(void)tCaller;
	removeHandledAnimation(gData.mBlackBackgroundID);

	gData.mHasBeenShown = 1;
	gData.mIsShowing = 0;
}

static void removeHelpText(void* tCaller) {
	(void)tCaller;
	removeHandledText(gData.mHelpTextID);

	tweenDouble(getAnimationTransparencyReference(gData.mBlackBackgroundID), 0.5, 0, quadraticTweeningFunction, 60, removeHelpTextBG, NULL);
}

static void startDrawingHelpText(void* tCaller) {
	(void)tCaller;
	gData.mHelpTextID = addHandledTextWithBuildup(makePosition(20, 200, 55), "TRY AS YOU MIGHT, I CAN READ YOUR EVERY MOVE.", 0, COLOR_WHITE, makePosition(30, 30, 1), makePosition(-5, 0, 0), makePosition(620, 480, 1), INF, 300);
	addTimerCB(500, removeHelpText, NULL);
}

static void showFinalBossHelpText() {

	gData.mBlackBackgroundID = playOneFrameAnimationLoop(makePosition(0, 0, 50), &gData.mWhiteTexture);
	setAnimationColorType(gData.mBlackBackgroundID, COLOR_BLACK);
	setAnimationSize(gData.mBlackBackgroundID, makePosition(640, 480, 1), makePosition(0, 0, 0));
	setAnimationTransparency(gData.mBlackBackgroundID, 0);

	tweenDouble(getAnimationTransparencyReference(gData.mBlackBackgroundID), 0, 0.5, quadraticTweeningFunction, 60, startDrawingHelpText, NULL);

	gData.mIsShowing = 1;
}

static void updateShowingHelpText() {
	if (gData.mIsShowing) return;

	int bossShots = getFinalBossShotsDeflected();
	if (bossShots < 100) return;

	showFinalBossHelpText();
}

static void updateSceneHandler(void* tData) {
	if (gData.mHasBeenShown) return;

	updateShowingHelpText();
	
}

ActorBlueprint FinalBossSceneHandler = {
	.mLoad = loadSceneHandler,
	.mUpdate = updateSceneHandler,
};