// drawCompass.h

#ifndef _DRAWCOMPASS_h
#define _DRAWCOMPASS_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

void drawCompass(Adafruit_ILI9341& tft);
void drawTicks(Adafruit_ILI9341& tft);
void drawNeedle(Adafruit_ILI9341& tft, double _degree);

#endif

