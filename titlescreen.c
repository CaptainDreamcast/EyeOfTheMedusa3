#include "titlescreen.h"

#include <tari/mugenanimationhandler.h>
#include <tari/input.h>
#include <tari/screeneffect.h>

#include "gamescreen.h"
#include "bg.h"
#include "level.h"
#include "player.h"

static struct {
	MugenSpriteFile mSprites;
	MugenAnimation* mTitleAnimation;
	int mTitleAnimationID;

} gData;

static void loadTitleScreen() {
	instantiateActor(getMugenAnimationHandlerActorBlueprint());
	
	gData.mSprites = loadMugenSpriteFileWithoutPalette("assets/title/TITLE.sff");
	 
	gData.mTitleAnimation = createOneFrameMugenAnimationForSprite(1, 0);
	gData.mTitleAnimationID = addMugenAnimation(gData.mTitleAnimation, &gData.mSprites, makePosition(0, 0, 1));

	instantiateActor(BackgroundHandler);
	setBackground("assets/title/BG.def", &gData.mSprites);

	addFadeIn(30, NULL, NULL);
}

static void goToGame(void* tCaller) {
	(void)tCaller;
	setLevelToStart();
	resetPlayerState();
	setNewScreen(&GameScreen);
}

static void updateTitleScreen() {
	if (hasPressedStartFlank()) {
		addFadeOut(30, goToGame, NULL);
	}

	if (hasPressedAbortFlank()) {
		abortScreenHandling();
	}
}

Screen TitleScreen = {
	.mLoad = loadTitleScreen,
	.mUpdate = updateTitleScreen,
};