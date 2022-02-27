// tftControl.h

#ifndef _TFTCONTROL_h
#define _TFTCONTROL_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

void disableVSPIScreens();
void disableHSPIScreens();
void enableScreen1();
void enableScreen2();
void enableScreen3();
void enableScreen4();
void enableScreen5();
void enableScreen6();
void enableScreen7();
void enableScreen8();

#endif

