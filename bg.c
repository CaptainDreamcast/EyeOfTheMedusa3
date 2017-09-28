#include "bg.h"

#include <tari/mugenanimationhandler.h>
#include <tari/mugendefreader.h>
#include <tari/mugenscriptparser.h>
#include <tari/animation.h>
#include <tari/wrapper.h>

typedef struct {
	Position mPosition;
	Position mOffset;
	MugenAnimation* mAnimation;
	int mAnimationID;

} BackgroundElement;

static struct {
	MugenSpriteFile* mSprites;

	double mSpeed;
	double mSize;

	TextureData mWhiteTexture;
	int mBlackAnimationID;

	Position mBasePosition;
	Vector mElements;
} gData;

static void loadBackgroundHandler(void* tData) {
	(void)tData;
	// TODO
	gData.mElements = new_vector();

	gData.mWhiteTexture = loadTexture("$/rd/effects/white.pkg");
	gData.mBlackAnimationID = playOneFrameAnimationLoop(makePosition(0, 0, 1), &gData.mWhiteTexture);
	setAnimationSize(gData.mBlackAnimationID, makePosition(640, 480, 1), makePosition(0, 0, 0));
	setAnimationColor(gData.mBlackAnimationID, 0, 0, 0);
}

static int isHeader(MugenDefScriptGroup* tGroup) {
	return !strcmp("Header", tGroup->mName);
}

static void handleHeader(MugenDefScriptGroup* tGroup) {
	(void)tGroup;
	gData.mSize = getMugenDefFloatOrDefaultAsGroup(tGroup, "size", 640*3);
	gData.mSpeed = getMugenDefFloatOrDefaultAsGroup(tGroup, "speed", 7);
	
	double opacity = getMugenDefFloatOrDefaultAsGroup(tGroup, "bgopacity", 0.2);
	setAnimationTransparency(gData.mBlackAnimationID, opacity);
}

static int isBackgroundElement(MugenDefScriptGroup* tGroup) {
	return !strcmp("BG Element", tGroup->mName);
}

static void handleBackgroundElement(MugenDefScriptGroup* tGroup) {
	BackgroundElement* e = allocMemory(sizeof(BackgroundElement));
	e->mOffset = getMugenDefVectorOrDefaultAsGroup(tGroup, "offset", makePosition(0, 0, 0));
	e->mPosition = e->mOffset;
	Vector3DI sprite = getMugenDefVectorIVariableAsGroup(tGroup, "sprite");
	e->mAnimation = createOneFrameMugenAnimationForSprite(sprite.x, sprite.y);
	e->mAnimationID = addMugenAnimation(e->mAnimation, gData.mSprites, makePosition(0, 0, 0));
	setMugenAnimationBasePosition(e->mAnimationID, &e->mPosition);

	vector_push_back_owned(&gData.mElements, e);
}

void setBackground(char* tPath, MugenSpriteFile* tSprites) {
	MugenDefScript script = loadMugenDefScript(tPath);
	gData.mSprites = tSprites;
	gData.mBasePosition = makePosition(0, 0, 2);

	resetMugenScriptParser();
	addMugenScriptParseFunction(isHeader, handleHeader);
	addMugenScriptParseFunction(isBackgroundElement, handleBackgroundElement);
	parseMugenScript(&script);

	unloadMugenDefScript(script);
}

static void updatePosition() {
	if (isWrapperPaused()) return;

	gData.mBasePosition.x += gData.mSpeed;
	if (gData.mBasePosition.x >= gData.mSize) {
		gData.mBasePosition.x -= gData.mSize;
	}
}

static void updateSingleBackgroundElement(void* tCaller, void* tData) {
	(void)tCaller;

	BackgroundElement* e = tData;
	e->mPosition = vecSub(e->mOffset, gData.mBasePosition);
}

static void updateBackgroundHandler(void* tData) {
	(void)tData;

	updatePosition();
	vector_map(&gData.mElements, updateSingleBackgroundElement, NULL);
}

ActorBlueprint BackgroundHandler = {
	.mLoad = loadBackgroundHandler,
	.mUpdate = updateBackgroundHandler,
};
