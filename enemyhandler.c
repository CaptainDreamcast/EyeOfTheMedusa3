#include "enemyhandler.h"

#include <assert.h>

#include <tari/collisionhandler.h>
#include <tari/datastructures.h>
#include <tari/mugenanimationreader.h>
#include <tari/mugenspritefilereader.h>
#include <tari/physicshandler.h>
#include <tari/mugenanimationhandler.h>
#include <tari/math.h>
#include <tari/mugenassignmentevaluator.h>

#include "shothandler.h"
#include "collision.h"
#include "itemhandler.h"

typedef struct {
	int mIdleAnimation;
	int mDeathAnimation;

	int mID;
} EnemyType;

typedef enum {
	ENEMY_MOVEMENT_STATE_GOTO_WAIT,
	ENEMY_MOVEMENT_STATE_WAIT,
	ENEMY_MOVEMENT_STATE_GOTO_FINAL
} EnemyMovementState;

typedef struct {
	int mType;
	
	int mListID;
	int mPhysicsID;
	int mAnimationID;
	
	Duration mShotNow;
	Duration mShotFrequency; 
	int mShotType;

	CollisionData mCollisionData;

	Position mStartPosition;
	Position mWaitPosition;
	Position mFinalPosition;
	EnemyMovementType mMovementType;
	EnemyMovementState mMovementState;

	Duration mWaitNow;
	Duration mWaitDuration;
	double mSpeed;

	int mHealth;

	int mIsAlive;

	StageEnemy* mEnemyBase;
} ActiveEnemy;

static struct {
	MugenAnimations* mEnemyAnimations;
	MugenSpriteFile* mEnemySprites;

	IntMap mEnemyTypes;

	List mActiveEnemies;
} gData;

static void loadEnemyHandler(void* tData) {
	(void)tData;
	gData.mEnemyTypes = new_int_map();
	gData.mActiveEnemies = new_list();
}

static int isEnemyTypeGroup(char* tName) {
	return !strcmp("EnemyType", tName);
}

static void loadEnemyTypeFromGroup(MugenDefScriptGroup* tGroup) {
	EnemyType* e = allocMemory(sizeof(EnemyType));

	e->mID = getMugenDefNumberVariableAsGroup(tGroup, "id");
	e->mIdleAnimation = getMugenDefNumberVariableAsGroup(tGroup, "anim");
	e->mDeathAnimation = getMugenDefNumberVariableAsGroup(tGroup, "deathanim");

	int_map_push_owned(&gData.mEnemyTypes, e->mID, e);
}


void loadEnemyTypesFromScript(MugenDefScript* tScript, MugenAnimations* tAnimations, MugenSpriteFile* tSprites)
{
	gData.mEnemyAnimations = tAnimations;
	gData.mEnemySprites = tSprites;

	MugenDefScriptGroup* currentGroup = tScript->mFirstGroup;
	while (currentGroup != NULL) {
		if (isEnemyTypeGroup(currentGroup->mName)) {
			loadEnemyTypeFromGroup(currentGroup);
		}

		currentGroup = currentGroup->mNext;
	}
}

static int getEnemyTypeIdleAnimation(int tID) {
	assert(int_map_contains(&gData.mEnemyTypes, tID));
	EnemyType* e = int_map_get(&gData.mEnemyTypes, tID);
	return e->mIdleAnimation;
}

static void removeActiveEnemy(ActiveEnemy* e) {
	removeMugenAnimation(e->mAnimationID);
	removeFromPhysicsHandler(e->mPhysicsID);
}

static void enemyHitCB(void* tCaller, void* tCollisionData) {
	ActiveEnemy* e = tCaller;
	CollisionData* collisionData = tCollisionData;

	if (collisionData->mCollisionList == getPlayerCollisionList()) return;
	
	e->mHealth--;
	if (e->mHealth) return;
	// TODO

	Position pos = *getHandledPhysicsPositionReference(e->mPhysicsID);
	int powerAmount = getMugenAssignmentAsIntegerValueOrDefaultWhenEmpty(e->mEnemyBase->mSmallPowerAmount, NULL, 0);
	addSmallPowerItems(pos, powerAmount);

	removeActiveEnemy(e);
	list_remove(&gData.mActiveEnemies, e->mListID);
}

typedef struct {
	int i;

} EnemyAssignmentCaller;

void getCurrentEnemyIndex(char* tDst, void* tCaller) {
	EnemyAssignmentCaller* caller = tCaller;

	sprintf(tDst, "%d", caller->i);
}

static void addSingleEnemy(StageEnemy* tEnemy, int i) {
	EnemyAssignmentCaller caller;
	caller.i = i;

	ActiveEnemy* e = allocMemory(sizeof(ActiveEnemy));
	e->mEnemyBase = tEnemy;
	e->mType = tEnemy->mType;
	e->mShotType = getMugenAssignmentAsIntegerValueOrDefaultWhenEmpty(tEnemy->mShotType, &caller, 0);
	e->mShotNow = 0;
	e->mShotFrequency = getMugenAssignmentAsFloatValueOrDefaultWhenEmpty(tEnemy->mShotFrequency, &caller, 60);
	e->mIsAlive = 1;

	e->mStartPosition = getMugenAssignmentAsVector3DValueOrDefaultWhenEmpty(tEnemy->mStartPosition, &caller, makePosition(0, 0, 0));
	e->mPhysicsID = addToPhysicsHandler(e->mStartPosition);
	e->mSpeed = getMugenAssignmentAsFloatValueOrDefaultWhenEmpty(tEnemy->mSpeed, &caller, 1);

	e->mFinalPosition = getMugenAssignmentAsVector3DValueOrDefaultWhenEmpty(tEnemy->mFinalPosition, &caller, makePosition(0, 0, 0));
	e->mWaitPosition = getMugenAssignmentAsVector3DValueOrDefaultWhenEmpty(tEnemy->mWaitPosition, &caller, e->mFinalPosition);
	e->mMovementType = tEnemy->mMovementType;
	e->mMovementState = e->mMovementType == ENEMY_MOVEMENT_TYPE_WAIT ? ENEMY_MOVEMENT_STATE_GOTO_WAIT : ENEMY_MOVEMENT_STATE_GOTO_FINAL;

	e->mWaitDuration = getMugenAssignmentAsFloatValueOrDefaultWhenEmpty(tEnemy->mWaitDuration, &caller, 120);

	e->mAnimationID = addMugenAnimation(getMugenAnimation(gData.mEnemyAnimations, getEnemyTypeIdleAnimation(e->mType)), gData.mEnemySprites, makePosition(0, 0, 0));
	setMugenAnimationBasePosition(e->mAnimationID, getHandledPhysicsPositionReference(e->mPhysicsID));
	e->mCollisionData.mCollisionList = getEnemyCollisionList();
	setMugenAnimationCollisionActive(e->mAnimationID, getEnemyCollisionList(), enemyHitCB, e, &e->mCollisionData);

	e->mHealth = getMugenAssignmentAsIntegerValueOrDefaultWhenEmpty(tEnemy->mHealth, &caller, 10);

	e->mListID = list_push_back_owned(&gData.mActiveEnemies, e);

}

void addEnemy(StageEnemy* tEnemy)
{
	int amount = getMugenAssignmentAsIntegerValueOrDefaultWhenEmpty(tEnemy->mAmount, NULL, 1);
	int i;
	for (i = 0; i < amount; i++) {
		addSingleEnemy(tEnemy, i);
	}
}

int getEnemyAmount()
{
	return list_size(&gData.mActiveEnemies);
}

typedef struct {
	Position mPosition;
	Position mClosestPosition;
	int mHasFoundPosition;
} GetClosestEnemyCaller;

static void getClosestEnemyPositionCheckSingleEnemy(void* tCaller, void* tData) {
	GetClosestEnemyCaller* caller = tCaller;
	ActiveEnemy* e = tData;
	Position p = *getHandledPhysicsPositionReference(e->mPhysicsID);

	double cd = getDistance2D(caller->mPosition, caller->mClosestPosition);
	double nd = getDistance2D(caller->mPosition, p);
	if (caller->mHasFoundPosition && cd  < nd) return;

	caller->mHasFoundPosition = 1;
	caller->mClosestPosition = p;
}


Position getClosestEnemyPosition(Position tPosition)
{
	GetClosestEnemyCaller caller;
	caller.mPosition = tPosition;
	caller.mClosestPosition = tPosition;
	caller.mHasFoundPosition = 0;
	list_map(&gData.mActiveEnemies, getClosestEnemyPositionCheckSingleEnemy, &caller);

	return caller.mClosestPosition;
}

Position getRandomEnemyPosition()
{
	if (!list_size(&gData.mActiveEnemies)) return makePosition(INF, INF, 0);

	int index = randfromInteger(0, list_size(&gData.mActiveEnemies) - 1);
	ActiveEnemy* e = list_get_by_ordered_index(&gData.mActiveEnemies, index);
	Position p = *getHandledPhysicsPositionReference(e->mPhysicsID);
	return p;
}

static void updateEnemyShot(ActiveEnemy* e) {
	if (handleDurationAndCheckIfOver(&e->mShotNow, e->mShotFrequency)) {
		e->mShotNow = 0;
		addShot(e->mShotType, getEnemyShotCollisionList(), *getHandledPhysicsPositionReference(e->mPhysicsID));
	}
}

static void updateEnemyWait(ActiveEnemy* e) {
	if (e->mMovementState != ENEMY_MOVEMENT_STATE_WAIT) return;

	if (handleDurationAndCheckIfOver(&e->mWaitNow, e->mWaitDuration)) {
		e->mMovementState = ENEMY_MOVEMENT_STATE_GOTO_FINAL;
	}
}

static void startWait(ActiveEnemy* e) {
	stopHandledPhysics(e->mPhysicsID);
	e->mMovementState = ENEMY_MOVEMENT_STATE_WAIT;
	e->mWaitNow = 0;
	e->mStartPosition = e->mWaitPosition;
}

static void updateEnemyMovement(ActiveEnemy* e) {
	if (e->mMovementState == ENEMY_MOVEMENT_STATE_WAIT) return;

	Position target;
	if (e->mMovementState == ENEMY_MOVEMENT_STATE_GOTO_WAIT) {
		target = e->mWaitPosition;
	}
	else {
		target = e->mFinalPosition;
	}

	Position start = e->mStartPosition;
	Position* pos = getHandledPhysicsPositionReference(e->mPhysicsID);
	
	double totalLength = vecLength(vecSub(target, start));
	double posLength = vecLength(vecSub(*pos, start));
	double t = posLength / totalLength;

	double stepSize = 1 / (totalLength / e->mSpeed);
	
	t = min(1, t+stepSize);

	*pos = interpolatePositionLinear(start, target, t);
	if (t >= 1)	{
		if (e->mMovementState == ENEMY_MOVEMENT_STATE_GOTO_WAIT) {
			startWait(e);
			return;
		}
		else {
			pos->x = -1000;
		}
	}

}

static int updateSingleActiveEnemy(void* tCaller, void* tData) {
	(void)tCaller;
	ActiveEnemy* e = tData;
	
	updateEnemyWait(e);
	updateEnemyMovement(e);

	Position p = *getHandledPhysicsPositionReference(e->mPhysicsID);
	if (p.x < -100) {
		removeActiveEnemy(e);
		return 1;
	}

	updateEnemyShot(e);

	return 0;
}

static void updateEnemyHandler(void* tData) {
	(void)tData;
	
	list_remove_predicate(&gData.mActiveEnemies, updateSingleActiveEnemy, NULL);
}

ActorBlueprint EnemyHandler = {
	.mLoad = loadEnemyHandler,
	.mUpdate = updateEnemyHandler,
};