/***************************************************************************

    file                 : wheel.cpp
    created              : Sun Mar 19 00:09:06 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: wheel.cpp,v 1.25 2005/11/18 00:34:33 olethros Exp $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include "sim.h"
#include "math/misc.h"

static char *WheelSect[4] = {SECT_FRNTRGTWHEEL, SECT_FRNTLFTWHEEL, SECT_REARRGTWHEEL, SECT_REARLFTWHEEL};
static char *SuspSect[4] = {SECT_FRNTRGTSUSP, SECT_FRNTLFTSUSP, SECT_REARRGTSUSP, SECT_REARLFTSUSP};
static char *BrkSect[4] = {SECT_FRNTRGTBRAKE, SECT_FRNTLFTBRAKE, SECT_REARRGTBRAKE, SECT_REARLFTBRAKE};

void
SimWheelConfig(tCar *car, int index)
{
	GfParmHandle *hdle = car->params;
	tCarElt *carElt = car->carElt;
	tWheel *wheel = &(car->wheel[index]);
	tdble rimdiam, tirewidth, tireratio, pressure;
	tdble x0, Ca, RFactor, EFactor, patchLen;

	pressure              = GfParmGetNum(hdle, WheelSect[index], PRM_PRESSURE, (char*)NULL, 275600);
	rimdiam               = GfParmGetNum(hdle, WheelSect[index], PRM_RIMDIAM, (char*)NULL, 0.33f);
	tirewidth             = GfParmGetNum(hdle, WheelSect[index], PRM_TIREWIDTH, (char*)NULL, 0.145f);
	tireratio             = GfParmGetNum(hdle, WheelSect[index], PRM_TIRERATIO, (char*)NULL, 0.75f);
	wheel->mu             = GfParmGetNum(hdle, WheelSect[index], PRM_MU, (char*)NULL, 1.0f);
	wheel->I              = GfParmGetNum(hdle, WheelSect[index], PRM_INERTIA, (char*)NULL, 1.5f);
	wheel->I += wheel->brake.I; // add brake inertia
	wheel->staticPos.y    = GfParmGetNum(hdle, WheelSect[index], PRM_YPOS, (char*)NULL, 0.0f);
	x0                    = GfParmGetNum(hdle, WheelSect[index], PRM_RIDEHEIGHT, (char*)NULL, 0.20f);
	wheel->staticPos.az   = GfParmGetNum(hdle, WheelSect[index], PRM_TOE, (char*)NULL, 0.0f);
	wheel->staticPos.ax   = GfParmGetNum(hdle, WheelSect[index], PRM_CAMBER, (char*)NULL, 0.0f);
	Ca                    = GfParmGetNum(hdle, WheelSect[index], PRM_CA, (char*)NULL, 30.0f);
	RFactor               = GfParmGetNum(hdle, WheelSect[index], PRM_RFACTOR, (char*)NULL, 0.8f);
	EFactor               = GfParmGetNum(hdle, WheelSect[index], PRM_EFACTOR, (char*)NULL, 0.7f);
	wheel->lfMax          = GfParmGetNum(hdle, WheelSect[index], PRM_LOADFMAX, (char*)NULL, 1.6f);
	wheel->lfMin          = GfParmGetNum(hdle, WheelSect[index], PRM_LOADFMIN, (char*)NULL, 0.8f);
	wheel->opLoad         = GfParmGetNum(hdle, WheelSect[index], PRM_OPLOAD, (char*)NULL, wheel->weight0 * 1.2f);
	wheel->mass           = GfParmGetNum(hdle, WheelSect[index], PRM_MASS, (char*)NULL, 20.0f);

#ifdef CONFIG_DEBUG
    printf(" ================== WHEEL CONFIGURATION DEBUG =======================\n");
    printf(" INDEX: %i\n", index);
    printf(" pressure       : %f\n", pressure);
    printf(" rimdiam        : %f\n", rimdiam);
    printf(" tirewidth      : %f\n", tirewidth);
    printf(" tireratio      : %f\n", tireratio);
    printf(" mu             : %f\n", wheel->mu);
    printf(" I              : %f\n", wheel->I);
    printf(" x0             : %f\n", x0);
    printf(" staticpos Y    : %f\n", wheel->staticPos.y);
    printf(" staticpos AZ   : %f\n", wheel->staticPos.az);
    printf(" staticpos AZ   : %f\n", wheel->staticPos.ax);
    printf(" Ca             : %f\n", Ca);
    printf(" RFactor        : %f\n", RFactor);
    printf(" EFactor        : %f\n", EFactor);
    printf(" lfMax          : %f\n", wheel->lfMax);
    printf(" lfMin          : %f\n", wheel->lfMin);
    printf(" opLoad         : %f\n", wheel->opLoad);
    printf(" mass           : %f\n", wheel->mass);
    printf(" =====================================================================\n");
#endif

	if (index % 2) {
		wheel->relPos.ax = -wheel->staticPos.ax;
	} else {
		wheel->relPos.ax = wheel->staticPos.ax;
	}

	wheel->lfMin = MIN(0.8f, wheel->lfMin);
	wheel->lfMax = MAX(1.6f, wheel->lfMax);

	RFactor = MIN(1.0f, RFactor);
	RFactor = MAX(0.1f, RFactor);
	EFactor = MIN(1.0f, EFactor);

	patchLen = wheel->weight0 / (tirewidth * pressure);

	wheel->radius = rimdiam / 2.0f + tirewidth * tireratio;
	wheel->tireSpringRate = wheel->weight0 / (wheel->radius * (1.0f - cos(asin(patchLen / (2.0f * wheel->radius)))));
	wheel->relPos.x = wheel->staticPos.x = car->axle[index/2].xpos;
	wheel->relPos.y = wheel->staticPos.y;
	wheel->relPos.z = wheel->radius - wheel->susp.spring.x0;
	wheel->relPos.ay = wheel->relPos.az = 0.0f;
	wheel->steer = 0.0f;

	/* components */
	SimSuspConfig(hdle, SuspSect[index], &(wheel->susp), wheel->weight0, x0);
	SimBrakeConfig(hdle, BrkSect[index], &(wheel->brake));

	carElt->_rimRadius(index) = rimdiam / 2.0f;
	carElt->_tireHeight(index) = tirewidth * tireratio;
	carElt->_tireWidth(index) = tirewidth;
	carElt->_brakeDiskRadius(index) = wheel->brake.radius;
	carElt->_wheelRadius(index) = wheel->radius;

	wheel->mfC = 2.0f - asin(RFactor) * 2.0f / PI;
	wheel->mfB = Ca / wheel->mfC;
	wheel->mfE = EFactor;

	wheel->lfK = log((1.0f - wheel->lfMin) / (wheel->lfMax - wheel->lfMin));

	wheel->feedBack.I += wheel->I;
	wheel->feedBack.spinVel = 0.0f;
	wheel->feedBack.Tq = 0.0f;
	wheel->feedBack.brkTq = 0.0f;
}


void
SimWheelUpdateRide(tCar *car, int index)
{
	tWheel *wheel = &(car->wheel[index]);
	tWheelState *wheelState = &(car->carElt->priv.wheel[index]);
	tdble prex;

	// compute suspension travel
	// IMRE: old track code, disable for now
	// RtTrackGlobal2Local(car->trkPos.seg, wheel->pos.x, wheel->pos.y, &(wheel->trkPos), TR_LPOS_SEGMENT);
	
	tdble Zroad = wheelState->zRoad;
	wheel->zRoad = Zroad;
	prex = wheel->susp.x;
	wheel->susp.x = wheel->rideHeight = wheel->pos.z - Zroad;

	// verify the suspension travel
	SimSuspCheckIn(&(wheel->susp));
	wheel->susp.v = (prex - wheel->susp.x) / SimDeltaTime;
	// update wheel brake
	SimBrakeUpdate(car, wheel, &(wheel->brake));
}


void
SimWheelUpdateForce(tCar *car, int index)
{
	tWheel *wheel = &(car->wheel[index]);
	tWheelState *wheelState = &(car->carElt->priv.wheel[index]);
	tdble axleFz = wheel->axleFz;
	tdble vt, v, v2, wrl; // wheel related velocity
	tdble Fn, Ft;
	tdble waz;
	tdble CosA, SinA;
	tdble s, sa, sx, sy; // slip vector
	tdble stmp, F, Bx;
	tdble mu;
	tdble reaction_force = 0.0f;
	wheel->state = 0;

	// VERTICAL STUFF CONSIDERING SMALL PITCH AND ROLL ANGLES
	// update suspension force
	SimSuspUpdate(&(wheel->susp));

	// check suspension state
	wheel->state |= wheel->susp.state;
	if ((wheel->state & SIM_SUSP_EXT) == 0) {
		wheel->forces.z = axleFz + wheel->susp.force;
		reaction_force = wheel->forces.z;
		if (wheel->forces.z < 0.0f) {
			wheel->forces.z = 0.0f;
		}
	} else {
		wheel->forces.z = 0.0f;
	}

	// update wheel coord, center relative to GC
	wheel->relPos.z = - wheel->susp.x / wheel->susp.spring.bellcrank + wheel->radius;

	// HORIZONTAL FORCES
	waz = wheel->steer + wheel->staticPos.az;
	CosA = cos(waz);
	SinA = sin(waz);

	// tangent velocity.
	vt = wheel->bodyVel.x * CosA + wheel->bodyVel.y * SinA;
	v2 = wheel->bodyVel.x * wheel->bodyVel.x + wheel->bodyVel.y * wheel->bodyVel.y;
	v = sqrt(v2);

	// slip angle
	if (v < 0.000001f) {
		sa = 0.0f;
	} else {
		sa = atan2(wheel->bodyVel.y, wheel->bodyVel.x) - waz;
	}
	NORM_PI_PI(sa);

	wrl = wheel->spinVel * wheel->radius;
	if ((wheel->state & SIM_SUSP_EXT) != 0) {
		sx = sy = 0.0f;
	} else if (v < 0.000001f) {
		sx = wrl;
		sy = 0.0f;
	} else {
		sx = (vt - wrl) / v; /* target */
		// TODO
		// Commented out and reset because sometimes robots apply full throttle and the engine does not rev up
		// nor do the wheels move. I'm not sure if that is the cause, but its actually the only visible candidate
		// from simuv2 in 1.2.2 up to the current version...
		//sx = (vt - wrl) / fabs(vt);
		sy = sin(sa);
	}

	Ft = 0.0f;
	Fn = 0.0f;
	s = sqrt(sx*sx+sy*sy);

	{
		// calculate _skid and _reaction for sound.
		if (v < 2.0f) {
			car->carElt->_skid[index] = 0.0f;
		} else {
			car->carElt->_skid[index] =  MIN(1.0f, (s*reaction_force*0.0002f));
		}
		car->carElt->_reaction[index] = reaction_force;
	}

	stmp = MIN(s, 1.5f);

	// MAGIC FORMULA
	Bx = wheel->mfB * stmp;
	F = sin(wheel->mfC * atan(Bx * (1.0f - wheel->mfE) + wheel->mfE * atan(Bx))) * (1.0f + stmp * simSkidFactor[car->carElt->_skillLevel]);

	// load sensitivity
	mu = wheel->mu * (wheel->lfMin + (wheel->lfMax - wheel->lfMin) * exp(wheel->lfK * wheel->forces.z / wheel->opLoad));
	
	//IMRE: in the next lines we set the wheel's property of friction and rolling resistance
	//TODO: these parameters should be injected into the wheel in each frame
	
//	float kFriction = 1.200f;
	float kFriction = 1.2f;
	kFriction = wheelState->friction;
	kFriction = 1.2f;
	float kRollRes  = 0.006f;
	

	F *= wheel->forces.z * mu * kFriction * (1.0f + 0.05f * sin(-wheel->staticPos.ax * 18.0f));	/* coeff */
	wheel->rollRes = wheel->forces.z * kRollRes;
	car->carElt->priv.wheel[index].rollRes = wheel->rollRes;

	if (s > 0.000001f) {
		// wheel axis based
		Ft -= F * sx / s;
		Fn -= F * sy / s;
	}

	RELAXATION2(Fn, wheel->preFn, 50.0f);
	RELAXATION2(Ft, wheel->preFt, 50.0f);

	wheel->relPos.az = waz;

	wheel->forces.x = Ft * CosA - Fn * SinA;
	wheel->forces.y = Ft * SinA + Fn * CosA;
	wheel->spinTq = Ft * wheel->radius;
	wheel->sa = sa;
	wheel->sx = sx;

	wheel->feedBack.spinVel = wheel->spinVel;
	wheel->feedBack.Tq = wheel->spinTq;
	wheel->feedBack.brkTq = wheel->brake.Tq;

	car->carElt->_wheelSlipSide(index) = sy*v;
	car->carElt->_wheelSlipAccel(index) = sx*v;
	car->carElt->_reaction[index] = reaction_force;
}


void
SimWheelUpdateRotation(tCar *car)
{
	int i;
	tWheel *wheel;

	for (i = 0; i < 4; i++) {
		wheel = &(car->wheel[i]);
		wheel->spinVel = wheel->in.spinVel;

		RELAXATION2(wheel->spinVel, wheel->prespinVel, 50.0f);

		wheel->relPos.ay += wheel->spinVel * SimDeltaTime;
		NORM_PI_PI(wheel->relPos.ay);
		car->carElt->_wheelSpinVel(i) = wheel->spinVel;
	}
}


void
SimUpdateFreeWheels(tCar *car, int axlenb)
{
	int i;
	tWheel *wheel;
	tdble BrTq;		// brake torque
	tdble ndot;		// rotation acceleration
	tdble I;

	for (i = axlenb * 2; i < axlenb * 2 + 2; i++) {
		wheel = &(car->wheel[i]);

		I = wheel->I + car->axle[axlenb].I / 2.0f;

		ndot = SimDeltaTime * wheel->spinTq / I;
		wheel->spinVel -= ndot;

		BrTq = - SIGN(wheel->spinVel) * wheel->brake.Tq;
		ndot = SimDeltaTime * BrTq / I;

		if (fabs(ndot) > fabs(wheel->spinVel)) {
			ndot = -wheel->spinVel;
		}

		wheel->spinVel += ndot;
		wheel->in.spinVel = wheel->spinVel;
	}
}
