#include "rkUtil/lib.h"

#define RKCOMP_LCD

#include "rkCompetition/lib.h"

#include "rkControl/base.h"
#include "rkControl/diff.h"
#include "rkControl/tbh.h"
#include "rkControl/tbhController.h"

#include "rkLogic/dlatch.h"

ADiff flyDiff, fly2Diff;
Tbh flyTbh;
TbhController flyCtl;

float flyLRPwr = 800,
	flySRPwr = 500;

void startFlyTbh(bool useCtl) {
	resetDiff(&flyDiff, SensorValue[flyEnc]);
	resetDiff(&fly2Diff, flyDiff.out);

	if (useCtl) updateTbhController(flyCtl, 0);
	else setTbhDoRun(&flyTbh, true);

	startCtlLoop();
}

void stopAllCtls() {
	setTbhDoRun(&flyTbh, false);

	startCtlLoop();
}

task lcd() { }

void init() {
	initTbh(&flyTbh, 5, .1, .1, 127, true);
	initTbhController(&flyCtl, &flyTbh, false);

	ctlLoopInterval = 50;
}

void updateCtl(float dt) {
	if (flyTbh.doUpdate) {
		updateDiff(&flyDiff, SensorValue[flyEnc]);
		updateDiff(&fly2Diff, flyDiff.out);

		motor[tlFly] = motor[trFly] =
			motor[blFly] = motor[brFly] =
			updateTbh(&flyTbh, flyDiff.out, fly2Diff.out, dt);
	}
}

task auton() { }

void endAuton() { }

task userOp() {
	DLatch flyLRLatch, flySRLatch;
	word flyDir;

	resetDLatch(&flyLRLatch, 0);
	resetDLatch(&flySRLatch, 0);

	startFlyTbh();

	while (true) {
		if (risingEdge(&flyLRLatch, vexRT[Btn5U]) !=
			risingEdge(&flySRLatch, vexRT[Btn5D])) {

			if (flyLRLatch.out) flyDir = flyDir == 1 ? 0 : 1;
			else flyDir = flyDir == 2 ? 0 : 2;
		}

		updateTbhController(&flyCtl, flyPwr);
	}
}

void endUserOp() {
	stopAllCtls();
}
