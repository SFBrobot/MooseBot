#pragma config(UART_Usage, UART1, uartVEXLCD, baudRate19200, IOPins, None, None)
#pragma config(UART_Usage, UART2, uartNotUsed, baudRate4800, IOPins, None, None)
#pragma config(I2C_Usage, I2C1, i2cSensors)
#pragma config(Sensor, dgtl1,  redLed,         sensorLEDtoVCC)
#pragma config(Sensor, dgtl2,  yellowLed,      sensorLEDtoVCC)
#pragma config(Sensor, dgtl3,  greenLed,       sensorLEDtoVCC)
#pragma config(Sensor, I2C_1,  flyEnc,         sensorQuadEncoderOnI2CPort,    , AutoAssign )
#pragma config(Motor,  port1,           intake,        tmotorVex393HighSpeed_HBridge, openLoop)
#pragma config(Motor,  port2,           brWheel,       tmotorVex393HighSpeed_MC29, openLoop, reversed, driveRight)
#pragma config(Motor,  port3,           mrWheel,       tmotorVex393HighSpeed_MC29, openLoop, reversed, driveRight)
#pragma config(Motor,  port4,           frWheel,       tmotorVex393HighSpeed_MC29, openLoop, reversed, driveRight)
#pragma config(Motor,  port5,           lFly,          tmotorVex393TurboSpeed_MC29, openLoop, reversed, encoderPort, I2C_1)
#pragma config(Motor,  port6,           rFly,          tmotorVex393TurboSpeed_MC29, openLoop)
#pragma config(Motor,  port7,           flWheel,       tmotorVex393HighSpeed_MC29, openLoop, driveLeft)
#pragma config(Motor,  port8,           mlWheel,       tmotorVex393HighSpeed_MC29, openLoop, driveLeft)
#pragma config(Motor,  port9,           blWheel,       tmotorVex393HighSpeed_MC29, openLoop, driveLeft)
#pragma config(Motor,  port10,          lift,          tmotorVex393HighSpeed_HBridge, openLoop, driveLeft)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

#define RKCOMP_LCD //Enable LCD stuff in rkCompetition
//#define RKCOMP_DEBUG //Comment to disable debug mode

#define RKCOMP_DEBUG_MENU_COND vexRT[Btn8L]
#define RKCOMP_DEBUG_DISABLE_COND vexRT[Btn8U]
#define RKCOMP_DEBUG_AUTON_COND vexRT[Btn8R]
#define RKCOMP_DEBUG_DRIVER_COND vexRT[Btn8D]
#define RKCOMP_DEBUG_RESTART_COND vexRT[Btn6U]

#define FLY_LR_BTN vexRT[Btn7U]
#define FLY_MR_BTN vexRT[Btn7L]
#define FLY_SR_BTN vexRT[Btn7D]
#define FLY_OFF_BTN vexRT[Btn7R]

#define FLY_BTNS (FLY_LR_BTN || FLY_MR_BTN || FLY_SR_BTN || FLY_OFF_BTN)

#define PWR_BTN_DOWN ((nLCDButtons & kButtonLeft) || vexRT[Btn5DXmtr2])
#define PWR_BTN_UP ((nLCDButtons & kButtonRight) || vexRT[Btn5UXmtr2])

#define DRIVE_TANK_BTN false //vexRT[Btn8L] Alexi doesn't like tank, he kept pressing the button and saying the right drive was stalled
#define DRIVE_FLIP_BTN vexRT[Btn8R]

#define INTAKE_FEEDIN_BTN vexRT[Btn5U]
#define INTAKE_FEEDOUT_BTN vexRT[Btn5D]

#define LIFT_RAISE_BTN vexRT[Btn6U]
#define LIFT_LOWER_BTN vexRT[Btn6D]

#include "rkUtil/lib.h"

#include "rkLogic/dlatch.h"

#include "rkLcd/lib.h"

#include "rkControl/base.h"
#include "rkControl/diff.h"
#include "rkControl/kalman.h"
#include "rkControl/rollAvg.h"
#include "rkControl/tbh.h"
#include "rkControl/tbhController.h"

#include "rkCompetition/lib.h"

#define USE_PRELOAD_AUTON //Comment to start the intake immediately in auton

int flyDir;

float flyPwr[4] = {0, 600, 700, 835};

const string flyPwrNames[4] = {
	"Off",
	"Short Power",
	"Mid Power",
	"Long Power",
};

const float autonFlyPwr = 830,
	velThresh = 75,
	accelThresh = 150;

ADiff flyDiff, fly2Diff;
//KFlt fly2Flt;
RAFlt flyDispFlt, fly2Flt;
Tbh flyTbh;
TbhController flyCtl;

task lcd() {
	const float flyPwrIncrement = 5;
	const word battThresh = 7800;
	const long pwrBtnsDelayInterval = 250,
		pwrBtnsRepeatInterval = 100,
		dispPwrTimeout = 1000;

	bool dispPwr = false,
		doUseLRPwr = true,
		flash = false,
		flashLeds,
		forceBattWarning = true,
		pwrBtnsDown,
		pwrBtnsDelayed,
		pwrBtnsRepeating;

	float pwrBtns;
	long time = nSysTime, flashTs = time, dispPwrTs = time, pwrBtnTs = time;
	string str;

	DLatch dismissWarningLatch, pwrBtnLatch;

	while (true) {
		time = nSysTime;

		if (nImmediateBatteryLevel < battThresh) {
			if ((time - flashTs) >= 500) {
				flash = !flash;
				flashTs = time;
			}

			flashLeds = true;
		}
		else {
			SensorValue[redLed] =
				SensorValue[yellowLed] =
				SensorValue[greenLed] = 0;
		}

		clearLCD();

		if (nLCDButtons & kButtonCenter) {
			bLCDBacklight = true;

			displayLCDCenteredString(0, "Battery:");

			sprintBatt(str);

			displayLCDString(1, 0, str);
		}
		else if (nImmediateBatteryLevel < battThresh && (rkBotDisabled || rkAutonMode || forceBattWarning)) {
			bLCDBacklight = flash;

			if (flash) displayLCDCenteredString(0, "BATTERY WARNING");

			sprintBatt(str);
			displayLCDString(1, 0, str);

			if (fallingEdge(&dismissWarningLatch, nLCDButtons || (abs(vexRT[AccelY]) > 63))) forceBattWarning = false;
		}
		else if (rkBotDisabled) {
			bLCDBacklight = false;

			displayLCDCenteredString(0, "Moosebot Mk. III");
			displayLCDCenteredString(1, "4800Buckets");
		}
		else if (rkAutonMode) {
			bLCDBacklight = true;

			displayLCDCenteredString(0, "BUCKETS MODE");
			displayLCDCenteredString(1, "ENGAGED");
		}
		else { //User op mode
			bLCDBacklight = true;

			pwrBtnsDown = PWR_BTN_DOWN ^ PWR_BTN_UP;
			risingEdge(&pwrBtnLatch, pwrBtnsDown);

			if ((pwrBtnsDown || FLY_BTNS) && flyDir) dispPwrTs = time;

			if (pwrBtnsDown) {
				dispPwrTs = time;

				if (pwrBtnLatch.out || (time - pwrBtnTs >= (pwrBtnsRepeating ? pwrBtnsRepeatInterval : pwrBtnsDelayInterval))) {
					pwrBtnTs = time;

					if (pwrBtnsDelayed) {
					  if (!pwrBtnsRepeating) pwrBtnsRepeating = true;
					}
					else pwrBtnsDelayed = true;

					pwrBtns = twoWay(PWR_BTN_DOWN, -flyPwrIncrement, PWR_BTN_UP, flyPwrIncrement);

					if (flyDir != 0)
						flyPwr[flyDir] += pwrBtns;
				}
			}
			else pwrBtnsDelayed = pwrBtnsRepeating = false;

			if (time - dispPwrTs <= dispPwrTimeout && flyDir) {
				displayLCDCenteredString(0, flyPwrNames[flyDir]);
				displayLCDNumber(1, 0, flyPwr[flyDir]);
			}
			else if (flyTbh.doRun) {
				sprintf(str, "% 07.2f  % 07.2f",
					fmaxf(-999.99, fminf(999.99, flyDispFlt.out)),
					fmaxf(-999.99, fminf(999.99, flyTbh.err)));
				displayLCDString(0, 0, str);

				sprintf(str, "% 07.2f  % 07.2f",
					fmaxf(-999.99, fminf(999.99, fly2Flt.out)),
					fmaxf(-999.99, fminf(999.99, flyTbh.out)));
				displayLCDString(1, 0, str);

				flashLeds = false;

				SensorValue[redLed] =
					SensorValue[yellowLed] =
					SensorValue[greenLed] = 0;

				if (isTbhInThresh(&flyTbh, velThresh)) {
					if (isTbhDerivInThresh(&flyTbh, accelThresh)) SensorValue[greenLed] = 1;
					else  SensorValue[yellowLed] = 1;
				}
				else SensorValue[redLed] = 1;
			}
			else {
				sprintf(str, "%-8s%8s", "Speed", "Error");
				displayLCDString(0, 0, str);

				sprintf(str, "%-8s%8s", "Accel", "Out");
				displayLCDString(1, 0, str);
			}
		}

		if (flashLeds) {
			SensorValue[redLed] =
				SensorValue[yellowLed] =
				SensorValue[greenLed] = flash;
		}

		wait1Msec(20);
	}
}

void startFlyTbh(bool useCtl) {
	resetTbh(&flyTbh, 0);

	resetDiff(&flyDiff, 0);
	resetDiff(&fly2Diff, 0);

	resetRAFlt(&fly2Flt, 0);

	if (useCtl) updateTbhController(&flyCtl, 0);
	else setTbhDoRun(&flyTbh, true);

	SensorValue[flyEnc] = 0;
}

void stopCtls() {
	setTbhDoRun(&flyTbh, false);

	stopCtlLoop();
}

#define FLY_DISP_FLT_LEN 10
#define FLY2_FLT_LEN 5
float flyDispFltBuf[FLY_DISP_FLT_LEN],
  flyFltBuf[FLY2_FLT_LEN];

void init() {
  ctlLoopInterval = 50;

  initTbh(&flyTbh, 7, .4, .2, 127, true);

  initTbhController(&flyCtl, &flyTbh, false);

  initRAFlt(&flyDispFlt, flyDispFltBuf, FLY_DISP_FLT_LEN);
  initRAFlt(&fly2Flt, flyFltBuf, FLY2_FLT_LEN);
}

void updateCtl(float dt) {
	updateDiff(&flyDiff, -SensorValue[flyEnc], dt);
	updateDiff(&fly2Diff, flyDiff.out, dt);

	updateRAFlt(&flyDispFlt, flyDiff.out);
	updateRAFlt(&fly2Flt, fly2Diff.out);

	if (flyTbh.doUpdate)
		motor[lFly] =
			motor[rFly] =
			updateTbh(&flyTbh, flyDiff.out, fly2Flt.out, dt);
}

task auton() {
	const float recoilThresh = 40;

	startFlyTbh(false);
	startCtlLoop();

	setTbh(&flyTbh, autonFlyPwr);

#ifdef USE_PRELOAD_AUTON
	block(!(isTbhInThresh(&flyTbh, velThresh) &&
		isTbhDerivInThresh(&flyTbh, accelThresh)));

	while (true) {
		motor[intake] = motor[lift] = 80;

		block(isTbhInThresh(&flyTbh, recoilThresh) &&
			isTbhDerivInThresh(&flyTbh, accelThresh));

		motor[intake] = motor[lift] = 0;

		block(!(isTbhInThresh(&flyTbh, velThresh) &&
			isTbhDerivInThresh(&flyTbh, accelThresh)));

		wait1Msec(20);
	}
#else
	motor[intake] = motor[lift] = 127;
#endif
}

void endAuton() {
	stopCtls();
}

task userOp() {
	startFlyTbh(true);
	startCtlLoop();

	const float cutFac = 3;

	DLatch cutLatch, flipLatch, tankLatch, flyLRLatch, flyMRLatch, flySRLatch, flyOffLatch;

	word driveA, driveB;

#define driveX driveA
#define driveY driveB
#define driveL driveA
#define driveR driveB

	resetDLatch(&cutLatch, 0);
	resetDLatch(&flipLatch, 0);
	resetDLatch(&tankLatch, 0);
	resetDLatch(&flyLRLatch, 0);
	resetDLatch(&flyMRLatch, 0);
	resetDLatch(&flySRLatch, 0);
	resetDLatch(&flyOffLatch, 0);

	while (true) {
		risingBistable(&flipLatch, DRIVE_FLIP_BTN);

		//TODO: Make this better
		if (risingBistable(&tankLatch, DRIVE_TANK_BTN)) {
			driveL = vexRT[ChLY];
			driveR = vexRT[ChRY];

			if (flipLatch.out) {
				driveL ^= driveR;
				driveR ^= driveL;
				driveL ^= driveR;

				driveL *= -1;
				driveR *= -1;
			}
		}
		else {
			driveX = vexRT[ChRX];
			driveY = vexRT[ChLY];

			if (flipLatch.out) driveY *= -1;
		}

		if (tankLatch.out) {
			tank4(driveL, driveR, flWheel, blWheel, frWheel, brWheel);
			tank2(driveL, driveR, mlWheel, mrWheel);
		}
		else {
			arcade4(driveX, driveY, flWheel, blWheel, frWheel, brWheel);
			arcade2(driveX, driveY, mlWheel, mrWheel);
		}

		if (risingEdge(&flyLRLatch, FLY_LR_BTN))
			flyDir = 3;

		if (risingEdge(&flyMRLatch, FLY_MR_BTN))
			flyDir = 2;

		if (risingEdge(&flySRLatch, FLY_SR_BTN))
			flyDir = 1;

		if (risingEdge(&flyOffLatch, FLY_OFF_BTN))
			flyDir = 0;

		updateTbhController(&flyCtl, flyPwr[flyDir]);

		motor[intake] =
			joyDigi2(INTAKE_FEEDIN_BTN, 127, INTAKE_FEEDOUT_BTN, -127);

		motor[lift] =
			joyDigi2(LIFT_RAISE_BTN, 127, LIFT_LOWER_BTN, -127);

		wait1Msec(25);
	}

#undef driveX
#undef driveY
#undef driveL
#undef driveR
}

void endUserOp() {
	stopCtls();
}
