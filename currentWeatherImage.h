// currentWeatherImage.h

#ifndef _CURRENTWEATHERIMAGE_h
#define _CURRENTWEATHERIMAGE_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

void currentWeatherImage(Adafruit_ILI9341& tft, int weatherID, boolean dayNight);

#endif

