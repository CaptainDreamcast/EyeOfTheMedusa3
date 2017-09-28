#pragma once

#include <tari/actorhandler.h>
#include <tari/geometry.h>

extern ActorBlueprint Player;

Position getPlayerPosition();

void resetPlayerState();
void resetLocalPlayerCounts();
void getLocalDeathCountVariable(char* tDst, void* tCaller);
void getLocalBombCountVariable(char* tDst, void* tCaller);
void setPlayerToFullPower();

int getContinueAmount();
void reduceContinueAmount();

void disablePlayerBossCollision();