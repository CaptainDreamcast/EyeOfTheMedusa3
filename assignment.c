#include "assignment.h"

#include <stdio.h>
#include <assert.h>

#include <tari/mugenassignmentevaluator.h>
#include <tari/math.h>

#include "boss.h"
#include "shothandler.h"
#include "enemyhandler.h"
#include "player.h"
#include "level.h"

static struct {
	int mRand1;
	int mRand2;
} gData;

static void loadGameAssignments();

static void loadAssignmentHandler(void* tData) {
	(void)tData;
	gData.mRand1 = 0;
	gData.mRand2 = 0;
	loadGameAssignments();
}

static void updateAssignmentHandler(void* tData) {
	(void)tData;
	gData.mRand1 = randfromInteger(0, 10000);
	gData.mRand2 = randfromInteger(0, 10000);
}

ActorBlueprint AssignmentHandler = {
	.mLoad = loadAssignmentHandler,
	.mUpdate = updateAssignmentHandler,
};

static void fetchRandFromValue(char* tDst, void* tCaller, char* tIndex) {
	(void)tCaller;
	double a, b;
	char comma[20];
	int items = sscanf(tIndex, "%lf %s %lf", &a, comma, &b);
	assert(items == 3);
	assert(!strcmp(",", comma));
	
	double val = randfrom(a, b);
	sprintf(tDst, "%f", val);
}

static void fetchRandFromIntegerValue(char* tDst, void* tCaller, char* tIndex) {
	(void)tCaller;
	int a, b;
	char comma[20];
	int items = sscanf(tIndex, "%d %s %d", &a, comma, &b);
	assert(items == 3);
	assert(!strcmp(",", comma));

	int val = randfromInteger(a, b);
	sprintf(tDst, "%d", val);
}


static void fetchIdentity(char* tDst, void* tCaller, char* tIndex) {
	(void)tCaller;
	sprintf(tDst, "%s", tIndex);
}

static void fetchRand1(char* tDst, void* tCaller) {
	(void)tCaller;
	sprintf(tDst, "%d", gData.mRand1);
}

static void fetchRand2(char* tDst, void* tCaller) {
	(void)tCaller;
	sprintf(tDst, "%d", gData.mRand2);
}

static void fetchPI(char* tDst, void* tCaller) {
	(void)tCaller;
	sprintf(tDst, "3.14159");
}

static void fetchInfinity(char* tDst, void* tCaller) {
	(void)tCaller;
	sprintf(tDst, "%d", INF);
}

static void loadGameAssignments()
{
	resetMugenAssignmentContext();
	
	addMugenAssignmentVariable("rand1", fetchRand1);
	addMugenAssignmentVariable("rand2", fetchRand2);
	addMugenAssignmentVariable("pi", fetchPI);
	addMugenAssignmentVariable("bosstime", fetchBossTimeVariable);
	addMugenAssignmentVariable("angletowardsplayer", getShotAngleTowardsPlayer);
	addMugenAssignmentVariable("inf", fetchInfinity);
	addMugenAssignmentVariable("curenemy", getCurrentEnemyIndex);
	addMugenAssignmentVariable("cursubshot", getCurrentSubShotIndex);
	addMugenAssignmentVariable("localdeathcount", getLocalDeathCountVariable);
	addMugenAssignmentVariable("localbombcount", getLocalBombCountVariable);
	addMugenAssignmentVariable("stageparttime", fetchStagePartTime);
	
	addMugenAssignmentVariable("bigbang", evaluateBigBangFunction);
	addMugenAssignmentVariable("bounce", evaluateBounceFunction); 
	addMugenAssignmentVariable("ackermann", evaluateAckermannFunction);
	addMugenAssignmentVariable("groovy", evaluateSwirlFunction);
	addMugenAssignmentVariable("blam", evaluateBlamFunction);
	addMugenAssignmentVariable("transience", evaluateTransienceFunction);


	addMugenAssignmentVariable("textaid", evaluateTextAidFunction);

	addMugenAssignmentArray("randfrom", fetchRandFromValue);
	addMugenAssignmentArray("randfrominteger", fetchRandFromIntegerValue);
	addMugenAssignmentArray("identity", fetchIdentity);
	
}
