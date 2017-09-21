#include "level.h"

#include <tari/mugendefreader.h>
#include <tari/animation.h>
#include <tari/collisionhandler.h>
#include <tari/mugenanimationreader.h>

#include "enemyhandler.h"
#include "banter.h"
#include "boss.h"
#include "bg.h"

typedef struct {
	TextureData mTextures[10];
	Animation mAnimation;

	Collider mCollider;

} ShotType;

typedef struct {
	int mActive;
	int mType;

	int mStagePart;
	Duration mTime;

	Position mStart;
	Velocity mVelocity;

	Duration mShotFrequency;
	int mShotType;
} StageEnemy;

typedef struct {
	int mIsActive;
	Duration mTime;
	int mStagePart;
} StageBoss;

typedef struct {
	int mIsActive;
	Duration mTime;
	int mStagePart;
} StageBreak;

static struct {
	int mCurrentLevel;

	MugenAnimations mAnimations;
	MugenSpriteFile mSprites;

	List mStageEnemies; // contains StageEnemy
	List mStageBreaks;
	StageBoss mBoss;

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

static void loadStageEnemy(MugenDefScriptGroup* tGroup) {
	StageEnemy* e = allocMemory(sizeof(StageEnemy));
	e->mActive = 1;
	e->mType = getMugenDefNumberVariableAsGroup(tGroup, "id");
	e->mTime = getMugenDefNumberVariableAsGroup(tGroup, "time");
	e->mStart = getMugenDefVectorVariableAsGroup(tGroup, "position");
	e->mStart.z = 20;
	e->mVelocity = getMugenDefVectorVariableAsGroup(tGroup, "velocity");
	e->mShotFrequency = getMugenDefNumberVariableAsGroup(tGroup, "shotfrequency");
	e->mShotType = getMugenDefNumberVariableAsGroup(tGroup, "shottype");
	e->mStagePart = gData.mStagePart;

	list_push_back_owned(&gData.mStageEnemies, e);
}

static int isStageBoss(char* tName) {
	return !strcmp("Boss", tName);
}

static void loadStageBoss(MugenDefScriptGroup* tGroup) {
	gData.mBoss.mTime = getMugenDefIntegerOrDefaultAsGroup(tGroup, "time", 0);
	gData.mBoss.mStagePart = gData.mStagePart;
}

static int isStageBreak(char* tName) {
	return !strcmp("Break", tName);
}

static void loadStageBreak(MugenDefScriptGroup* tGroup) {
	StageBreak* e = allocMemory(sizeof(StageBreak));
	e->mIsActive = 0;
	e->mStagePart = gData.mStagePart;
	e->mTime = getMugenDefIntegerOrDefaultAsGroup(tGroup, "time", 0);

	list_push_back_owned(&gData.mStageBreaks, e);
	gData.mStagePart++;
}

static void loadStageEnemiesFromScript(MugenDefScript* tScript) {
	gData.mStagePart = 0;

	MugenDefScriptGroup* current = tScript->mFirstGroup;
	while (current != NULL) {
		if (isStageEnemy(current->mName)) {
			loadStageEnemy(current);
		}
		else if (isStageBreak(current->mName)) {
			loadStageBreak(current);
		}
		else if (isStageBoss(current->mName)) {
			loadStageBoss(current);
		}

		current = current->mNext;
	}

}

static void loadBoss(MugenDefScript* tScript) {
	char* defPath = getAllocatedMugenDefStringVariable(tScript, "Header", "boss");
	loadBossFromDefinitionPath(defPath, &gData.mAnimations, &gData.mSprites);
	freeMemory(defPath);

	gData.mBoss.mIsActive = 0;
}

static void loadStage(MugenDefScript* tScript) {
	char* defPath = getAllocatedMugenDefStringVariable(tScript, "Header", "bg");
	setBackground(defPath, &gData.mSprites);
	freeMemory(defPath);
}

static void loadLevelHandler(void* tData) {
	(void)tData;
	gData.mCurrentLevel = 1; // TODO
	gData.mStageEnemies = new_list();

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

static void updateSingleStageEnemy(void* tCaller, void* tData) {
	(void)tCaller;
	StageEnemy* e = tData;
	
	if (!e->mActive) return;
	if (e->mStagePart != gData.mStagePart) return;
	if (!isDurationOver(gData.mTime, e->mTime)) return;

	e->mActive = 0;	
	addEnemy(e->mType, e->mStart, e->mVelocity, e->mShotFrequency, e->mShotType);
}

static void updateEnemies() {
	list_map(&gData.mStageEnemies, updateSingleStageEnemy, NULL);
}

static void updateBoss() {
	if (gData.mBoss.mIsActive) return;
	if (gData.mBoss.mStagePart != gData.mStagePart) return;
	if (!isDurationOver(gData.mTime, gData.mBoss.mTime)) return;

	activateBoss();
	gData.mBoss.mIsActive = 1;
}

static void increaseStagePart() {
	gData.mStagePart++;
	gData.mTime = 0;
}

static void updateSingleBreak(void* tCaller, void* tData) {
	(void)tCaller;
	StageBreak* e = tData;
	if (e->mIsActive) return;
	if (e->mStagePart != gData.mStagePart) return;
	if (!isDurationOver(gData.mTime, e->mTime)) return;
	if (getEnemyAmount()) return;

	e->mIsActive = 1;
	increaseStagePart();

}

static void updateBreaks() {
	list_map(&gData.mStageBreaks, updateSingleBreak, NULL);
}

static void updateLevelHandler(void* tData) {
	(void)tData;
	updateTime();
	updateEnemies();
	updateBoss();
	updateBreaks();
}

ActorBlueprint LevelHandler = {
	.mLoad = loadLevelHandler,
	.mUpdate = updateLevelHandler,
};