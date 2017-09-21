#pragma once

typedef struct {
	int mCollisionList;
} CollisionData;

void loadCollisions();

int getPlayerCollisionList();
int getPlayerShotCollisionList();
int getEnemyCollisionList();
int getEnemyShotCollisionList();