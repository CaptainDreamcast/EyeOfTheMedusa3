#pragma once

#include <tari/actorhandler.h>
#include <tari/mugenanimationreader.h>

extern ActorBlueprint BossHandler;
void  loadBossFromDefinitionPath(char * tDefinitionPath, MugenAnimations* tAnimations, MugenSpriteFile* tSprites);
void activateBoss();

void fetchBossTimeVariable(char* tDst, void* tCaller);
int isBossActive();
Position getBossPosition();