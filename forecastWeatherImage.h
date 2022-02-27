// forecastWeatherImage.h

#ifndef _FORECASTWEATHERIMAGE_h
#define _FORECASTWEATHERIMAGE_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

void forecastWeatherImage(Adafruit_ILI9341& tft, int weatherID);

#endif

