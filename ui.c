#include "ui.h"

#include <tari/animation.h>

static struct {
	TextureData mTexture;
	int mAnimationID;
} gData;

static void loadUserInterface(void* tData) {
	(void)tData;
	
	gData.mTexture = loadTexture("assets/ui/BG.pkg");
	gData.mAnimationID = playOneFrameAnimationLoop(makePosition(0, 327, 80), &gData.mTexture);
}

ActorBlueprint UserInterface = {
	.mLoad = loadUserInterface,
};