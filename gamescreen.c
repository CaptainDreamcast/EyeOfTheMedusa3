#include "gamescreen.h"

#include <stdio.h>

#include <tari/input.h>
#include <tari/mugenanimationhandler.h>
#include <tari/collisionhandler.h>

#include "player.h"
#include "bg.h"
#include "ui.h"
#include "enemyhandler.h"
#include "level.h"
#include "shothandler.h"
#include "collision.h"
#include "boss.h"
#include "assignment.h"
#include "itemhandler.h"
#include "effecthandler.h"
#include "continuehandler.h"
#include "gameoptionhandler.h"
#include "titlescreen.h"

static void loadGameScreen() {
	instantiateActor(getMugenAnimationHandlerActorBlueprint());

	
	loadCollisions();
	instantiateActor(AssignmentHandler);
	instantiateActor(ContinueHandler);
	instantiateActor(GameOptionHandler);
	instantiateActor(EffectHandler);
	instantiateActor(ItemHandler);
	instantiateActor(EnemyHandler);
	instantiateActor(BackgroundHandler);
	instantiateActor(UserInterface);
	instantiateActor(Player);
	instantiateActor(ShotHandler);
	instantiateActor(BossHandler);

	instantiateActor(LevelHandler);

	// activateCollisionHandlerDebugMode();
}

static void updateGameScreen() {


	if (hasPressedAbortFlank()) {
		setNewScreen(&TitleScreen);
	}
}

Screen GameScreen = {
	.mLoad = loadGameScreen,
	.mUpdate = updateGameScreen,
};
