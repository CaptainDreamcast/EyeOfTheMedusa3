#include "banter.h"

#include <assert.h>
#include <string.h>

#include <tari/log.h>
#include <tari/system.h>
#include <tari/memoryhandler.h>
#include <tari/mugenspritefilereader.h>
#include <tari/mugenanimationhandler.h>
#include <tari/texthandler.h>
#include <tari/math.h>

typedef struct {
	Vector3DI mSprite;

	char* mSpeaker;
	char* mText;
} BanterStep;

typedef struct {
	Vector mSteps;
} Banter;

typedef enum {
	BANTER_TRIGGER_START,
	BANTER_TRIGGER_BOSS_START,
	BANTER_TRIGGER_BOSS_END,
} ForcedBanterTrigger;

typedef struct {
	ForcedBanterTrigger mTrigger;
	Banter mBanter;
} ForcedBanter;

static struct {
	Vector mRandomBanter;

	Vector mForcedBanter;
	Vector mRandomBossBanter;

	MugenSpriteFile mSprites;

	MugenAnimation* mAnimation;
	int mAnimationID;
	int mSpeakerTextID;
	int mBanterTextID;

	int mIsRandomBanter;
	Banter* mActiveBanter;
	int mActiveBanterStep;
	int mIsBanterActive;

	Duration mBanterStepNow;
	Duration mBanterStepDuration;
} gData;

static int isNewBanter(char* tName) {
	return !strcmp("Banter", tName);
}

static int isBanterStep(char* tName) {
	return !strcmp("BanterStep", tName);
}

static void handleNewBanter() {
	Banter* e = allocMemory(sizeof(Banter));
	e->mSteps = new_vector();
	vector_push_back_owned(&gData.mRandomBanter, e);
}

static BanterStep* loadGeneralBanterStepFromScript(MugenDefScriptGroup* tGroup) {
	BanterStep* e = allocMemory(sizeof(BanterStep));
	e->mSpeaker = getAllocatedMugenDefStringVariableAsGroup(tGroup, "speaker");
	e->mText = getAllocatedMugenDefStringVariableAsGroup(tGroup, "text");
	e->mSprite = getMugenDefVectorIVariableAsGroup(tGroup, "sprite");

	return e;
}

static void handleBanterStep(MugenDefScriptGroup* tGroup) {
	BanterStep* e = loadGeneralBanterStepFromScript(tGroup);

	assert(vector_size(&gData.mRandomBanter));
	Banter* b = vector_get(&gData.mRandomBanter, vector_size(&gData.mRandomBanter) - 1);

	vector_push_back_owned(&b->mSteps, e);
}

static void loadRandomBanter() {
	gData.mRandomBanter = new_vector();

	MugenDefScript script = loadMugenDefScript("assets/banter/random.def");
	MugenDefScriptGroup* current = script.mFirstGroup;
	while (current != NULL) {
		if (isNewBanter(current->mName)) {
			handleNewBanter();
		}
		else if (isBanterStep(current->mName)) {
			handleBanterStep(current);
		}
		else {
			logError("Unknown banter group");
			logErrorString(current->mName);
			abortSystem();
		}

		current = current->mNext;
	}

	unloadMugenDefScript(script);

}

static void loadBanterHandler(void* tData) {
	(void)tData;
	loadRandomBanter();

	gData.mForcedBanter = new_vector();
	gData.mRandomBossBanter = new_vector();

	gData.mBanterStepDuration = 600;
	gData.mIsBanterActive = 0;
}

static void setNewBanterStepActive() {
	Banter* banter = gData.mActiveBanter;

	assert(gData.mActiveBanterStep < vector_size(&banter->mSteps));
	BanterStep* step = vector_get(&banter->mSteps, gData.mActiveBanterStep);
	
	destroyMugenAnimation(gData.mAnimation);
	gData.mAnimation = createOneFrameMugenAnimationForSprite(step->mSprite.x, step->mSprite.y);
	changeMugenAnimation(gData.mAnimationID, gData.mAnimation);

	removeHandledText(gData.mSpeakerTextID);
	removeHandledText(gData.mBanterTextID);
	
	gData.mSpeakerTextID = addHandledText(makePosition(152, 359, 85), step->mSpeaker, 0, COLOR_WHITE, makePosition(15, 15, 0), makePosition(-5, -5, 0), makePosition(INF, INF, 1), INF);
	gData.mBanterTextID = addHandledTextWithBuildup(makePosition(150, 379, 85), step->mText, 0, COLOR_WHITE, makePosition(15, 15, 0), makePosition(-5, 0, 0), makePosition(180, 100, 1), INF, 1);

	gData.mBanterStepNow = 0;
}

static void updateSettingNewBanter() {
	if (gData.mIsBanterActive) return;

	gData.mIsRandomBanter = 1;
	int i = randfromInteger(0, vector_size(&gData.mRandomBanter) - 1);
	gData.mActiveBanter = vector_get(&gData.mRandomBanter, i);
	gData.mActiveBanterStep = 0;
	
	setNewBanterStepActive();

	gData.mIsBanterActive = 1;
}

static void increaseBanterStep() {
	gData.mActiveBanterStep++;
	if (gData.mActiveBanterStep >= vector_size(&gData.mActiveBanter->mSteps)) {
		gData.mIsBanterActive = 0;
		return;
	}

	setNewBanterStepActive();
}

static void updateGettingToNextBanterStep() {
	assert(gData.mIsBanterActive);

	if (handleDurationAndCheckIfOver(&gData.mBanterStepNow, gData.mBanterStepDuration)) {
		increaseBanterStep();
		gData.mBanterStepNow = 0;
	}
}

static void updateBanterHandler(void* tData) {
	(void)tData;

	updateSettingNewBanter();
	updateGettingToNextBanterStep();
}

ActorBlueprint BanterHandler = {
	.mLoad = loadBanterHandler,
	.mUpdate = updateBanterHandler,
};

static int isNewRandomBossBanter(char* tName) {
	return !strcmp("RandomBossBanter", tName);
}

static void handleNewRandomBossBanter() {
	Banter* e = allocMemory(sizeof(Banter));
	e->mSteps = new_vector();
	vector_push_back_owned(&gData.mRandomBossBanter, e);
}

static int isRandomBossBanterStep(char* tName) {
	return !strcmp("RandomBossBanterStep", tName);
}

static void handleRandomBossBanterStep(MugenDefScriptGroup* tGroup) {
	BanterStep* e = loadGeneralBanterStepFromScript(tGroup);

	assert(vector_size(&gData.mRandomBossBanter));
	Banter* b = vector_get(&gData.mRandomBossBanter, vector_size(&gData.mRandomBossBanter) - 1);

	vector_push_back_owned(&b->mSteps, e);
}

static int isNewForcedBanter(char* tName) {
	return !strcmp("ForcedBanter", tName);
}

static void handleNewForcedBanter(MugenDefScriptGroup* tGroup) {
	ForcedBanter* e = allocMemory(sizeof(ForcedBanter));
	
	char* type = getAllocatedMugenDefStringVariableAsGroup(tGroup, "trigger");
	if (!strcmp("start", type)) e->mTrigger = BANTER_TRIGGER_START;
	else if (!strcmp("bossstart", type)) e->mTrigger = BANTER_TRIGGER_BOSS_START;
	else if (!strcmp("bossend", type)) e->mTrigger = BANTER_TRIGGER_BOSS_END;
	else {
		logError("Unrecognized type.");
		logErrorString(type);
		abortSystem();
	}
	freeMemory(type);

	e->mBanter.mSteps = new_vector();
	
	vector_push_back_owned(&gData.mForcedBanter, e);
}

static int isForcedBanterStep(char* tName) {
	return !strcmp("ForcedBanterStep", tName);
}

static void handleForcedBanterStep(MugenDefScriptGroup* tGroup) {
	BanterStep* e = loadGeneralBanterStepFromScript(tGroup);

	assert(vector_size(&gData.mForcedBanter));
	ForcedBanter* b = vector_get(&gData.mForcedBanter, vector_size(&gData.mForcedBanter) - 1);

	vector_push_back_owned(&b->mBanter.mSteps, e);
}

static int isBanterHeader(char* tName) {
	return !strcmp("Header", tName);
}

static void handleBanterHeader(MugenDefScriptGroup* tGroup) {
	char* spritesPath = getAllocatedMugenDefStringVariableAsGroup(tGroup, "sprites");
	gData.mSprites = loadMugenSpriteFileWithoutPalette(spritesPath);
	freeMemory(spritesPath);
}

static void initiateTextAndImage() {
	gData.mAnimation = createOneFrameMugenAnimationForSprite(0, 0);
	gData.mAnimationID = addMugenAnimation(gData.mAnimation, &gData.mSprites, makePosition(38, 354, 85));
	
	gData.mSpeakerTextID = addHandledText(makePosition(152, 359, 85), "Kat:", 0, COLOR_WHITE, makePosition(15,15,0), makePosition(-5, -5, 0), makePosition(INF, INF, 1), INF);
	gData.mBanterTextID = addHandledTextWithBuildup(makePosition(150, 379, 85), "You know it's not just a dog, right?", 0, COLOR_WHITE, makePosition(15, 15, 0), makePosition(-5, 0, 0), makePosition(180, 100, 1), INF, 1);
}

void loadBanterFromScript(MugenDefScript * tScript)
{
	char* specialBanterDef = getAllocatedMugenDefStringVariable(tScript, "Header", "banter");
	MugenDefScript banterscript = loadMugenDefScript(specialBanterDef);

	MugenDefScriptGroup* current = banterscript.mFirstGroup;
	while (current != NULL) {
		if (isBanterHeader(current->mName)) {
			handleBanterHeader(current);
		}
		else if (isNewRandomBossBanter(current->mName)) {
			handleNewRandomBossBanter();
		}
		else if (isRandomBossBanterStep(current->mName)) {
			handleRandomBossBanterStep(current);
		}
		else if (isNewForcedBanter(current->mName)) {
			handleNewForcedBanter(current);
		}
		else if (isForcedBanterStep(current->mName)) {
			handleForcedBanterStep(current);
		}
		else {
			logError("Unknown group.");
			logErrorString(current->mName);
			abortSystem();
		}
	
		current = current->mNext;
	}

	freeMemory(specialBanterDef);
	unloadMugenDefScript(banterscript);

	initiateTextAndImage();
}
