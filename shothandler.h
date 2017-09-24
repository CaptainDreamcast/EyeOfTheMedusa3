#pragma once

#include <tari/mugendefreader.h>
#include <tari/actorhandler.h>
#include <tari/physics.h>

void getShotAngleTowardsPlayer(char* tOutput, void* tCaller);

void getCurrentSubShotIndex(char* tOutput, void* tCaller);
void addShot(int tID, int tCollisionList, Position tPosition);
void removeEnemyShots();

extern ActorBlueprint ShotHandler;
