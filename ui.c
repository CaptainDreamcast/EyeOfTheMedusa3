#include "ui.h"

#include <tari/animation.h>
#include <tari/texthandler.h>
#include <tari/math.h>

static struct {
	TextureData mTexture;
	int mAnimationID;

	int mPowerTextID;
} gData;

static void loadUserInterface(void* tData) {
	(void)tData;
	
	gData.mTexture = loadTexture("assets/ui/BG.pkg");
	gData.mAnimationID = playOneFrameAnimationLoop(makePosition(0, 327, 80), &gData.mTexture);

	gData.mPowerTextID = addHandledText(makePosition(480, 420, 81), "0.00", 0, COLOR_WHITE, makePosition(30,30,30), makePosition(-5, -5, 0), makePosition(INF, INF, INF), INF);
}

ActorBlueprint UserInterface = {
	.mLoad = loadUserInterface,
};

void setPowerText(int tPower)
{
	int decimal = tPower / 100;
	int fraction = tPower % 100;

	char text[100];
	if (fraction < 10) {
		sprintf(text, "%d.0%d", decimal, fraction);
	}
	else {
		sprintf(text, "%d.%d", decimal, fraction);
	}
	setHandledText(gData.mPowerTextID, text);
}
