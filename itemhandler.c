#include "itemhandler.h"

#include <tari/mugenspritefilereader.h>
#include <tari/mugenanimationhandler.h>
#include <tari/physicshandler.h>
#include <tari/math.h>

#include "collision.h"

typedef enum { 
	ITEM_TYPE_SMALL_POWER,
	ITEM_TYPE_BIG_POWER,
}ItemType;

typedef struct {
	ItemType mType;

	int mAnimationID;
	int mPhysicsID;
	int mListID;

	CollisionData mCollisionData;
} Item;

static struct {
	MugenSpriteFile mSprites;
	MugenAnimations mAnimations;

	List mItems;
} gData;

static void loadItemHandler(void* tData) {
	(void)tData;

	gData.mItems = new_list();
	gData.mSprites = loadMugenSpriteFileWithoutPalette("assets/items/ITEMS.sff");
	gData.mAnimations = loadMugenAnimationFile("assets/items/ITEMS.air");
}

static void unloadItem(Item* e) {
	removeMugenAnimation(e->mAnimationID);
	removeFromPhysicsHandler(e->mPhysicsID);
}

static int updateSingleItem(void* tCaller, void* tData) {
	(void)tData;
	Item* e = tData;
	Position p = *getHandledPhysicsPositionReference(e->mPhysicsID);
	
	if (p.x < -100) {
		unloadItem(e);
		return 1;
	}
	
	return 0;
}



static void updateItemHandler(void* tData) {
	(void)tData;
	list_remove_predicate(&gData.mItems, updateSingleItem, NULL);
}

ActorBlueprint ItemHandler = {
	.mLoad = loadItemHandler,
	.mUpdate = updateItemHandler,
};

static void powerItemHitCB(void* tCaller, void* tCollisionData) {
	(void)tCollisionData;
	printf("hit\n");
	Item* e = tCaller;
	unloadItem(e);
	list_remove(&gData.mItems, e->mListID);
}

static void addSingleSmallPowerItem(Position tPosition) {
	Item* e = allocMemory(sizeof(Item));

	e->mPhysicsID = addToPhysicsHandler(tPosition);
	addAccelerationToHandledPhysics(e->mPhysicsID, makePosition(-2, 0, 0));
	MugenAnimation* animation = getMugenAnimation(&gData.mAnimations, 1);
	e->mAnimationID = addMugenAnimation(animation, &gData.mSprites, makePosition(0,0,30));
	setMugenAnimationBasePosition(e->mAnimationID, getHandledPhysicsPositionReference(e->mPhysicsID));

	e->mCollisionData.mCollisionList = getPowerItemCollisionList();
	setMugenAnimationCollisionActive(e->mAnimationID, getPowerItemCollisionList(), powerItemHitCB, e, &e->mCollisionData);

	e->mListID = list_push_back_owned(&gData.mItems, e);
}

void addSmallPowerItems(Position tPosition, int tAmount)
{
	int i = 0;
	for (i = 0; i < tAmount; i++) {
		Position p = vecAdd(tPosition, makePosition(randfrom(-10, 10), randfrom(-10, 10), 0));
		addSingleSmallPowerItem(p);
	}
}


