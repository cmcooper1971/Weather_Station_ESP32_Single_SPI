// 
// 
// 

#include "getHeading.h"				// Get wind heading.

/*-----------------------------------------------------------------*/

// Calculate wind direction

int getHeadingReturn(int direction) {

	// Using array char* headingArray[8] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };

	if (direction < 22) {
		Serial.println("N");
		return 0;
		//heading = 0;
	}

	else if (direction < 67) {
		Serial.println("NE");
		return 1;
		//heading = 1;
	}

	else if (direction < 112) {
		Serial.println("E");
		return 2;
		//heading = 2;
	}

	else if (direction < 157) {
		Serial.println("SE");
		return 3;
		//heading = 3;
	}

	else if (direction < 212) {
		Serial.println("S");
		return 4;
		//heading = 4;
	}

	else if (direction < 247) {
		Serial.println("SW");
		return 5;
		//heading = 5;
	}

	else if (direction < 292) {
		Serial.println("W");
		return 6;
		//heading = 6;
	}

	else if (direction < 337) {
		Serial.println("NW");
		return 7;
		//heading = 7;
	}

	else {
		Serial.println("N");
		return 0;
		//heading = 0;
	}

}  // Close function.

/*-----------------------------------------------------------------*/

