// drawBarGraph.h

#ifndef _DRAWBARGRAPH_h
#define _DRAWBARGRAPH_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

void drawBarChartV1(Adafruit_ILI9341& tft, byte screen, double x, double y, double w, double h, double loval, double hival, double inc, int curval, int dig, int dec, unsigned int barcolor, unsigned int voidcolor, unsigned int bordercolor, unsigned int textcolor, unsigned int backcolor, String label, boolean fillBar, boolean& redraw);
void drawBarChartV2(Adafruit_ILI9341& tft, byte screen, double x, double y, double w, double h, double loval, double hival, double inc, int curval, int dig, int dec, unsigned int barcolor, unsigned int voidcolor, unsigned int bordercolor, unsigned int textcolor, unsigned int backcolor, String label, boolean fillBar, boolean& redraw);
String Format(double val, int dec, int dig);

#endif

