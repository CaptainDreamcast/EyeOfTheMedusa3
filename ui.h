#pragma once

#include <tari/actorhandler.h>
#include <tari/mugenspritefilereader.h>

extern ActorBlueprint UserInterface;

void loadStageDisplay(MugenSpriteFile* mSprites);
void setPowerText(int tPower);
void setLifeText(int tLifeAmount);
void setBombText(int tBombAmount);