#include "assignment.h"

#include <stdio.h>
#include <assert.h>

#include <tari/mugenassignmentevaluator.h>
#include <tari/math.h>

#include "boss.h"
#include "shothandler.h"
#include "enemyhandler.h"
#include "player.h"


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

static void fetchPI(char* tDst, void* tCaller) {
	(void)tCaller;
	sprintf(tDst, "3.14159");
}

static void fetchInfinity(char* tDst, void* tCaller) {
	(void)tCaller;
	sprintf(tDst, "%d", INF);
}

void loadGameAssignments()
{
	resetMugenAssignmentContext();
	addMugenAssignmentVariable("pi", fetchPI);
	addMugenAssignmentVariable("bosstime", fetchBossTimeVariable);
	addMugenAssignmentVariable("angletowardsplayer", getShotAngleTowardsPlayer);
	addMugenAssignmentVariable("inf", fetchInfinity);
	addMugenAssignmentVariable("curenemy", getCurrentEnemyIndex);
	addMugenAssignmentVariable("cursubshot", getCurrentSubShotIndex);
	addMugenAssignmentVariable("localdeathcount", getLocalDeathCountVariable);
	addMugenAssignmentVariable("localbombcount", getLocalBombCountVariable);
	addMugenAssignmentArray("randfrom", fetchRandFromValue);
	
}
