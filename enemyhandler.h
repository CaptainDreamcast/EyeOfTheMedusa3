#pragma once

#include <tari/mugendefreader.h>
#include <tari/animation.h>
#include <tari/actorhandler.h>
#include <tari/mugenanimationreader.h>
#include <tari/mugenassignment.h>

typedef enum {
	ENEMY_MOVEMENT_TYPE_WAIT,
	ENEMY_MOVEMENT_TYPE_RUSH,
} EnemyMovementType;

typedef struct {
	int mType;

	MugenAssignment* mAmount;
	MugenAssignment* mStartPosition;
	MugenAssignment* mWaitPosition;
	MugenAssignment* mFinalPosition;
	EnemyMovementType mMovementType;
	MugenAssignment* mSpeed;
	MugenAssignment* mSmallPowerAmount;
	MugenAssignment* mLifeDropAmount;
	MugenAssignment* mBombDropAmount;

	MugenAssignment* mShotFrequency;
	MugenAssignment* mShotType;

	MugenAssignment* mHealth;
	MugenAssignment* mWaitDuration;
} StageEnemy;

extern ActorBlueprint EnemyHandler;

void loadEnemyTypesFromScript(MugenDefScript* tScript, MugenAnimations* tAnimations, MugenSpriteFile* tSprites);
void getCurrentEnemyIndex(char* tDst, void* tCaller);
void addEnemy(StageEnemy* tEnemy);
int getEnemyAmount();
Position getClosestEnemyPosition(Position tPosition);
Position getRandomEnemyPosition();