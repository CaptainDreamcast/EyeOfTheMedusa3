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
#include <tari/wrapper.h>

#include "collision.h"
#include "enemyhandler.h"
#include "boss.h"
#include "player.h"

typedef enum {
	SHOT_TYPE_NORMAL,
	SHOT_TYPE_HOMING,
	SHOT_TYPE_HOMING_FINAL,
	SHOT_TYPE_TARGET_RANDOM,
	SHOT_TYPE_TARGET_RANDOM_FINAL,
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

	MugenAssignment* mGimmick;

	int mIdleAnimation;
	int mHitAnimation;

	CollisionCirc mColCirc;
	MugenAssignment* mColor;
} SubShotType;

typedef struct {
	int mID;
	IntMap mSubShots;
} ShotType;


typedef struct {
	ShotType* mType;
	IntMap mSubShots;
	int mSubShotsLeft;

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

	int mHasGimmickData;
	void* mGimmickData;

	int mIsStillActive;
} ActiveSubShot;

static struct {
	MugenSpriteFile mSprites;
	MugenAnimations mAnimations;

	IntMap mShotTypes;

	IntMap mActiveShots;
} gData;

static ShotType* gActiveShotType;

static int isShotType(MugenDefScriptGroup* tGroup) {
	return !strcmp("Shot", tGroup->mName);
}

static void handleNewShotType(MugenDefScriptGroup* tGroup) {
	ShotType* e = allocMemory(sizeof(ShotType));
	e->mID = getMugenDefNumberVariableAsGroup(tGroup, "id");
	e->mSubShots = new_int_map();

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
	else if (!strcmp("homing_final", text)) {
		e->mHomingType = SHOT_TYPE_HOMING_FINAL;
	}
	else if (!strcmp("targetrandom", text)) {
		e->mHomingType = SHOT_TYPE_TARGET_RANDOM;
	}
	else if (!strcmp("targetrandom_final", text)) {
		e->mHomingType = SHOT_TYPE_TARGET_RANDOM_FINAL;
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
	fetchMugenAssignmentFromGroupAndReturnWhetherItExistsDefaultString("color", tGroup, &e->mColor, "white");
	fetchMugenAssignmentFromGroupAndReturnWhetherItExistsDefaultString("gimmick", tGroup, &e->mGimmick, "");

	Position center = getMugenDefVectorOrDefaultAsGroup(tGroup, "center", makePosition(0, 0, 0));
	double radius = getMugenDefFloatVariableAsGroup(tGroup, "radius");
	e->mColCirc = makeCollisionCirc(center, radius);
	parseHomingType(e, tGroup);

	int_map_push_back_owned(&gActiveShotType->mSubShots, e);
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
	gData.mActiveShots = new_int_map();

	MugenDefScript script = loadMugenDefScript("assets/shots/SHOTS.def");
	loadShotTypesFromScript(&script);
	unloadMugenDefScript(script);
}

static void unloadSubShot(ActiveSubShot* e) {
	if (e->mHasGimmickData) {
		freeMemory(e->mGimmickData);
	}

	removeMugenAnimation(e->mAnimationID);
	removeFromCollisionHandler(e->mRoot->mCollisionData.mCollisionList, e->mCollisionID);
	destroyCollider(&e->mCollider);
	removeFromPhysicsHandler(e->mPhysicsID);
	e->mRoot->mSubShotsLeft--;
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

static void updateFinalHoming(ActiveSubShot* e) {
	if (e->mType->mHomingType != SHOT_TYPE_HOMING_FINAL) return;
	Position p = *getHandledPhysicsPositionReference(e->mPhysicsID);
	Position closestEnemy = getPlayerPosition();

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

static void updateGimmick(ActiveSubShot* e) {
	getMugenAssignmentAsIntegerValueOrDefaultWhenEmpty(e->mType->mGimmick, e, 0);
}

static int updateSubShot(void* tCaller, void* tData) {
	(void)tCaller;
	ActiveSubShot* e = tData;

	updateRotation(e);
	updateHoming(e);
	updateFinalHoming(e);
	updateGimmick(e);

	Position p = *getHandledPhysicsPositionReference(e->mPhysicsID);
	if (!e->mIsStillActive || p.x < -100 || p.x > 740 || p.y < -100 || p.y > 480) {
		unloadSubShot(e);
		return 1;
	}

	return 0;
}

static int unloadSubShotCB(void* tCaller, void* tData) {
	(void)tCaller;
	ActiveSubShot* e = tData;
	unloadSubShot(e);
	return 1;
}

static void unloadShot(ActiveShot* e) {
	int_map_remove_predicate(&e->mSubShots, unloadSubShotCB, NULL);
	delete_int_map(&e->mSubShots);
}

static int updateShot(void* tCaller, void* tData) {
	(void)tCaller;
	ActiveShot* e = tData;

	int_map_remove_predicate(&e->mSubShots, updateSubShot, e);

	if (!e->mSubShotsLeft) {
		unloadShot(e);
		return 1;
	}

	return 0;
}

static void updateActiveShots() {
	int_map_remove_predicate(&gData.mActiveShots, updateShot, NULL);
}

static void updateShotHandler(void* tData) {
	(void)tData;
	if (isWrapperPaused()) return;
	updateActiveShots();
}

ActorBlueprint ShotHandler = {
	.mLoad = loadShotHandler,
	.mUpdate = updateShotHandler,
};

static void shotHitCB(void* tCaller, void* tCollisionData) {
	(void)tCollisionData;
	ActiveSubShot* e = tCaller;
	unloadSubShot(e); 
	int_map_remove(&e->mRoot->mSubShots, e->mListID);
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

static void setShotColor(SubShotType* subShot, ActiveSubShot* e, SubShotAssignmentParseCaller* caller) {
	double r, g, b;

	char* text = evaluateMugenAssignmentAndReturnAsAllocatedString(subShot->mColor, caller);

	if (!strcmp("white", text)) {
		r = g = b = 1;
	}
	else if (!strcmp("red", text)) {
		r = 1;
		b = g = 0;
	}
	else if (!strcmp("grey", text)) {
		r = g = b = 0.5;
	}
	else if (!strcmp("yellow", text)) {
		r = g = 1;
		b = 0;
	}
	else if (!strcmp("rainbow", text)) {
		r = randfromInteger(0, 1);
		g = randfromInteger(0, 1);
		b = randfromInteger(0, 1);
		if (!r && !g && !b) r = 1;
	}
	else if (!strcmp("green", text)) {
		r = 0;
		g = 1;
		b = 0;
	}
	else {
		r = g = b = 1;
		logError("Unrecognized color.");
		logErrorString(text);
		abortSystem();
	}

	setMugenAnimationColor(e->mAnimationID, r, g, b);
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

	double angle = 0;
	if (subShot->mHasVelocity) {
		velocity = getMugenAssignmentAsVector3DValueOrDefaultWhenEmpty(subShot->mVelocity, &assignmentCaller, makePosition(0, 0, 0));
		angle = getAngleFromDirection(velocity);
	}
	
	if (subShot->mHasAngle) {
		angle = evaluateMugenAssignmentAndReturnAsFloat(subShot->mAngle, &assignmentCaller);
		velocity = getDirectionFromAngleZ(angle);	
		angle *= -1; // TODO: fix
	}

	if (subShot->mHomingType == SHOT_TYPE_TARGET_RANDOM || subShot->mHomingType == SHOT_TYPE_TARGET_RANDOM_FINAL) {
		Position p = vecAdd(caller->mPosition, offset);
		Position enemyPos;
		if (subShot->mHomingType == SHOT_TYPE_TARGET_RANDOM) {
			enemyPos = getRandomEnemyOrBossPosition();
		}
		else {
			enemyPos = getPlayerPosition();
		}
		
		swap(&enemyPos.x, &p.x);
		angle = getAngleFromDirection(vecSub(enemyPos, p));
		velocity = getDirectionFromAngleZ(angle);
		//offset = vecRotateZ(offset, angle);
		angle *= -1; // TODO: fix
	}
	
	if (subShot->mHasSpeed) {
		double speed = evaluateMugenAssignmentAndReturnAsFloat(subShot->mSpeed, &assignmentCaller);
		velocity = vecScale(vecNormalize(velocity), speed);
	}
	 
	e->mPhysicsID = addToPhysicsHandler(vecAdd(caller->mPosition, offset));
	addAccelerationToHandledPhysics(e->mPhysicsID, velocity);

	e->mCollider = makeColliderFromCirc(subShot->mColCirc);
	e->mCollisionID = addColliderToCollisionHandler(caller->mRoot->mCollisionData.mCollisionList, getHandledPhysicsPositionReference(e->mPhysicsID), e->mCollider, shotHitCB, e, &caller->mRoot->mCollisionData);

	double z;
	if(caller->mRoot->mCollisionData.mCollisionList == getEnemyShotCollisionList()) z = 30;
	else z = 25;
	e->mAnimationID = addMugenAnimation(getMugenAnimation(&gData.mAnimations, subShot->mIdleAnimation), &gData.mSprites, makePosition(0, 0, z));
	setMugenAnimationBasePosition(e->mAnimationID, getHandledPhysicsPositionReference(e->mPhysicsID));

	e->mRotation = getMugenAssignmentAsFloatValueOrDefaultWhenEmpty(subShot->mStartRotation, &assignmentCaller, angle);
	setMugenAnimationDrawAngle(e->mAnimationID, e->mRotation);

	setShotColor(subShot, e, &assignmentCaller);

	e->mHasGimmickData = 0;
	e->mIsStillActive = 1;

	caller->mRoot->mSubShotsLeft++;
	e->mListID = int_map_push_back_owned(&caller->mRoot->mSubShots, e);
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
	e->mCollisionData.mIsItem = 0;
	e->mSubShotsLeft = 0;
	e->mSubShots = new_int_map();
	int_map_push_back_owned(&gData.mActiveShots, e);

	SubShotCaller caller;
	caller.mRoot = e;
	caller.mPosition = tPosition;
	int_map_map(&e->mType->mSubShots, addSubShot, &caller);
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

	int_map_remove_predicate(&e->mSubShots, removeShotTypeForSingleSubShot, NULL);
}

void removeEnemyShots()
{
	RemoveShotsForSingleListCaller caller;
	caller.mCollisionList = getEnemyShotCollisionList();
	int_map_map(&gData.mActiveShots, removeShotTypeForSingleShot, &caller);
}

typedef struct {
	Position mTarget;
	int mState;
} BigBangData;

static void bangOut(BigBangData* data) {
	int side = randfromInteger(0, 3);
	if (side == 0) {
		data->mTarget = makePosition(randfrom(0, 640), 0, 0);
	}
	else if (side == 1) {
		data->mTarget = makePosition(randfrom(0, 640), 327, 0);
	}
	else if (side == 2) {
		data->mTarget = makePosition(0, randfrom(0, 327), 0);
	}
	else {
		data->mTarget = makePosition(0, randfrom(0, 327), 0);
	}

	data->mState = 0;
}

static void bangIn(BigBangData* data) {

	data->mTarget = makePosition(320, 163, 0);
	data->mState = 1;
}

static void loadBigBang(ActiveSubShot* e) {
	e->mGimmickData = allocMemory(sizeof(BigBangData));
	e->mHasGimmickData = 1;

	BigBangData* data = e->mGimmickData;
	bangOut(data);
}

void evaluateBigBangFunction(char * tDst, void * tCaller)
{
	ActiveSubShot* e = tCaller;

	if (!e->mHasGimmickData) {
		loadBigBang(e);
	}
	
	BigBangData* data = e->mGimmickData;
	Position pos = *getHandledPhysicsPositionReference(e->mPhysicsID);
	Vector3D* vel = getHandledPhysicsVelocityReference(e->mPhysicsID);
	
	Vector3D delta = vecSub(data->mTarget, pos);
	double l = vecLength(delta);
	if (l < 2) {
		if (data->mState) bangOut(data);
		else bangIn(data);
	}
		
	*vel = vecScale(vecNormalize(delta), 2);
	
	strcpy(tDst, "");
}

void evaluateBounceFunction(char * tDst, void * tCaller)
{
	ActiveSubShot* e = tCaller;
	Position pos = *getHandledPhysicsPositionReference(e->mPhysicsID);
	Velocity* vel = getHandledPhysicsVelocityReference(e->mPhysicsID);

	if (pos.x < 0) vel->x = 1;
	if (pos.x > 640) vel->x = -1;

	strcpy(tDst, "");
}

typedef struct {

	Velocity mDirection;
	double mState;
	int mIsActive;
} AckermannData;

static void initAckermann(ActiveSubShot* e) {
	e->mGimmickData = allocMemory(sizeof(AckermannData));
	e->mHasGimmickData = 1;

	AckermannData* data = e->mGimmickData;
	data->mIsActive = 0;
}

void evaluateAckermannFunction(char * tDst, void * tCaller)
{
	ActiveSubShot* e = tCaller;
	if (!e->mHasGimmickData) {
		initAckermann(e);
	}

	Velocity* vel = getHandledPhysicsVelocityReference(e->mPhysicsID);
	AckermannData* data = e->mGimmickData;
	if (!data->mIsActive) {
		if (vecLength(*vel) > 0 && randfrom(0, 1) < 0.005) {
			data->mIsActive = 1;
			data->mState = 0;
			data->mDirection = *vel;
		}
	}
	else {
		data->mState = min(data->mState + 1 / 60.0, 1);
		*vel = vecRotateZ(data->mDirection, 2*M_PI*data->mState);
		double angle = getAngleFromDirection(*vel);
		setMugenAnimationDrawAngle(e->mAnimationID, angle);
	}

	strcpy(tDst, "");
}

void evaluateSwirlFunction(char * tDst, void * tCaller)
{
	ActiveSubShot* e = tCaller;
	Velocity* vel = getHandledPhysicsVelocityReference(e->mPhysicsID);
	*vel = vecRotateZ(*vel, 0.01);
	double angle = getAngleFromDirection(*vel);
	setMugenAnimationDrawAngle(e->mAnimationID, angle);

	strcpy(tDst, "");
}

void evaluateBlamFunction(char * tDst, void * tCaller)
{

	ActiveSubShot* e = tCaller;
	Velocity* vel = getHandledPhysicsVelocityReference(e->mPhysicsID);
	
	double l = vecLength(*vel);
	if (l > 0) {
		*vel = vecScale(vecNormalize(*vel), min(l*1.1, 20));
	}

	strcpy(tDst, "");
}


typedef struct {
	Duration mNow;	
} TransienceData;

static void initTransience(ActiveSubShot* e) {
	e->mGimmickData = allocMemory(sizeof(TransienceData));
	e->mHasGimmickData = 1;

	TransienceData* data = e->mGimmickData;
	data->mNow = 0;
}

void evaluateTransienceFunction(char * tDst, void * tCaller)
{
	ActiveSubShot* e = tCaller;
	if (!e->mHasGimmickData) {
		initTransience(e);
	}
	TransienceData* data = e->mGimmickData;

	if (handleDurationAndCheckIfOver(&data->mNow, 60)) {
		e->mIsStillActive = 0;
	}

	strcpy(tDst, "");
}
