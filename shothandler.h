#pragma once

#include <tari/mugendefreader.h>
#include <tari/actorhandler.h>
#include <tari/physics.h>

void addShot(int tID, int tCollisionList, Position tPosition);
void removeAllShots();

extern ActorBlueprint ShotHandler;
