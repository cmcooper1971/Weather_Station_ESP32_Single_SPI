// 
// 
// 

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "drawCompass.h"
#include "colours.h"                // Colours.

// Compass - Thanks to Pololu Forum - User Streichre https://forum.pololu.com/t/1-8-tft-display-to-baby-o-with-lsm303dlh-compass/4125

/* Setting the DialRadius determines the size of the dial.
int x, y sets the position of the center of the dial.
All parameters associated with the dial will attempt to
locate around the dial.  Making the DialRadius too large
will push parts of text and the dial off the screen.
Design differnt dials for compass, speed, wind direction, etc. */

int DialRadius = 65;
int x = 110;
int y = 130;

// These are used to draw lines and the dial needle.

double calcSin = 0;
double myPI = 3.14;

// Array used to draw the tick marks around the dial.

double tickMarks[12] = { 22.5, 45, 67.5, 112.5, 135, 157.5, 202.5, 225, 247.5, 292.5, 315, 337.5 };

// Compass pointer.

#define DEG2RAD 0.0174532925 

float compassPivot_x = 70;
float compassPivot_y = 90;
float c_x1, c_x2, c_x3, c_x4;
float c_y1, c_y2, c_y3, c_y4;
float c_x1_old, c_x2_old, c_x3_old, c_x4_old;
float c_y1_old, c_y2_old, c_y3_old, c_y4_old;

float compassAngle;
int   compass_r = 50;

/*-----------------------------------------------------------------*/

// Draws the main dial.

void drawCompass(Adafruit_ILI9341& tft) {

	// Draw the dial.

	tft.setTextSize(2);
	tft.drawCircle(x, y, DialRadius + 6, BLACK);
	tft.drawCircle(x, y, DialRadius - 5, BLACK);

	// Current N, S, E, W tick marks.

	tft.fillTriangle(x + 5, y - DialRadius - 10, x - 5, y - DialRadius - 10, x, y - DialRadius - 1, RED);
	tft.fillTriangle(x - 5, y + DialRadius + 10, x + 5, y + DialRadius + 10, x, y + DialRadius + 1, BLUE);
	tft.fillTriangle(x - DialRadius - 10, y - 5, x - DialRadius - 10, y + 5, x - DialRadius - 1, y, BLUE);
	tft.fillTriangle(x + DialRadius + 10, y - 5, x + DialRadius + 10, y + 5, x + DialRadius + 1, y, BLUE);

	// The center dot in the middle of the dial.

	tft.fillCircle(x, y, 3, RED);

	// Text around the dial attempts to adjust to dial size.

	tft.setCursor(x - 5, y - DialRadius - 30);
	tft.setTextColor(RED);
	tft.print("N");

	tft.setCursor(x - 5, y + DialRadius + 15);
	tft.setTextColor(BLUE);
	tft.print("S");

	tft.setCursor(x - DialRadius - 25, y - 7);
	tft.print("W");

	tft.setCursor(x + DialRadius + 15, y - 7);
	tft.print("E");

	tft.setTextSize(1);

	tft.setCursor((x + ((DialRadius + 45) / 2)), (y - ((DialRadius + 50) / 2)));
	tft.print("NE");

	tft.setCursor((x + ((DialRadius + 45) / 2)), (y + ((DialRadius + 50) / 2)));
	tft.print("SE");

	tft.setCursor((x - ((DialRadius + 70) / 2)), (y + ((DialRadius + 50) / 2)));
	tft.print("SW");

	tft.setCursor((x - ((DialRadius + 70) / 2)), (y - ((DialRadius + 50) / 2)));
	tft.print("NW");

	tft.setTextColor(BLACK);
	tft.setTextSize(2);

	// Draw dial tick marks.

	drawTicks(tft);

} // Close function.

/*-----------------------------------------------------------------*/

void drawTicks(Adafruit_ILI9341& tft) {

	// Uses the tickMark[] array

	for (int i = 0; i < 12; i++) {

		double xtick_F = 0;
		double ytick_F = 0;
		double xtick_T = 0;
		double ytick_T = 0;

		double x_t = 0;
		double calcRad = tickMarks[i] * myPI / 180;
		calcSin = sin(calcRad);

		if (tickMarks[i] > -1 && tickMarks[i] < 91)
		{
			xtick_F = sin(calcRad) * (DialRadius - 8);
			x_t = -1 * xtick_F;
			ytick_F = -1 * sqrt((DialRadius - 8) * (DialRadius - 8) - (x_t * x_t));
			xtick_T = sin(calcRad) * (DialRadius + 8);
			x_t = -1 * xtick_T;
			ytick_T = -1 * sqrt((DialRadius + 8) * (DialRadius + 8) - (x_t * x_t));
		}

		if (tickMarks[i] > 90 && tickMarks[i] < 181)
		{
			xtick_F = sin(calcRad) * (DialRadius - 8);
			x_t = xtick_F;
			ytick_F = sqrt((DialRadius - 8) * (DialRadius - 8) - (x_t * x_t));
			xtick_T = sin(calcRad) * (DialRadius + 8);
			x_t = xtick_T;
			ytick_T = sqrt((DialRadius + 8) * (DialRadius + 8) - (x_t * x_t));
		}

		if (tickMarks[i] > 180 && tickMarks[i] < 271)
		{
			xtick_F = sin(calcRad) * (DialRadius - 8);
			x_t = xtick_F;
			ytick_F = sqrt((DialRadius - 8) * (DialRadius - 8) - (x_t * x_t));
			xtick_T = sin(calcRad) * (DialRadius + 8);
			x_t = xtick_T;
			ytick_T = sqrt((DialRadius + 8) * (DialRadius + 8) - (x_t * x_t));
		}

		if (tickMarks[i] > 270 && tickMarks[i] < 361)
		{
			xtick_F = sin(calcRad) * (DialRadius - 8);
			x_t = -1 * xtick_F;
			ytick_F = -1 * sqrt((DialRadius - 8) * (DialRadius - 8) - (x_t * x_t));
			xtick_T = sin(calcRad) * (DialRadius + 8);
			x_t = -1 * xtick_T;
			ytick_T = -1 * sqrt((DialRadius + 8) * (DialRadius + 8) - (x_t * x_t));
		}

		tft.drawLine(x + xtick_T, y + ytick_T, x + xtick_F, y + ytick_F, RED);
	}

} // Close function.

/*-----------------------------------------------------------------*/

void drawNeedle(Adafruit_ILI9341& tft, int degree) {

	// Remove old compass pointer.
	
	tft.fillTriangle(c_x1_old, c_y1_old, c_x2_old, c_y2_old, c_x4_old, c_y4_old, WHITE);          
	tft.fillTriangle(c_x1_old, c_y1_old, c_x3_old, c_y3_old, c_x4_old, c_y4_old, WHITE);

	// Set angle.

	compassAngle = ((degree - 90) * DEG2RAD);

	// Ccalculate coordinates

	c_x1 = ((compassPivot_x + 40) + (compass_r * cos(compassAngle)));                       
	c_y1 = ((compassPivot_y + 40) + (compass_r * sin(compassAngle)));

	c_x2 = ((compassPivot_x + 40) + (compass_r * cos(compassAngle + 170 * DEG2RAD)));
	c_y2 = ((compassPivot_y + 40) + (compass_r * sin(compassAngle + 170 * DEG2RAD)));

	c_x3 = ((compassPivot_x + 40) + (compass_r * cos(compassAngle - 170 * DEG2RAD)));
	c_y3 = ((compassPivot_y + 40) + (compass_r * sin(compassAngle - 170 * DEG2RAD)));

	c_x4 = ((compassPivot_x + 40) + ((compass_r - 4) * cos(compassAngle - 180 * DEG2RAD)));
	c_y4 = ((compassPivot_y + 40) + ((compass_r - 4) * sin(compassAngle - 180 * DEG2RAD)));

	c_x1_old = c_x1; c_x2_old = c_x2; c_x3_old = c_x3;   c_x4_old = c_x4;
	c_y1_old = c_y1; c_y2_old = c_y2; c_y3_old = c_y3;   c_y4_old = c_y4;

	// New pointer.

	tft.fillTriangle(c_x1, c_y1, c_x3, c_y3, c_x4, c_y4, LTBLUE);                          
	tft.fillTriangle(c_x1, c_y1, c_x2, c_y2, c_x4, c_y4, BLUE);

} // Close function.

/*-----------------------------------------------------------------*/
