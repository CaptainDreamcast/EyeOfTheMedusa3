#pragma once

#include <tari/actorhandler.h>
#include <tari/geometry.h>

extern ActorBlueprint ItemHandler;

void addSmallPowerItems(Position tPosition, int tAmount);
void addLifeItems(Position tPosition, int tAmount);
void addBombItems(Position tPosition, int tAmount);