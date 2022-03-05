// 
// 
// 

#include "tftControl.h"		// Enable / disable TFT screens

#define VSPI_CS0	36 // This is set to an erroneous pin as to not confuse manual chip selects using digital writes.
#define VSPI_CS1    5  // Screen one chip select.
#define VSPI_CS2    17  // 21 // Screen two chip select.
#define VSPI_CS3    16 // 16 // Screen three chip select.
#define VSPI_CS4    15 // 17 // Screen four chip select.
#define VSPI_CS5    26 // 15 // Screen five chip select.
#define VSPI_CS6    25 // Screen six chip select.
#define VSPI_CS7    33 // Screen seven chip select.
#define VSPI_CS8    32 // Screen eight chip select.

/*-----------------------------------------------------------------*/

// Disable outputs for all displays on VSPI bus.

void disableVSPIScreens() {

	digitalWrite(VSPI_CS1, HIGH);
	digitalWrite(VSPI_CS2, HIGH);
	digitalWrite(VSPI_CS3, HIGH);
	digitalWrite(VSPI_CS4, HIGH);
	digitalWrite(VSPI_CS5, HIGH);
	digitalWrite(VSPI_CS6, HIGH);
	digitalWrite(VSPI_CS7, HIGH);
	digitalWrite(VSPI_CS8, HIGH);

} // Close function.

/*-----------------------------------------------------------------*/

// Enable screen one on bank one to recieve SPI commands.

void enableScreen1() {

	digitalWrite(VSPI_CS1, LOW);
	digitalWrite(VSPI_CS2, HIGH);
	digitalWrite(VSPI_CS3, HIGH);
	digitalWrite(VSPI_CS4, HIGH);
	digitalWrite(VSPI_CS5, HIGH);
	digitalWrite(VSPI_CS6, HIGH);
	digitalWrite(VSPI_CS7, HIGH);
	digitalWrite(VSPI_CS8, HIGH);

} // Close function.

/*-----------------------------------------------------------------*/

// Enable screen two on bank one to recieve SPI commands.

void enableScreen2() {

	digitalWrite(VSPI_CS1, HIGH);
	digitalWrite(VSPI_CS2, LOW);
	digitalWrite(VSPI_CS3, HIGH);
	digitalWrite(VSPI_CS4, HIGH);
	digitalWrite(VSPI_CS5, HIGH);
	digitalWrite(VSPI_CS6, HIGH);
	digitalWrite(VSPI_CS7, HIGH);
	digitalWrite(VSPI_CS8, HIGH);

} // Close function.

/*-----------------------------------------------------------------*/

// Enable screen three on bank one to recieve SPI commands.

void enableScreen3() {

	digitalWrite(VSPI_CS1, HIGH);
	digitalWrite(VSPI_CS2, HIGH);
	digitalWrite(VSPI_CS3, LOW);
	digitalWrite(VSPI_CS4, HIGH);
	digitalWrite(VSPI_CS5, HIGH);
	digitalWrite(VSPI_CS6, HIGH);
	digitalWrite(VSPI_CS7, HIGH);
	digitalWrite(VSPI_CS8, HIGH);

} // Close function.

/*-----------------------------------------------------------------*/

// Enable screen four on bank one to recieve SPI commands.

void enableScreen4() {

	digitalWrite(VSPI_CS1, HIGH);
	digitalWrite(VSPI_CS2, HIGH);
	digitalWrite(VSPI_CS3, HIGH);
	digitalWrite(VSPI_CS4, LOW);
	digitalWrite(VSPI_CS5, HIGH);
	digitalWrite(VSPI_CS6, HIGH);
	digitalWrite(VSPI_CS7, HIGH);
	digitalWrite(VSPI_CS8, HIGH);

} // Close function.

/*-----------------------------------------------------------------*/

// Enable screen five on bank two to recieve SPI commands.

void enableScreen5() {

	digitalWrite(VSPI_CS1, HIGH);
	digitalWrite(VSPI_CS2, HIGH);
	digitalWrite(VSPI_CS3, HIGH);
	digitalWrite(VSPI_CS4, HIGH);
	digitalWrite(VSPI_CS5, LOW);
	digitalWrite(VSPI_CS6, HIGH);
	digitalWrite(VSPI_CS7, HIGH);
	digitalWrite(VSPI_CS8, HIGH);

} // Close function.

/*-----------------------------------------------------------------*/

// Enable screen six on bank two to recieve SPI commands.

void enableScreen6() {

	digitalWrite(VSPI_CS1, HIGH);
	digitalWrite(VSPI_CS2, HIGH);
	digitalWrite(VSPI_CS3, HIGH);
	digitalWrite(VSPI_CS4, HIGH);
	digitalWrite(VSPI_CS5, HIGH);
	digitalWrite(VSPI_CS6, LOW);
	digitalWrite(VSPI_CS7, HIGH);
	digitalWrite(VSPI_CS8, HIGH);

} // Close function.

/*-----------------------------------------------------------------*/

// Enable screen seven on bank two to recieve SPI commands.

void enableScreen7() {

	digitalWrite(VSPI_CS1, HIGH);
	digitalWrite(VSPI_CS2, HIGH);
	digitalWrite(VSPI_CS3, HIGH);
	digitalWrite(VSPI_CS4, HIGH);
	digitalWrite(VSPI_CS5, HIGH);
	digitalWrite(VSPI_CS6, HIGH);
	digitalWrite(VSPI_CS7, LOW);
	digitalWrite(VSPI_CS8, HIGH);

}  // Close function.

/*-----------------------------------------------------------------*/

// Enable screen eight on bank two to recieve SPI commands.

void enableScreen8() {

	digitalWrite(VSPI_CS1, HIGH);
	digitalWrite(VSPI_CS2, HIGH);
	digitalWrite(VSPI_CS3, HIGH);
	digitalWrite(VSPI_CS4, HIGH);
	digitalWrite(VSPI_CS5, HIGH);
	digitalWrite(VSPI_CS6, HIGH);
	digitalWrite(VSPI_CS7, HIGH);
	digitalWrite(VSPI_CS8, LOW);
}

/*-----------------------------------------------------------------*/
