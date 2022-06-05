// 
// 
// 

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "drawBitmap.h"

// Thanks to https://github.com/KrisKasprzak

/*-----------------------------------------------------------------*/

void drawBitmap(Adafruit_ILI9341& tft, int x, int y, const uint16_t* bitmap, int bw, int bh) {

	//int h = 64, w = 64, row, col, buffidx = 0;
	int buffidx = 0;
	int row;
	int col;
	int w = bw + y;
	int h = bh + x;

	for (row = x; row < h; row++) {
		
		// For each scanline...
		for (col = y; col < w; col++) { // For each pixel...
		  //To read from Flash Memory, pgm_read_XXX is required.
		  //Since image is stored as uint16_t, pgm_read_word is used as it uses 16bit address
			tft.drawPixel(col, row, pgm_read_word(bitmap + buffidx));
			buffidx++;
		} // end pixel
	}

} // Close function.

/*-----------------------------------------------------------------*/
