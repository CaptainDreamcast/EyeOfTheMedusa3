#include "effecthandler.h"

#include <tari/mugenanimationhandler.h>

static struct {
	MugenSpriteFile mSprites;
	MugenAnimations mAnimations;

} gData;

static void loadEffectHandler(void* tData) {
	(void)tData;
	gData.mSprites = loadMugenSpriteFileWithoutPalette("assets/effects/EFFECTS.sff");
	gData.mAnimations = loadMugenAnimationFile("assets/effects/EFFECTS.air");
}

ActorBlueprint EffectHandler = {
	.mLoad = loadEffectHandler,
};

void addExplosionEffect(Position tPosition)
{
	tPosition = vecAdd(tPosition, makePosition(0, 0, 20));
	MugenAnimation* animation = getMugenAnimation(&gData.mAnimations, 2);
	int id = addMugenAnimation(animation, &gData.mSprites, tPosition);
	setMugenAnimationNoLoop(id);
}
