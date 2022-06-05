// 
// 
// 

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "currentWeatherImage.h"	// Main weather image.
#include "screenLayout.h"			// Screen layout.
#include "drawBitmap.h"				// Draw bitmaps.
#include "tftControl.h"				// Enable / disable TFT screens
#include "iconsAtmosphere.h"		// Weather bitmaps.
#include "iconsClear.h"				// Weather bitmaps.
#include "iconsClouds.h"			// Weather bitmaps.
#include "iconsDrizzle.h"			// Weather bitmaps.
#include "iconsRain.h"				// Weather bitmaps.
#include "iconsSnow.h"				// Weather bitmaps.
#include "iconsThunderstorms.h"		// Weather bitmaps.


/*-----------------------------------------------------------------*/

// Display current weather main image.

void currentWeatherImage(Adafruit_ILI9341& tft, int weatherID, boolean dayNight) {

	enableScreen1();

	if (dayNight == false) {

		if (weatherID == 200 || weatherID == 201 || weatherID == 202 || weatherID == 230 || weatherID == 231 || weatherID == 232) {
			//Serial.println(F("Thunderstorm with rain or drizzle"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, thunderstormRain, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 210 || weatherID == 211 || weatherID == 212 || weatherID == 221) {
			//Serial.println(F("Thunderstorm"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, thunderstorm, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 300 || weatherID == 301) {
			//Serial.println(F("Light drizzle"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, lightRainNight, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 302) {
			//Serial.println(F("Heavy drizzle"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, heavyRainNight, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 310 || weatherID == 311) {
			//Serial.println(F("Drizzle"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, lightRainNight, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 312 || weatherID == 313 || weatherID == 314 || weatherID == 321) {
			//Serial.println(F("Drizzle"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, rainNight, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 500 || weatherID == 501) {
			//Serial.println(F("Light or moderate rain"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, lightRainNight, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 502) {
			//Serial.println(F("Heavy rain"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, heavyRainNight, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 503 || weatherID == 504) {
			//Serial.println(F("Heavy or intense rain"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, heavyRainNight, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 504) {
			//Serial.println(F("Freezing rain"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, sleet, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 520 || weatherID == 521 || weatherID == 522 || weatherID == 531) {
			//Serial.println(F("Shower rain"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, showerRain, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 600) {
			//Serial.println(F("Light snow"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, lightSnowNight, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 601) {
			//Serial.println(F("Snow"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, snowNight, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 602) {
			Serial.println(F("Heavy snow"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, heavySnowNight, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 611 || weatherID == 612 || weatherID == 613 || weatherID == 615 || weatherID == 616 || weatherID == 620 || weatherID == 621 || weatherID == 622) {
			//Serial.println(F("All sorts of snow"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, sleet, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID >= 700 && weatherID <= 799) {
			//Serial.println(F("All sorts of mist fog"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, fogMist, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 800) {
			//Serial.println(F("Clear"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, clearSkyNight, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 801 || weatherID == 802) {
			//Serial.println(F("Few clouds"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, fewCloudsNight, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 803) {
			//Serial.println(F("Broken clouds"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, fewCloudsNight, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 804) {
			//Serial.println(F("Overcast clouds"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, fewCloudsNight, WEATHERICON_W, WEATHERICON_H);
		}

		// Make sure this goes back in

		else {
			//Serial.println(F("Who knows?"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, rainbow, WEATHERICON_W, WEATHERICON_H);
		}

	}

	if (dayNight == true) {

		if (weatherID == 200 || weatherID == 201 || weatherID == 202 || weatherID == 230 || weatherID == 231 || weatherID == 232) {
			//Serial.println(F("Thunderstorm with rain or drizzle"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, thunderstormRain, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 210 || weatherID == 211 || weatherID == 212 || weatherID == 221) {
			//Serial.println(F("Thunderstorm"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, thunderstorm, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 300 || weatherID == 301) {
			//Serial.println(F("Light drizzle"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, lightDrizzle, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 302) {
			//Serial.println(F("Heavy drizzle"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, heavyDrizzle, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 310 || weatherID == 311) {
			//Serial.println(F("Drizzle"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, drizzle, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 312 || weatherID == 313 || weatherID == 314 || weatherID == 321) {
			//Serial.println(F("Drizzle"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, showerRain, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 500 || weatherID == 501) {
			//Serial.println(F("Light or moderate rain"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, lightRain, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 502) {
			//Serial.println(F("Heavy rain"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, heavyRain, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 503 || weatherID == 504) {
			//Serial.println(F("Heavy or intense rain"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, heavyRain, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 504) {
			//Serial.println(F("Freezing rain"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, sleet, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 520 || weatherID == 521 || weatherID == 522 || weatherID == 531) {
			//Serial.println(F("Shower rain"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, showerRain, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 600) {
			//Serial.println(F("Light snow"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, lightSnow, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 601) {
			//Serial.println(F("Snow"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, snow, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 602) {
			//Serial.println(F("Heavy snow"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, heavySnow, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 611 || weatherID == 612 || weatherID == 613 || weatherID == 615 || weatherID == 616 || weatherID == 620 || weatherID == 621 || weatherID == 622) {
			//Serial.println(F("All sorts of snow"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, sleet, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID >= 700 && weatherID <= 799) {
			//Serial.println(F("All sorts of mist fog"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, fogMist, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 800) {
			//Serial.println(F("Clear"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, clearSkyDay, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 801 || weatherID == 802) {
			//Serial.println(F("Few clouds"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, fewCloudsDay, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 803) {
			//Serial.println(F("Broken clouds"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, brokenCloudsDay, WEATHERICON_W, WEATHERICON_H);
		}

		else if (weatherID == 804) {
			//Serial.println(F("Overcast clouds"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, overcastCloudsDay, WEATHERICON_W, WEATHERICON_H);
		}

		// Make sure this goes back in

		else {
			//Serial.println(F("Who knows?"));
			drawBitmap(tft, WEATHERICON_Y, WEATHERICON_X, rainbow, WEATHERICON_W, WEATHERICON_H);
		}

	}

	disableVSPIScreens();

}  // Close function.

/*-----------------------------------------------------------------*/