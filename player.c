#include "player.h"

#include <tari/animation.h>
#include <tari/input.h>
#include <tari/physicshandler.h>
#include <tari/collisionhandler.h>
#include <tari/log.h>
#include <tari/system.h>
#include <tari/math.h>
#include <tari/screeneffect.h>
#include <tari/wrapper.h>
#include <tari/mugenanimationhandler.h>

#include "collision.h"
#include "shothandler.h"
#include "ui.h"
#include "effecthandler.h"
#include "titlescreen.h"
#include "continuehandler.h"
#include "gameoverscreen.h"
#include "boss.h"

static struct {
	MugenSpriteFile mSprites;
	MugenAnimations mAnimations;

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
	int mIsFinalBossBombing;

	int mIsDying;
	Duration mDyingNow;
	Duration mDyingDuration;

	int mPower;

	int mLocalBombCount;
	int mLocalDeathCount;

	int mLifeAmount;
	int mBombAmount;
	int mContinueAmount;

	int mIsHit;
	Duration mIsHitNow;
	Duration mIsHitDuration;

	int mCanBeHitByEnemies;
} gData;

static void playerHitCB(void* tCaller, void* tCollisionData);

static void loadPlayer(void* tData) {
	(void)tData;
	setLifeText(gData.mLifeAmount);
	setBombText(gData.mBombAmount);
	setPowerText(gData.mPower);

	gData.mSprites = loadMugenSpriteFileWithoutPalette("assets/player/PLAYER.sff");
	gData.mAnimations = loadMugenAnimationFile("assets/player/PLAYER.air");

	gData.mPhysicsID = addToPhysicsHandler(makePosition(40, 200, 0));
	setHandledPhysicsDragCoefficient(gData.mPhysicsID, makePosition(0.3, 0.3, 0));

	gData.mAnimationID = addMugenAnimation(getMugenAnimation(&gData.mAnimations, 1), &gData.mSprites, makePosition(0, 0, 10));
	setMugenAnimationBasePosition(gData.mAnimationID, getHandledPhysicsPositionReference(gData.mPhysicsID));

	gData.mCollisionData.mCollisionList = getPlayerCollisionList();
	gData.mCollider = makeColliderFromCirc(makeCollisionCirc(makePosition(0, 0, 0), 2));
	gData.mCollisionID = addColliderToCollisionHandler(getPlayerCollisionList(), getHandledPhysicsPositionReference(gData.mPhysicsID), gData.mCollider, playerHitCB, NULL, &gData.mCollisionData);

	gData.mItemCollider = makeColliderFromCirc(makeCollisionCirc(makePosition(0, 0, 0), 40));
	gData.mItemCollisionID = addColliderToCollisionHandler(getPlayerItemCollisionList(), getHandledPhysicsPositionReference(gData.mPhysicsID), gData.mItemCollider, playerHitCB, NULL, &gData.mCollisionData);
	
	gData.mHitboxTexture = loadTexture("assets/debug/collision_circ.pkg");
	gData.mHitBoxAnimationID = playOneFrameAnimationLoop(makePosition(-8, -8, 35), &gData.mHitboxTexture);
	setAnimationBasePositionReference(gData.mHitBoxAnimationID, getHandledPhysicsPositionReference(gData.mPhysicsID));
	setAnimationSize(gData.mHitBoxAnimationID, makePosition(4, 4, 0), makePosition(8, 8, 0));
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

	gData.mIsFocused = 0;

	gData.mLocalBombCount = 0;
	gData.mLocalDeathCount = 0;

	gData.mIsHit = 0;
	gData.mIsHitDuration = 120;

	gData.mIsDying = 0;
	gData.mDyingDuration = 5;

	gData.mCanBeHitByEnemies = 1;
}

static void updateMovement() {
	Position* pos = getHandledPhysicsPositionReference(gData.mPhysicsID);
	*pos = clampPositionToGeoRectangle(*pos, makeGeoRectangle(0, 0, 640, 327));
	
	if (hasPressedLeftSingle(0) || hasPressedLeftSingle(1)) {
		addAccelerationToHandledPhysics(gData.mPhysicsID, makePosition(-gData.mAcceleration, 0, 0));
	}
	if (hasPressedRightSingle(0)|| hasPressedRightSingle(1)) {
		addAccelerationToHandledPhysics(gData.mPhysicsID, makePosition(gData.mAcceleration, 0, 0));
	}
	if (hasPressedUpSingle(0) || hasPressedUpSingle(1)) {
		addAccelerationToHandledPhysics(gData.mPhysicsID, makePosition(0, -gData.mAcceleration, 0));
	}
	if (hasPressedDownSingle(0) || hasPressedDownSingle(1)) {
		addAccelerationToHandledPhysics(gData.mPhysicsID, makePosition(0, gData.mAcceleration, 0));
	}
}

static void updateFocus() {
	if (hasPressedRSingle(0) || hasPressedRSingle(1)) {
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

	if (hasPressedASingle(0)) {
		addFinalBossShot(shotID + 30);
	}

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

	if (hasPressedASingle(0) || hasPressedASingle(1)) {
		firePlayerShot();
	}
}

static void updateBomb() {
	if (gData.mIsBombing) {
		removeEnemyShots();
		Position p = *getHandledPhysicsPositionReference(gData.mPhysicsID);
		addShot(20, getPlayerShotCollisionList(), p);
		if (gData.mIsFinalBossBombing) {
			addFinalBossShot(50);
		}

		if (handleDurationAndCheckIfOver(&gData.mBombNow, gData.mBombDuration)) {
			gData.mIsBombing = 0;
			if (gData.mIsFinalBossBombing) {
				setFinalBossVulnerable();
			}
		}
		return;
	}
	
	if (!gData.mBombAmount) return;

	int isSecondPort = hasPressedBFlankSingle(1);
	if (hasPressedBFlankSingle(0) || isSecondPort) {
		gData.mLocalBombCount++;
		gData.mBombAmount--;
		setBombText(gData.mBombAmount);
		gData.mIsBombing = 1;
		gData.mBombNow = 0;
		gData.mIsDying = 0;

		gData.mIsFinalBossBombing = !isSecondPort;
		if (gData.mIsFinalBossBombing) {
			setFinalBossInvincible();
		}
	}

}

static void updateBeingHit() {
	if (!gData.mIsHit) return;

	if (handleDurationAndCheckIfOver(&gData.mIsHitNow, gData.mIsHitDuration)) {
		gData.mIsHit = 0;
		setMugenAnimationTransparency(gData.mAnimationID, 1);
	}
}

static void setDead();

static void updateDying() {
	if (!gData.mIsDying) return;

	if (handleDurationAndCheckIfOver(&gData.mDyingNow, gData.mDyingDuration)) {
		setDead();
	}
}

static void handleSmallPowerItemCollection() {
	gData.mPower = min(400, gData.mPower + 1);
	setPowerText(gData.mPower);
}

static void updatePlayer(void* tData) {
	(void)tData;
	if (isWrapperPaused()) return;

	updateMovement();
	updateFocus();
	updateShot();
	updateBomb();
	updateBeingHit();
	updateDying();
}

ActorBlueprint Player = {
	.mLoad = loadPlayer,
	.mUpdate = updatePlayer,
};

static void handleLifeItemCollection() {
	gData.mLifeAmount++;
	setLifeText(gData.mLifeAmount);
}

static void handleBombItemCollection() {
	gData.mBombAmount++;
	setBombText(gData.mBombAmount);
}

static void handleItemCollection(CollisionData* tCollisionData) {
	if (tCollisionData->mItemType == ITEM_TYPE_SMALL_POWER) {
		handleSmallPowerItemCollection();
	} else if (tCollisionData->mItemType == ITEM_TYPE_LIFE) {
		handleLifeItemCollection();
	}
	else if (tCollisionData->mItemType == ITEM_TYPE_BOMB) {
		handleBombItemCollection();
	}
	else {
		logError("Unrecognized item type.");
		logErrorInteger(tCollisionData->mItemType);
		abortSystem();
	}
}

static void setHit() {
	Position pos = *getHandledPhysicsPositionReference(gData.mPhysicsID);
	addExplosionEffect(pos);

	setMugenAnimationTransparency(gData.mAnimationID, 0.5);
	gData.mIsHit = 1;
	gData.mIsHitNow = 0;
}

static void goToGameOverScreen(void* tCaller) {
	setNewScreen(&GameOverScreen); 
}

static void setDead() {
	gData.mLocalDeathCount++;

	gData.mPower = max(0, gData.mPower - 100);
	setPowerText(gData.mPower);

	gData.mBombAmount = max(3, gData.mBombAmount);
	setBombText(gData.mBombAmount);

	if (!gData.mLifeAmount) {
		if (gData.mContinueAmount) {
			setContinueActive();
		}
		else {
			addFadeOut(30, goToGameOverScreen, NULL);
		}
	}
	else {
		gData.mLifeAmount--;
		setLifeText(gData.mLifeAmount);
	}

	setHit();
	gData.mIsDying = 0;
}

static void playerHitCB(void* tCaller, void* tCollisionData) {
	(void)tCaller;
	CollisionData* collisionData = tCollisionData;

	if (collisionData->mIsItem) {
		handleItemCollection(collisionData);
		return;
	}

	if (gData.mIsHit || gData.mIsBombing) return;
	if (!gData.mCanBeHitByEnemies && collisionData->mCollisionList == getEnemyCollisionList()) return;
	if (gData.mIsDying) return;

	gData.mIsDying = 1;
	gData.mDyingNow = 0;
}

Position getPlayerPosition()
{
	return *getHandledPhysicsPositionReference(gData.mPhysicsID);
}

PhysicsObject * getPlayerPhysics()
{
	return getPhysicsFromHandler(gData.mPhysicsID);
}

double getPlayerAcceleration()
{
	return gData.mAcceleration;
}

double getPlayerSpeed()
{
	return gData.mIsFocused ? gData.mFocusSpeed : gData.mNormalSpeed;
}

void resetPlayerState()
{
	gData.mPower = 0;
	gData.mLifeAmount = 2; 
	gData.mBombAmount = 3;
	gData.mContinueAmount = 5;
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

void setPlayerToFullPower()
{
	gData.mPower = 400;
	setPowerText(gData.mPower);
	setLifeText(gData.mLifeAmount); // TODO: move
	setBombText(gData.mBombAmount); // TODO: move
}

int getContinueAmount()
{
	return gData.mContinueAmount;
}

void reduceContinueAmount()
{
	gData.mLifeAmount = 2;
	gData.mBombAmount = 3;
	gData.mContinueAmount--;
}

void disablePlayerBossCollision()
{
	gData.mCanBeHitByEnemies = 0;
}
