#include "shothandler.h"

#include <assert.h>

#include <tari/mugenspritefilereader.h>
#include <tari/mugenanimationreader.h>
#include <tari/collisionhandler.h>
#include <tari/physicshandler.h>
#include <tari/mugenanimationhandler.h>
#include <tari/log.h>
#include <tari/system.h>
#include <tari/mugenscriptparser.h>
#include <tari/mugenassignmentevaluator.h>
#include <tari/math.h>

#include "collision.h"
#include "enemyhandler.h"
#include "boss.h"
#include "player.h"

typedef enum {
	SHOT_TYPE_NORMAL,
	SHOT_TYPE_HOMING,
	SHOT_TYPE_TARGET_RANDOM,
} ShotHomingType;

typedef struct {
	MugenAssignment* mAmount;
	
	ShotHomingType mHomingType;
	
	MugenAssignment* mOffset;

	int mHasAbsolutePosition;
	MugenAssignment* mAbsolutePosition;

	int mHasVelocity;
	MugenAssignment* mVelocity;
	int mHasAngle;
	MugenAssignment* mAngle;
	int mHasSpeed;
	MugenAssignment* mSpeed;

	MugenAssignment* mStartRotation;
	MugenAssignment* mRotationAdd;

	int mIdleAnimation;
	int mHitAnimation;

	CollisionCirc mColCirc;
} SubShotType;

typedef struct {
	int mID;
	List mSubShots;
} ShotType;


typedef struct {
	ShotType* mType;
	List mSubShots;

	CollisionData mCollisionData;
} ActiveShot;


typedef struct {
	ActiveShot* mRoot;
	
	SubShotType* mType;

	int mPhysicsID;
	int mAnimationID;
	int mCollisionID;

	Collider mCollider;
	int mListID;

	double mRotation;
} ActiveSubShot;

static struct {
	MugenSpriteFile mSprites;
	MugenAnimations mAnimations;

	IntMap mShotTypes;

	List mActiveShots;
} gData;

static ShotType* gActiveShotType;

static int isShotType(MugenDefScriptGroup* tGroup) {
	return !strcmp("Shot", tGroup->mName);
}

static void handleNewShotType(MugenDefScriptGroup* tGroup) {
	ShotType* e = allocMemory(sizeof(ShotType));
	e->mID = getMugenDefNumberVariableAsGroup(tGroup, "id");
	e->mSubShots = new_list();

	int_map_push_owned(&gData.mShotTypes, e->mID, e);

	gActiveShotType = e;
}

static int isSubShotType(MugenDefScriptGroup* tGroup) {
	return !strcmp("SubShot", tGroup->mName);
}

static void parseHomingType(SubShotType* e, MugenDefScriptGroup* tGroup) {

	char* text = getAllocatedMugenDefStringVariableAsGroup(tGroup, "type");
	if (!strcmp("normal", text)) {
		e->mHomingType = SHOT_TYPE_NORMAL;
	}
	else if (!strcmp("homing", text)) {
		e->mHomingType = SHOT_TYPE_HOMING;
	}
	else if (!strcmp("targetrandom", text)) {
		e->mHomingType = SHOT_TYPE_TARGET_RANDOM;
	}
	else {
		logError("Unrecognized type");
		logErrorString(text);
		abortSystem();
	}

	freeMemory(text);
}

static void handleNewSubShotType(MugenDefScriptGroup* tGroup) {
	assert(gActiveShotType);

	SubShotType* e = allocMemory(sizeof(SubShotType));
	fetchMugenAssignmentFromGroupAndReturnWhetherItExistsDefaultString("amount", tGroup, &e->mAmount, "");
	e->mIdleAnimation = getMugenDefNumberVariableAsGroup(tGroup, "anim");
	e->mHitAnimation = getMugenDefNumberVariableAsGroup(tGroup, "hitanim");
	fetchMugenAssignmentFromGroupAndReturnWhetherItExistsDefaultString("offset", tGroup, &e->mOffset, "");
	e->mHasAbsolutePosition = fetchMugenAssignmentFromGroupAndReturnWhetherItExists("absolute", tGroup, &e->mAbsolutePosition);
	e->mHasVelocity = fetchMugenAssignmentFromGroupAndReturnWhetherItExists("velocity", tGroup, &e->mVelocity);
	e->mHasAngle = fetchMugenAssignmentFromGroupAndReturnWhetherItExists("angle", tGroup, &e->mAngle);
	e->mHasSpeed = fetchMugenAssignmentFromGroupAndReturnWhetherItExists("speed", tGroup, &e->mSpeed);
	fetchMugenAssignmentFromGroupAndReturnWhetherItExistsDefaultString("rotation", tGroup, &e->mStartRotation, "");
	fetchMugenAssignmentFromGroupAndReturnWhetherItExistsDefaultString("rotationadd", tGroup, &e->mRotationAdd, "");

	Position center = getMugenDefVectorOrDefaultAsGroup(tGroup, "center", makePosition(0,0,0));
	double radius = getMugenDefFloatVariableAsGroup(tGroup, "radius");
	e->mColCirc = makeCollisionCirc(center, radius);
	parseHomingType(e, tGroup);

	list_push_back_owned(&gActiveShotType->mSubShots, e);
}

static void loadShotTypesFromScript(MugenDefScript* tScript) {
	gActiveShotType = NULL;

	resetMugenScriptParser();
	addMugenScriptParseFunction(isShotType, handleNewShotType);
	addMugenScriptParseFunction(isSubShotType, handleNewSubShotType);
	parseMugenScript(tScript);
}

static void loadShotHandler(void* tData) {
	(void)tData;

	gData.mSprites = loadMugenSpriteFileWithoutPalette("assets/shots/SHOTS.sff");
	gData.mAnimations = loadMugenAnimationFile("assets/shots/SHOTS.air");

	gData.mShotTypes = new_int_map();
	
	MugenDefScript script = loadMugenDefScript("assets/shots/SHOTS.def");
	loadShotTypesFromScript(&script);
	unloadMugenDefScript(script);
}

static void unloadSubShot(ActiveSubShot* e) {
	removeMugenAnimation(e->mAnimationID);
	removeFromCollisionHandler(e->mRoot->mCollisionData.mCollisionList, e->mCollisionID);
	destroyCollider(&e->mCollider);
	removeFromPhysicsHandler(e->mPhysicsID);
}

static Position getClosestEnemyPositionIncludingBoss(Position p) {
	Position closest = getClosestEnemyPosition(p);

	if (!isBossActive()) return closest;
		
	Position bPosition = getBossPosition();
	if (getDistance2D(p, closest) < 1e-6 || getDistance2D(p, bPosition) < getDistance2D(p, closest)) {
		return bPosition;
	}
	else {
		return closest;
	}
}

static void updateHoming(ActiveSubShot* e) {
	if (e->mType->mHomingType != SHOT_TYPE_HOMING) return;
	Position p = *getHandledPhysicsPositionReference(e->mPhysicsID);
	Position closestEnemy = getClosestEnemyPositionIncludingBoss(p);

	Velocity* vel = getHandledPhysicsVelocityReference(e->mPhysicsID);
	Vector3D dir = vecScale(vecNormalize(vecSub(closestEnemy, p)), vecLength(*vel));
	dir.z = 0;
	if (vecLength(dir) < 1e-6) return;

	double angle = getAngleFromDirection(dir);
	setMugenAnimationDrawAngle(e->mAnimationID, angle);
	*vel = dir;
}

static void updateRotation(ActiveSubShot* e) {
	
	SubShotType* subShot = e->mType;

	double rotationAdd = getMugenAssignmentAsFloatValueOrDefaultWhenEmpty(subShot->mRotationAdd, NULL, 0);
	e->mRotation += rotationAdd;
	setMugenAnimationDrawAngle(e->mAnimationID, e->mRotation);
}

static int updateSubShot(void* tCaller, void* tData) {
	(void)tCaller;
	ActiveSubShot* e = tData;

	updateRotation(e);
	updateHoming(e);

	Position p = *getHandledPhysicsPositionReference(e->mPhysicsID);
	if (p.x < -100 || p.x > 740 || p.y < -100 || p.y > 480) {
		unloadSubShot(e);
		return 1;
	}

	return 0;
}

static int updateShot(void* tCaller, void* tData) {
	(void)tCaller;
	ActiveShot* e = tData;

	list_remove_predicate(&e->mSubShots, updateSubShot, e);
	
	int hasNoShotsLeft = list_size(&e->mSubShots) == 0;
	return hasNoShotsLeft;
}

static void updateActiveShots() {
	list_remove_predicate(&gData.mActiveShots, updateShot, NULL);
}

static void updateShotHandler(void* tData) {
	(void)tData;
	updateActiveShots();
}

ActorBlueprint ShotHandler = {
	.mLoad = loadShotHandler,
	.mUpdate = updateShotHandler,
};

static void shotHitCB(void* tCaller, void* tCollisionData) {
	(void)tCollisionData;
	ActiveSubShot* e = tCaller;
	unloadSubShot(e); // TODO
	list_remove(&e->mRoot->mSubShots, e->mListID);
}

static Position getRandomEnemyOrBossPosition() {
	Position p = getRandomEnemyPosition();
	
	if (!isBossActive()) {
		if (p.x == INF) p = makePosition(randfrom(-100, 740), randfrom(-100, 580), 0);
		return p;
	}

	Position bossP = getBossPosition();
	if (p.x == INF) return bossP;
	
	int amount = getEnemyAmount();
	double probability = 1 / (amount + 1);
	if (randfrom(0, 1) <= probability) return bossP;
	else return p;
}

typedef struct {
	ActiveShot* mRoot;
	Position mPosition;
} SubShotCaller;

static void swap(double* a, double* b) {
	double c = *a;
	*a = *b;
	*b = c;
}

typedef struct {
	ActiveSubShot* mActiveShot;
	SubShotCaller* mActiveCaller;
	Position* mOffsetReference; // TODO: better
	int i;
} SubShotAssignmentParseCaller;

void getCurrentSubShotIndex(char* tOutput, void* tCaller) {
	SubShotAssignmentParseCaller* caller = tCaller;

	sprintf(tOutput, "%d", caller->i);
}

void getShotAngleTowardsPlayer(char* tOutput, void* tCaller) {
	SubShotAssignmentParseCaller* caller = tCaller;

	Position pos = vecAdd(caller->mActiveCaller->mPosition, *caller->mOffsetReference);
	Position playerPos = getPlayerPosition();

	Vector3D direction = vecNormalize(vecSub(playerPos, pos));
	direction.x *= -1; // TODO
	double angle = getAngleFromDirection(direction);

	sprintf(tOutput, "%f", angle);
}

static void addSingleSubShot(SubShotCaller* caller, SubShotType* subShot, int i) {
	ActiveSubShot* e = allocMemory(sizeof(ActiveSubShot));
	e->mType = subShot;
	e->mRoot = caller->mRoot;

	SubShotAssignmentParseCaller assignmentCaller;
	assignmentCaller.mActiveCaller = caller;
	assignmentCaller.mActiveShot = e;
	assignmentCaller.i = i;

	Vector3D offset = getMugenAssignmentAsVector3DValueOrDefaultWhenEmpty(subShot->mOffset, &assignmentCaller, makePosition(0, 0, 0));
	Vector3D velocity = makePosition(0, 0, 0);
	assignmentCaller.mOffsetReference = &offset; // TODO: better

	if (subShot->mHasAbsolutePosition) {
		caller->mPosition = getMugenAssignmentAsVector3DValueOrDefaultWhenEmpty(subShot->mAbsolutePosition, &assignmentCaller, makePosition(0, 0, 0));
	}

	if (subShot->mHasVelocity) {
		velocity = getMugenAssignmentAsVector3DValueOrDefaultWhenEmpty(subShot->mVelocity, &assignmentCaller, makePosition(0, 0, 0));
	}
	double angle = 0;
	if (subShot->mHasAngle) {
		angle = evaluateMugenAssignmentAndReturnAsFloat(subShot->mAngle, &assignmentCaller);
		velocity = getDirectionFromAngleZ(angle);
		offset = vecRotateZ(offset, angle);

		angle *= -1; // TODO: fix
	}
	
	if (subShot->mHomingType == SHOT_TYPE_TARGET_RANDOM) {
		Position p = vecAdd(caller->mPosition, offset);
		Position enemyPos = getRandomEnemyOrBossPosition();
		swap(&enemyPos.x, &p.x);
		double angle = getAngleFromDirection(vecSub(enemyPos, p));
		velocity = getDirectionFromAngleZ(angle);
		offset = vecRotateZ(offset, angle);
	}

	if (subShot->mHasSpeed) {
		double speed = evaluateMugenAssignmentAndReturnAsFloat(subShot->mSpeed, &assignmentCaller);
		velocity = vecScale(vecNormalize(velocity), speed);
	}

	e->mPhysicsID = addToPhysicsHandler(vecAdd(caller->mPosition, offset));
	addAccelerationToHandledPhysics(e->mPhysicsID, velocity);
	setHandledPhysicsMaxVelocity(e->mPhysicsID, vecLength(velocity));

	e->mCollider = makeColliderFromCirc(subShot->mColCirc);
	e->mCollisionID = addColliderToCollisionHandler(caller->mRoot->mCollisionData.mCollisionList, getHandledPhysicsPositionReference(e->mPhysicsID), e->mCollider, shotHitCB, e, &caller->mRoot->mCollisionData);

	e->mAnimationID = addMugenAnimation(getMugenAnimation(&gData.mAnimations, subShot->mIdleAnimation), &gData.mSprites, makePosition(0, 0, 10));
	setMugenAnimationBasePosition(e->mAnimationID, getHandledPhysicsPositionReference(e->mPhysicsID));

	e->mRotation = getMugenAssignmentAsFloatValueOrDefaultWhenEmpty(subShot->mStartRotation, &assignmentCaller, angle);
	setMugenAnimationDrawAngle(e->mAnimationID, e->mRotation);

	e->mListID = list_push_back_owned(&caller->mRoot->mSubShots, e);
}

static void addSubShot(void* tCaller, void* tData) {
	SubShotCaller* caller = tCaller;
	SubShotType* subShot = tData;

	int amount = getMugenAssignmentAsIntegerValueOrDefaultWhenEmpty(subShot->mAmount, NULL, 1);
	int i;
	for (i = 0; i < amount; i++) {
		addSingleSubShot(caller, subShot, i);
	}

	
}

void addShot(int tID, int tCollisionList, Position tPosition)
{
	ActiveShot* e = allocMemory(sizeof(ActiveShot));
	assert(int_map_contains(&gData.mShotTypes, tID));
	e->mType = int_map_get(&gData.mShotTypes, tID);
	e->mCollisionData.mCollisionList = tCollisionList;
	e->mSubShots = new_list();
	list_push_back_owned(&gData.mActiveShots, e);

	SubShotCaller caller;
	caller.mRoot = e;
	caller.mPosition = tPosition;
	list_map(&e->mType->mSubShots, addSubShot, &caller);
}

typedef struct {
	int mCollisionList;
} RemoveShotsForSingleListCaller;

static int removeShotTypeForSingleSubShot(void* tCaller, void* tData) {
	(void)tCaller;
	ActiveSubShot* e = tData;
	unloadSubShot(e);
	return 1;
}

static void removeShotTypeForSingleShot(void* tCaller, void* tData) {
	ActiveShot* e = tData;
	RemoveShotsForSingleListCaller* caller = tCaller;
	if (caller->mCollisionList != e->mCollisionData.mCollisionList) return;

	list_remove_predicate(&e->mSubShots, removeShotTypeForSingleSubShot, NULL);
}

void removeEnemyShots()
{
	RemoveShotsForSingleListCaller caller;
	caller.mCollisionList = getEnemyShotCollisionList();
	list_map(&gData.mActiveShots, removeShotTypeForSingleShot, &caller);
}
