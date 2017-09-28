#pragma once

#include <tari/actorhandler.h>

extern ActorBlueprint LevelHandler;

void setLevelToStart();
void goToNextLevel();
void fetchStagePartTime(char* tDst, void* tCaller);
void advanceStagePart();