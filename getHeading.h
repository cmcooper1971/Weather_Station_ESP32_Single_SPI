// getHeading.h

#ifndef _GETHEADING_h
#define _GETHEADING_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

int getHeadingReturn(int direction);

#endif

