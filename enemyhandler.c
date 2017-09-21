#include "enemyhandler.h"

#include <assert.h>

#include <tari/collisionhandler.h>
#include <tari/datastructures.h>
#include <tari/mugenanimationreader.h>
#include <tari/mugenspritefilereader.h>
#include <tari/physicshandler.h>
#include <tari/mugenanimationhandler.h>
#include <tari/math.h>

#include "shothandler.h"
#include "collision.h"

typedef struct {
	int mIdleAnimation;
	int mDeathAnimation;

	int mID;
} EnemyType;

typedef struct {
	int mType;
	
	int mListID;
	int mPhysicsID;
	int mAnimationID;
	
	Duration mShotNow;
	Duration mShotFrequency; 
	int mShotType;

	CollisionData mCollisionData;

	int mIsAlive;
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

	// TODO
	removeActiveEnemy(e);
	list_remove(&gData.mActiveEnemies, e->mListID);
}

void addEnemy(int tType, Position mPosition, Velocity mVelocity, Duration mShotFrequency, int mShotType)
{
	ActiveEnemy* e = allocMemory(sizeof(ActiveEnemy));
	e->mType = tType;
	e->mShotType = mShotType;
	e->mShotNow = 0;
	e->mShotFrequency = mShotFrequency;
	e->mIsAlive = 1;

	e->mPhysicsID = addToPhysicsHandler(mPosition);
	addAccelerationToHandledPhysics(e->mPhysicsID, mVelocity);

	e->mAnimationID = addMugenAnimation(getMugenAnimation(gData.mEnemyAnimations, getEnemyTypeIdleAnimation(e->mType)), gData.mEnemySprites, makePosition(0,0,0));
	setMugenAnimationBasePosition(e->mAnimationID, getHandledPhysicsPositionReference(e->mPhysicsID));
	e->mCollisionData.mCollisionList = getEnemyCollisionList();
	setMugenAnimationCollisionActive(e->mAnimationID, getEnemyCollisionList(), enemyHitCB, e, &e->mCollisionData);

	e->mListID = list_push_back_owned(&gData.mActiveEnemies, e);
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

static int updateSingleActiveEnemy(void* tCaller, void* tData) {
	(void)tCaller;
	ActiveEnemy* e = tData;

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