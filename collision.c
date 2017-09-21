#include "collision.h"

#include <tari/collisionhandler.h>

static struct {
	int mPlayerCollisionList;
	int mPlayerShotCollisionList;

	int mEnemyCollisionList;
	int mEnemyShotCollisionList;

} gData;

void loadCollisions()
{
	gData.mPlayerCollisionList = addCollisionListToHandler();
	gData.mPlayerShotCollisionList = addCollisionListToHandler();
	gData.mEnemyCollisionList = addCollisionListToHandler();
	gData.mEnemyShotCollisionList = addCollisionListToHandler();

	addCollisionHandlerCheck(gData.mPlayerCollisionList, gData.mEnemyShotCollisionList);
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
