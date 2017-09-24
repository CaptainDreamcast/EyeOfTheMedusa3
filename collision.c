#include "collision.h"

#include <tari/collisionhandler.h>

static struct {
	int mPlayerCollisionList;
	int mPlayerShotCollisionList;

	int mEnemyCollisionList;
	int mEnemyShotCollisionList;

	int mPowerItemCollisionList;
	int mPlayerItemCollisionList;

} gData;

void loadCollisions()
{
	gData.mPlayerCollisionList = addCollisionListToHandler();
	gData.mPlayerShotCollisionList = addCollisionListToHandler();
	gData.mEnemyCollisionList = addCollisionListToHandler();
	gData.mEnemyShotCollisionList = addCollisionListToHandler();
	gData.mPowerItemCollisionList = addCollisionListToHandler();
	gData.mPlayerItemCollisionList = addCollisionListToHandler();

	addCollisionHandlerCheck(gData.mPlayerCollisionList, gData.mEnemyShotCollisionList);
	addCollisionHandlerCheck(gData.mPlayerItemCollisionList, gData.mPowerItemCollisionList);
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

int getPowerItemCollisionList()
{
	return gData.mPowerItemCollisionList;
}

int getPlayerItemCollisionList()
{
	return gData.mPlayerItemCollisionList;
}
