#pragma once

#include <tari/mugendefreader.h>
#include <tari/actorhandler.h>
#include <tari/physics.h>

void getShotAngleTowardsPlayer(char* tOutput, void* tCaller);

void getCurrentSubShotIndex(char* tOutput, void* tCaller);
void addShot(int tID, int tCollisionList, Position tPosition);
void removeEnemyShots();
void evaluateBigBangFunction(char* tDst, void* tCaller);
void evaluateBounceFunction(char* tDst, void* tCaller);
void evaluateTextAidFunction(char* tDst, void* tCaller);
void evaluateAckermannFunction(char* tDst, void* tCaller);
void evaluateSwirlFunction(char* tDst, void* tCaller);
void evaluateBlamFunction(char* tDst, void* tCaller);
void evaluateTransienceFunction(char* tDst, void* tCaller);

int getFinalBossShotsDeflected();

extern ActorBlueprint ShotHandler;
