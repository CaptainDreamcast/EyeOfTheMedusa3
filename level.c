#include "level.h"

#include <tari/mugendefreader.h>
#include <tari/animation.h>
#include <tari/collisionhandler.h>
#include <tari/mugenanimationreader.h>
#include <tari/mugenassignmentevaluator.h>
#include <tari/log.h>
#include <tari/system.h>

#include "enemyhandler.h"
#include "banter.h"
#include "boss.h"
#include "bg.h"
#include "player.h"

typedef struct {
	TextureData mTextures[10];
	Animation mAnimation;

	Collider mCollider;

} ShotType;

typedef enum {
	LEVEL_ACTION_TYPE_ENEMY,
	LEVEL_ACTION_TYPE_BOSS,
	LEVEL_ACTION_TYPE_BREAK,
	LEVEL_ACTION_TYPE_RESET_LOCAL_COUNTERS,

} LevelActionType;

typedef struct {
	int mHasBeenActivated;
	int mStagePart;
	Duration mTime;

	LevelActionType mType;
	void* mData;
} LevelAction;

static struct {
	int mCurrentLevel;

	MugenAnimations mAnimations;
	MugenSpriteFile mSprites;

	List mStageActions;

	Duration mTime;

	int mStagePart;
} gData;

static void loadSpritesAndAnimations(MugenDefScript* tScript) {
	char* animationPath = getAllocatedMugenDefStringVariable(tScript, "Header", "animations");
	gData.mAnimations = loadMugenAnimationFile(animationPath);
	freeMemory(animationPath);

	char* spritePath = getAllocatedMugenDefStringVariable(tScript, "Header", "sprites");
	gData.mSprites = loadMugenSpriteFileWithoutPalette(spritePath);
	freeMemory(spritePath);
}

static int isStageEnemy(char* tName) {
	return !strcmp("Enemy", tName);
}

static void loadStageEnemyMovementType(StageEnemy* e, MugenDefScriptGroup* tGroup) {
	char* type = getAllocatedMugenDefStringVariableAsGroup(tGroup, "movementtype");

	if (!strcmp("wait", type)) {
		e->mMovementType = ENEMY_MOVEMENT_TYPE_WAIT;
	} else if (!strcmp("rush", type)) {
		e->mMovementType = ENEMY_MOVEMENT_TYPE_RUSH;
	}
	else {
		logError("Unrecognized movement type");
		logErrorString(type);
		abortSystem();
	}

	freeMemory(type);
}

static void loadStageEnemy(MugenDefScriptGroup* tGroup, LevelAction* tLevelAction) {
	StageEnemy* e = allocMemory(sizeof(StageEnemy));
	e->mType = getMugenDefNumberVariableAsGroup(tGroup, "id");
	
	fetchMugenAssignmentFromGroupAndReturnWhetherItExistsDefaultString("position", tGroup, &e->mStartPosition, "");
	fetchMugenAssignmentFromGroupAndReturnWhetherItExistsDefaultString("waitposition", tGroup, &e->mWaitPosition, "");
	fetchMugenAssignmentFromGroupAndReturnWhetherItExistsDefaultString("waitduration", tGroup, &e->mWaitDuration, "");
	fetchMugenAssignmentFromGroupAndReturnWhetherItExistsDefaultString("finalposition", tGroup, &e->mFinalPosition, "");
	fetchMugenAssignmentFromGroupAndReturnWhetherItExistsDefaultString("speed", tGroup, &e->mSpeed, "");
	fetchMugenAssignmentFromGroupAndReturnWhetherItExistsDefaultString("shotfrequency", tGroup, &e->mShotFrequency, "");
	fetchMugenAssignmentFromGroupAndReturnWhetherItExistsDefaultString("shottype", tGroup, &e->mShotType, "");
	fetchMugenAssignmentFromGroupAndReturnWhetherItExistsDefaultString("health", tGroup, &e->mHealth, "");
	fetchMugenAssignmentFromGroupAndReturnWhetherItExistsDefaultString("smallpower", tGroup, &e->mSmallPowerAmount, "");
	fetchMugenAssignmentFromGroupAndReturnWhetherItExistsDefaultString("lifedrop", tGroup, &e->mLifeDropAmount, "");
	fetchMugenAssignmentFromGroupAndReturnWhetherItExistsDefaultString("bombdrop", tGroup, &e->mBombDropAmount, "");
	fetchMugenAssignmentFromGroupAndReturnWhetherItExistsDefaultString("amount", tGroup, &e->mAmount, "");

	loadStageEnemyMovementType(e, tGroup);
	
	tLevelAction->mData = e;
}

static int isStageBoss(char* tName) {
	return !strcmp("Boss", tName);
}

static void loadStageBoss(MugenDefScriptGroup* tGroup, LevelAction* tLevelAction) {
	tLevelAction->mData = NULL;
}

static int isStageBreak(char* tName) {
	return !strcmp("Break", tName);
}

static void loadStageBreak(MugenDefScriptGroup* tGroup, LevelAction* tLevelAction) {
	tLevelAction->mData = NULL;
	gData.mStagePart++;
}

static int isLocalCountReset(char* tName) {
	return !strcmp("ResetLocalCounts", tName);
}

static void loadLocalCountReset(MugenDefScriptGroup* tGroup, LevelAction* tLevelAction) {
	tLevelAction->mData = NULL;
}

static LevelAction* loadLevelActionFromGroup(MugenDefScriptGroup* tGroup) {
	LevelAction* e = allocMemory(sizeof(LevelAction));
	e->mTime = getMugenDefNumberVariableAsGroup(tGroup, "time");
	e->mHasBeenActivated = 0;
	e->mStagePart = gData.mStagePart;
	return e;
}

static void loadSingleLevelAction(MugenDefScriptGroup* tGroup, LevelActionType tType, void(*tLoadFunc)(MugenDefScriptGroup*, LevelAction*)) {
	LevelAction* e = loadLevelActionFromGroup(tGroup);
	e->mType = tType;
	tLoadFunc(tGroup, e);
	list_push_back_owned(&gData.mStageActions, e);
}

static void loadStageEnemiesFromScript(MugenDefScript* tScript) {
	gData.mStagePart = 0;

	MugenDefScriptGroup* current = tScript->mFirstGroup;
	while (current != NULL) {
		
		if (isStageEnemy(current->mName)) {
			loadSingleLevelAction(current, LEVEL_ACTION_TYPE_ENEMY, loadStageEnemy);
		}
		else if (isStageBreak(current->mName)) {
			loadSingleLevelAction(current, LEVEL_ACTION_TYPE_BREAK, loadStageBreak);
		}
		else if (isStageBoss(current->mName)) {
			loadSingleLevelAction(current, LEVEL_ACTION_TYPE_BOSS, loadStageBoss);
		}
		else if (isLocalCountReset(current->mName)) {
			loadSingleLevelAction(current, LEVEL_ACTION_TYPE_RESET_LOCAL_COUNTERS, loadLocalCountReset);
		}

		
		current = current->mNext;
	}

}

static void loadBoss(MugenDefScript* tScript) {
	char* defPath = getAllocatedMugenDefStringVariable(tScript, "Header", "boss");
	loadBossFromDefinitionPath(defPath, &gData.mAnimations, &gData.mSprites);
	freeMemory(defPath);
}

static void loadStage(MugenDefScript* tScript) {
	char* defPath = getAllocatedMugenDefStringVariable(tScript, "Header", "bg");
	setBackground(defPath, &gData.mSprites);
	freeMemory(defPath);
}

static void loadLevelHandler(void* tData) {
	(void)tData;
	gData.mCurrentLevel = 2; // TODO
	gData.mStageActions = new_list();

	char path[1024];
	sprintf(path, "assets/stage/%d.def", gData.mCurrentLevel);
	MugenDefScript script = loadMugenDefScript(path);

	loadSpritesAndAnimations(&script);
	loadEnemyTypesFromScript(&script, &gData.mAnimations, &gData.mSprites);
	loadBanterFromScript(&script);
	loadStageEnemiesFromScript(&script);
	loadBoss(&script);
	loadStage(&script);
	unloadMugenDefScript(script);

	gData.mStagePart = 0;
	gData.mTime = 0;
}

static void updateTime() {
	gData.mTime++;
}

static void updateSingleStageEnemy(LevelAction* tLevelAction) {
	StageEnemy* e = tLevelAction->mData;
	tLevelAction->mHasBeenActivated = 1;	
	addEnemy(e);
}


static void updateBoss(LevelAction* tLevelAction) {

	tLevelAction->mHasBeenActivated = 1;
	activateBoss();

}

static void increaseStagePart() {
	gData.mStagePart++;
	gData.mTime = 0;
}

static void updateSingleBreak(LevelAction* tLevelAction) {
	if (getEnemyAmount()) return;

	tLevelAction->mHasBeenActivated = 1;
	increaseStagePart();
}

static void updateLocalCounterReset(LevelAction* tLevelAction) {
	tLevelAction->mHasBeenActivated = 1;
	resetLocalPlayerCounts();
}

static void updateSingleAction(void* tCaller, void* tData) {
	(void)tCaller;
	LevelAction* e = tData;

	if (e->mHasBeenActivated) return;
	if (e->mStagePart != gData.mStagePart) return;
	if (!isDurationOver(gData.mTime, e->mTime)) return;
	
	if (e->mType == LEVEL_ACTION_TYPE_ENEMY) {
		updateSingleStageEnemy(e);
	} else if (e->mType == LEVEL_ACTION_TYPE_BOSS) {
		updateBoss(e);
	}
	else if (e->mType == LEVEL_ACTION_TYPE_BREAK) {
		updateSingleBreak(e);
	}
	else if (e->mType == LEVEL_ACTION_TYPE_RESET_LOCAL_COUNTERS) {
		updateLocalCounterReset(e);
	}
	else {
		logError("Unrecognized action type.");
		logErrorInteger(e->mType);
		abortSystem();
	}
}

static void updateActions() {
	list_map(&gData.mStageActions, updateSingleAction, NULL);
}

static void updateLevelHandler(void* tData) {
	(void)tData;
	updateTime();
	updateActions();
}

ActorBlueprint LevelHandler = {
	.mLoad = loadLevelHandler,
	.mUpdate = updateLevelHandler,
};