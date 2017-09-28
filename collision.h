#pragma once

typedef enum {
	ITEM_TYPE_SMALL_POWER,
	ITEM_TYPE_LIFE,
	ITEM_TYPE_BOMB,

} ItemType;

typedef struct {
	int mCollisionList;

	int mIsItem;
	ItemType mItemType;
} CollisionData;

void loadCollisions();

int getPlayerCollisionList();
int getPlayerShotCollisionList();
int getEnemyCollisionList();
int getEnemyShotCollisionList();
int getItemCollisionList();
int getPlayerItemCollisionList();