#pragma once

#include <tari/mugendefreader.h>
#include <tari/animation.h>
#include <tari/actorhandler.h>
#include <tari/mugenanimationreader.h>

extern ActorBlueprint EnemyHandler;

void loadEnemyTypesFromScript(MugenDefScript* tScript, MugenAnimations* tAnimations, MugenSpriteFile* tSprites);
void addEnemy(int tType, Position mPosition, Velocity mVelocity, Duration mShotFrequency, int mShotType);
int getEnemyAmount();
Position getClosestEnemyPosition(Position tPosition);
Position getRandomEnemyPosition();