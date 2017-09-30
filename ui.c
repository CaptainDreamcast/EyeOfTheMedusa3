#include "ui.h"

#include <tari/animation.h>
#include <tari/texthandler.h>
#include <tari/math.h>
#include <tari/mugenanimationhandler.h>

static struct {
	MugenSpriteFile mSprites;
	MugenAnimation* mAnimation;
	int mAnimationID;

	MugenAnimation* mStageDisplayAnimation;
	int mStageDisplayAnimationID;

	int mPowerTextID;
	int mLifeAmountTextID;
	int mBombAmountTextID;
} gData;

static void loadUserInterface(void* tData) {
	(void)tData;
	
	gData.mSprites = loadMugenSpriteFileWithoutPalette("assets/ui/UI.sff");
	gData.mAnimation = createOneFrameMugenAnimationForSprite(0, 0);

	gData.mAnimationID = addMugenAnimation(gData.mAnimation, &gData.mSprites, makePosition(0, 327, 40));

	gData.mLifeAmountTextID = addHandledText(makePosition(540, 350, 45), "0", 0, COLOR_WHITE, makePosition(30, 30, 30), makePosition(-5, -5, 0), makePosition(INF, INF, INF), INF);
	gData.mBombAmountTextID = addHandledText(makePosition(540, 387, 45), "0", 0, COLOR_WHITE, makePosition(30, 30, 30), makePosition(-5, -5, 0), makePosition(INF, INF, INF), INF);
	gData.mPowerTextID = addHandledText(makePosition(480, 420, 45), "0.00", 0, COLOR_WHITE, makePosition(30,30,30), makePosition(-5, -5, 0), makePosition(INF, INF, INF), INF);
}

ActorBlueprint UserInterface = {
	.mLoad = loadUserInterface,
};

void loadStageDisplay(MugenSpriteFile * tSprites)
{
	gData.mStageDisplayAnimation = createOneFrameMugenAnimationForSprite(9500, 0);
	gData.mStageDisplayAnimationID = addMugenAnimation(gData.mStageDisplayAnimation, tSprites, makePosition(15, 327+29, 45));
}

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

void setLifeText(int tLifeAmount)
{
	char text[100];
	sprintf(text, "%d", tLifeAmount);
	setHandledText(gData.mLifeAmountTextID, text);
}

void setBombText(int tBombAmount)
{
	char text[100];
	sprintf(text, "%d", tBombAmount);
	setHandledText(gData.mBombAmountTextID, text);
}
