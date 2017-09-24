#include "player.h"

#include <tari/animation.h>
#include <tari/input.h>
#include <tari/physicshandler.h>
#include <tari/collisionhandler.h>

#include "collision.h"
#include "shothandler.h"
#include "ui.h"

static struct {
	TextureData mIdleTextures[10];
	Animation mIdleAnimation;

	TextureData mHitboxTexture;
	int mHitBoxAnimationID;

	int mAnimationID;
	int mPhysicsID;
	int mCollisionID;

	CollisionData mCollisionData;
	Collider mCollider;

	int mItemCollisionID;
	Collider mItemCollider;

	double mAcceleration;
	double mFocusSpeed;
	double mNormalSpeed;
	int mIsFocused;

	int mIsInCooldown;
	Duration mCooldownNow;
	Duration mCooldownDuration;

	int mIsBombing;
	Duration mBombNow;
	Duration mBombDuration;

	int mPower;

	int mLocalBombCount;
	int mLocalDeathCount;
} gData;

static void playerHitCB(void* tCaller, void* tCollisionData);

static void loadPlayer(void* tData) {
	(void)tData;
	gData.mIdleAnimation = createEmptyAnimation();
	gData.mIdleAnimation.mFrameAmount = 1;
	loadConsecutiveTextures(gData.mIdleTextures, "assets/player/KAT.pkg", gData.mIdleAnimation.mFrameAmount);

	gData.mPhysicsID = addToPhysicsHandler(makePosition(40, 200, 3));
	setHandledPhysicsDragCoefficient(gData.mPhysicsID, makePosition(0.3, 0.3, 0));

	gData.mAnimationID = playAnimationLoop(makePosition(-(23 + 8), -(12 + 8), 0), gData.mIdleTextures, gData.mIdleAnimation, makeRectangleFromTexture(gData.mIdleTextures[0]));
	setAnimationBasePositionReference(gData.mAnimationID, getHandledPhysicsPositionReference(gData.mPhysicsID));

	gData.mCollisionData.mCollisionList = getPlayerCollisionList();
	gData.mCollider = makeColliderFromCirc(makeCollisionCirc(makePosition(0, 0, 0), 5));
	gData.mCollisionID = addColliderToCollisionHandler(getPlayerCollisionList(), getHandledPhysicsPositionReference(gData.mPhysicsID), gData.mCollider, playerHitCB, NULL, &gData.mCollisionData);

	gData.mItemCollider = makeColliderFromCirc(makeCollisionCirc(makePosition(0, 0, 0), 40));
	gData.mItemCollisionID = addColliderToCollisionHandler(getPlayerItemCollisionList(), getHandledPhysicsPositionReference(gData.mPhysicsID), gData.mItemCollider, playerHitCB, NULL, &gData.mCollisionData);
	
	gData.mHitboxTexture = loadTexture("assets/debug/collision_circ.pkg");
	gData.mHitBoxAnimationID = playOneFrameAnimationLoop(makePosition(-8, -8, 1), &gData.mHitboxTexture);
	setAnimationBasePositionReference(gData.mHitBoxAnimationID, getHandledPhysicsPositionReference(gData.mPhysicsID));
	setAnimationSize(gData.mHitBoxAnimationID, makePosition(10, 10, 0), makePosition(8, 8, 0));
	setAnimationTransparency(gData.mHitBoxAnimationID, 0);
	
	gData.mAcceleration = 2;
	gData.mNormalSpeed = 4;
	gData.mFocusSpeed = 1;
	setHandledPhysicsMaxVelocity(gData.mPhysicsID, gData.mNormalSpeed);

	gData.mIsInCooldown = 0;
	gData.mCooldownNow = 0;
	gData.mCooldownDuration = 20;

	gData.mIsBombing = 0;
	gData.mBombNow = 0;
	gData.mBombDuration = 180;

	gData.mPower = 0;
	gData.mIsFocused = 0;

	gData.mLocalBombCount = 0;
	gData.mLocalDeathCount = 0;
}

static void updateMovement() {
	if (hasPressedLeft()) {
		addAccelerationToHandledPhysics(gData.mPhysicsID, makePosition(-gData.mAcceleration, 0, 0));
	}
	if (hasPressedRight()) {
		addAccelerationToHandledPhysics(gData.mPhysicsID, makePosition(gData.mAcceleration, 0, 0));
	}
	if (hasPressedUp()) {
		addAccelerationToHandledPhysics(gData.mPhysicsID, makePosition(0, -gData.mAcceleration, 0));
	}
	if (hasPressedDown()) {
		addAccelerationToHandledPhysics(gData.mPhysicsID, makePosition(0, gData.mAcceleration, 0));
	}
}

static void updateFocus() {
	if (hasPressedR()) {
		setHandledPhysicsMaxVelocity(gData.mPhysicsID, gData.mFocusSpeed);
		setAnimationTransparency(gData.mHitBoxAnimationID, 1);
		gData.mIsFocused = 1;
	}
	else {
		setHandledPhysicsMaxVelocity(gData.mPhysicsID, gData.mNormalSpeed);
		setAnimationTransparency(gData.mHitBoxAnimationID, 0);
		gData.mIsFocused = 0;
	}
}

static void firePlayerShot() {
	Position p = *getHandledPhysicsPositionReference(gData.mPhysicsID);

	int powerBase = gData.mPower / 100;
	int shotID = gData.mIsFocused * 10 + powerBase;
	addShot(shotID, getPlayerShotCollisionList(), p);

	gData.mCooldownNow = 0;
	gData.mIsInCooldown = 1;
}

static void updateShot() {
	if (gData.mIsInCooldown) {
		if (handleDurationAndCheckIfOver(&gData.mCooldownNow, gData.mCooldownDuration)) {
			gData.mIsInCooldown = 0;
		}
		return;
	}

	if (hasPressedA()) {
		firePlayerShot();
	}
}

static void updateBomb() {
	if (gData.mIsBombing) {
		removeEnemyShots();
		Position p = *getHandledPhysicsPositionReference(gData.mPhysicsID);
		addShot(20, getPlayerShotCollisionList(), p);
		if (handleDurationAndCheckIfOver(&gData.mBombNow, gData.mBombDuration)) {
			gData.mIsBombing = 0;
		}
		return;
	}
	
	
	if (hasPressedBFlank()) {
		gData.mIsBombing = 1;
		gData.mBombNow = 0;
	}

}

static void updatePlayer(void* tData) {
	(void)tData;
	updateMovement();
	updateFocus();
	updateShot();
	updateBomb();
}

ActorBlueprint Player = {
	.mLoad = loadPlayer,
	.mUpdate = updatePlayer,
};

static void handlePowerItemCollection() {
	gData.mPower++;
	setPowerText(gData.mPower);
}

static void playerHitCB(void* tCaller, void* tCollisionData) {
	(void)tCaller;
	CollisionData* collisionData = tCollisionData;

	if (collisionData->mCollisionList == getPowerItemCollisionList()) {
		handlePowerItemCollection();
		return;
	}

	printf("ded\n"); // TODO

}

Position getPlayerPosition()
{
	return *getHandledPhysicsPositionReference(gData.mPhysicsID);
}

void resetLocalPlayerCounts()
{
	gData.mLocalBombCount = 0;
	gData.mLocalDeathCount = 0;
}

void getLocalDeathCountVariable(char * tDst, void * tCaller)
{
	(void)tCaller;
	sprintf(tDst, "%d", gData.mLocalDeathCount);
}

void getLocalBombCountVariable(char * tDst, void * tCaller)
{
	(void)tCaller;
	sprintf(tDst, "%d", gData.mLocalBombCount);
}
