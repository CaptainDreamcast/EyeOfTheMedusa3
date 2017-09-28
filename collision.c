#include "collision.h"

#include <tari/collisionhandler.h>

static struct {
	int mPlayerCollisionList;
	int mPlayerShotCollisionList;

	int mEnemyCollisionList;
	int mEnemyShotCollisionList;

	int mItemCollisionList;
	int mPlayerItemCollisionList;

} gData;

void loadCollisions()
{
	gData.mPlayerCollisionList = addCollisionListToHandler();
	gData.mPlayerShotCollisionList = addCollisionListToHandler();
	gData.mEnemyCollisionList = addCollisionListToHandler();
	gData.mEnemyShotCollisionList = addCollisionListToHandler();
	gData.mItemCollisionList = addCollisionListToHandler();
	gData.mPlayerItemCollisionList = addCollisionListToHandler();

	addCollisionHandlerCheck(gData.mPlayerCollisionList, gData.mEnemyShotCollisionList);
	addCollisionHandlerCheck(gData.mPlayerCollisionList, gData.mEnemyCollisionList);
	addCollisionHandlerCheck(gData.mPlayerItemCollisionList, gData.mItemCollisionList);
	addCollisionHandlerCheck(gData.mEnemyCollisionList, gData.mPlayerShotCollisionList);
}

int getPlayerCollisionList()
{
	return gData.mPlayerCollisionList;
}

int getPlayerShotCollisionList()
{
	return gData.mPlayerShotCollisionList;
}

int getEnemyCollisionList()
{
	return gData.mEnemyCollisionList;
}

int getEnemyShotCollisionList()
{
	return gData.mEnemyShotCollisionList;
}

int getItemCollisionList()
{
	return gData.mItemCollisionList;
}

int getPlayerItemCollisionList()
{
	return gData.mPlayerItemCollisionList;
}
