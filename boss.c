#include "boss.h"

#include <assert.h>

#include <tari/mugenanimationhandler.h>
#include <tari/mugendefreader.h>
#include <tari/mugenscriptparser.h>
#include <tari/physicshandler.h>
#include <tari/log.h>
#include <tari/system.h>
#include <tari/math.h>
#include <tari/mugenassignmentevaluator.h>

#include "collision.h"
#include "shothandler.h"

typedef enum {
	BOSS_ACTION_TYPE_GOTO,
	BOSS_ACTION_TYPE_SHOT,
} BossActionType;


typedef struct {
	MugenAssignment* mTarget;
	MugenAssignment* mSpeed;

} GotoAction;

typedef struct {
	int mShotID;
} ShotAction;

typedef struct {
	int mIsTimeBased;
	MugenAssignment* mTime;
	int mIsRepeating;
	MugenAssignment* mRepeatTime;

	int mIsHealthBased;
	MugenAssignment* mHealth;

	BossActionType mType;
	void* mData;
} BossAction;

typedef struct {
	int mLifeStart;

	Vector mActions;
} BossPattern;

static struct {
	int mIsActive;
	int mIsLoaded;
	int mIdleAnimationNumber;
	MugenAnimation* mIdleAnimation;
	MugenSpriteFile* mSprites;
	
	char* mName;
	int mLifeMax;
	int mLife;
	Position mStartPosition;
	
	int mAnimationID;
	int mPhysicsID;
	CollisionData mCollisionData;

	Vector mPatterns;

	int mCurrentPattern;
	Duration mTime;
	Vector3D mTarget;

} gData;

static void loadBossHandler(void* tData) {
	(void)tData;
	gData.mIsActive = 0;
	gData.mIsLoaded = 0;
}

static int isHeader(MugenDefScriptGroup* tGroup) {
	return !strcmp("Header", tGroup->mName);
}

static void loadHeader(MugenDefScriptGroup* tGroup) {
	gData.mName = getAllocatedMugenDefStringVariableAsGroup(tGroup, "name");
	gData.mLifeMax = getMugenDefNumberVariableAsGroup(tGroup, "life");
	gData.mStartPosition = getMugenDefVectorOrDefaultAsGroup(tGroup, "startpos", makePosition(0, 0, 0));
	gData.mIdleAnimationNumber = getMugenDefNumberVariableAsGroup(tGroup, "anim");
}

static int isNewPattern(MugenDefScriptGroup* tGroup) {
	return !strcmp("Pattern", tGroup->mName);
}

static void loadNewPattern(MugenDefScriptGroup* tGroup) {
	BossPattern* e = allocMemory(sizeof(BossPattern));
	e->mLifeStart = getMugenDefIntegerOrDefaultAsGroup(tGroup, "lifestart", gData.mLifeMax);
	e->mActions = new_vector();

	vector_push_back_owned(&gData.mPatterns, e);
}

static int isAction(MugenDefScriptGroup* tGroup) {
	return !strcmp("Action", tGroup->mName);
}

static void loadGotoAction(BossAction* tAction, MugenDefScriptGroup* tGroup) {
	GotoAction* e = allocMemory(sizeof(GotoAction));
	fetchMugenAssignmentFromGroupAndReturnWhetherItExistsDefaultString("value", tGroup, &e->mTarget, "");
	fetchMugenAssignmentFromGroupAndReturnWhetherItExistsDefaultString("speed", tGroup, &e->mSpeed, "");
	tAction->mData = e;
}


static void loadShotAction(BossAction* tAction, MugenDefScriptGroup* tGroup) {
	ShotAction* e = allocMemory(sizeof(ShotAction));
	e->mShotID = getMugenDefIntegerOrDefaultAsGroup(tGroup, "shottype", 0);
	tAction->mData = e;
}

static void loadActionType(BossAction* e, MugenDefScriptGroup* tGroup) {
	char* typeString = getAllocatedMugenDefStringVariableAsGroup(tGroup, "type");
	if (!strcmp("goto", typeString)) {
		e->mType = BOSS_ACTION_TYPE_GOTO;
		loadGotoAction(e, tGroup);
	}
	else if (!strcmp("shot", typeString)) {
		e->mType = BOSS_ACTION_TYPE_SHOT;
		loadShotAction(e, tGroup);
	}
	else {
		logError("Unrecognized action type");
		logErrorString(typeString);
		abortSystem();
	}

	freeMemory(typeString);
}

static void loadAction(MugenDefScriptGroup* tGroup) {
	BossAction* e = allocMemory(sizeof(BossAction));
	e->mIsTimeBased = fetchMugenAssignmentFromGroupAndReturnWhetherItExists("time", tGroup, &e->mTime);
	e->mIsHealthBased = fetchMugenAssignmentFromGroupAndReturnWhetherItExists("health", tGroup, &e->mHealth);
	assert(e->mIsTimeBased ^ e->mIsHealthBased);
	e->mIsRepeating= fetchMugenAssignmentFromGroupAndReturnWhetherItExists("timerepeated", tGroup, &e->mRepeatTime);

	loadActionType(e, tGroup);

	assert(vector_size(&gData.mPatterns));
	BossPattern* pattern = vector_get_back(&gData.mPatterns);

	vector_push_back_owned(&pattern->mActions, e);
}

void loadBossFromDefinitionPath(char * tDefinitionPath, MugenAnimations* tAnimations, MugenSpriteFile* tSprites)
{
	gData.mPatterns = new_vector();

	MugenDefScript script = loadMugenDefScript(tDefinitionPath);
	resetMugenScriptParser();
	addMugenScriptParseFunction(isHeader, loadHeader);
	addMugenScriptParseFunction(isNewPattern, loadNewPattern);
	addMugenScriptParseFunction(isAction, loadAction);
	parseMugenScript(&script);

	unloadMugenDefScript(script);

	gData.mIdleAnimation = getMugenAnimation(tAnimations, gData.mIdleAnimationNumber);
	gData.mSprites = tSprites;
	gData.mCollisionData.mCollisionList = getEnemyCollisionList();

	gData.mIsLoaded = 1;
}

static void bossHitCB(void* tCaller, void* tCollisionData) {
	(void)tCaller;
	(void)tCollisionData;

	printf("Boss ded\n");
}

void activateBoss() {
	gData.mPhysicsID = addToPhysicsHandler(gData.mStartPosition);

	gData.mAnimationID = addMugenAnimation(gData.mIdleAnimation, gData.mSprites, makePosition(0, 0, 20));
	setMugenAnimationBasePosition(gData.mAnimationID, getHandledPhysicsPositionReference(gData.mPhysicsID));
	setMugenAnimationCollisionActive(gData.mAnimationID, getEnemyCollisionList(), bossHitCB, NULL, &gData.mCollisionData);

	gData.mTime = 0;
	gData.mCurrentPattern = 0;
	gData.mLife = gData.mLifeMax;
	gData.mTarget = *getHandledPhysicsPositionReference(gData.mPhysicsID);
	gData.mIsActive = 1;
}

void fetchBossTimeVariable(char * tDst, void * tCaller)
{
	(void)tCaller;
	sprintf(tDst, "%f", gData.mTime);
}

int isBossActive()
{
	return gData.mIsActive;
}

Position getBossPosition()
{
	assert(gData.mIsActive);
	Position p = *getHandledPhysicsPositionReference(gData.mPhysicsID);
	return p;
}

static void setBossTarget(Vector3D tTarget) {
	gData.mTarget = tTarget;
}

static void performGoto(BossAction* tAction) {
	GotoAction* e = tAction->mData;
	double speed = getMugenAssignmentAsFloatValueOrDefaultWhenEmpty(e->mSpeed, NULL, 2);
	Vector3D target = getMugenAssignmentAsVector3DValueOrDefaultWhenEmpty(e->mTarget, NULL, makePosition(0, 0, 0));

	setHandledPhysicsMaxVelocity(gData.mPhysicsID, speed);
	setBossTarget(target);
}

static void performShot(BossAction* tAction) {
	ShotAction* e = tAction->mData;
	Position p = *getHandledPhysicsPositionReference(gData.mPhysicsID);
	addShot(e->mShotID, getEnemyShotCollisionList(), p);
}

static void performAction(BossAction* e) {
	if (e->mType == BOSS_ACTION_TYPE_GOTO) {
		performGoto(e);
	}
	else if (e->mType == BOSS_ACTION_TYPE_SHOT) {
		performShot(e);
	}
	else {
		logError("Unrecognized boss action type");
		logErrorInteger(e->mType);
		abortSystem();
	}

}

static void updateSingleAction(void* tCaller, void* tData) {
	(void)tCaller;
	BossAction* e = tData;
	
	int isTimeTrigger = e->mIsTimeBased && isDurationOver(gData.mTime, evaluateMugenAssignmentAndReturnAsFloat(e->mTime, NULL));
	if (isTimeTrigger) {
		if (e->mIsRepeating) {
			int repeatTime = getMugenAssignmentAsIntegerValueOrDefaultWhenEmpty(e->mRepeatTime, NULL, INF);
			e->mTime = makeFloatMugenAssignment(gData.mTime + repeatTime);
		}
		else {
			e->mTime = makeFloatMugenAssignment(INF);
		}
	}

	int isHealthTrigger = e->mIsHealthBased && gData.mLife < evaluateMugenAssignmentAndReturnAsInteger(e->mHealth, NULL);
	if (isHealthTrigger) {
		e->mHealth = makeNumberMugenAssignment(-1);
	}

	if (isTimeTrigger || isHealthTrigger) {
		performAction(e);
	}
}

static void updateActions() {
	BossPattern* pattern = vector_get(&gData.mPatterns, gData.mCurrentPattern);
	vector_map(&pattern->mActions, updateSingleAction, NULL);
}

static void updateMovement() {
	Position p = *getHandledPhysicsPositionReference(gData.mPhysicsID);
	Vector3D delta = vecSub(gData.mTarget, p);
	double len = vecLength(delta);
	if (vecLength(delta) < 2) {
		stopHandledPhysics(gData.mPhysicsID);
		return;
	}
	Vector3D accel = (len < 1) ? delta : vecNormalize(delta);
	addAccelerationToHandledPhysics(gData.mPhysicsID, accel);

}

static void updateTime() {
	gData.mTime++;
}

static void updateBoss(void* tData) {
	(void)tData;
	if (!gData.mIsActive) return;
	updateActions();
	updateMovement();
	updateTime();
}

ActorBlueprint BossHandler = {
	.mLoad = loadBossHandler,
	.mUpdate = updateBoss,
};