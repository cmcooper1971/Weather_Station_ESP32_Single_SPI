// 
// 
// 

#include "tftControl.h"		// Enable / disable TFT screens

#define VSPI_CS0	36 // This is set to an erroneous pin as to not confuse manual chip selects using digital writes.
#define VSPI_CS1    5  // Screen one chip select
#define VSPI_CS2    21 // Screen two chip select
#define VSPI_CS3    16 // Screen three chip select
#define VSPI_CS4    17 // Screen four chip select

#define HSPI_CS9    39 // This is set to an erroneous pin as to not confuse manual chip selects using digital writes.
#define HSPI_CS5    15 // Screen five chip select.
#define HSPI_CS6    25 // Screen six chip select.
#define HSPI_CS7    33 // Screen seven chip select.
#define HSPI_CS8    32 // Screen eight chip select.

/*-----------------------------------------------------------------*/

// Disable outputs for all displays on VSPI bus.

void disableVSPIScreens() {

	digitalWrite(VSPI_CS1, HIGH);
	digitalWrite(VSPI_CS2, HIGH);
	digitalWrite(VSPI_CS3, HIGH);
	digitalWrite(VSPI_CS4, HIGH);

} // Close function.

/*-----------------------------------------------------------------*/

// Disable outputs for all displays on HSPI bus.

void disableHSPIScreens() {

	digitalWrite(HSPI_CS5, HIGH);
	digitalWrite(HSPI_CS6, HIGH);
	digitalWrite(HSPI_CS7, HIGH);
	digitalWrite(HSPI_CS8, HIGH);

} // Close function.

/*-----------------------------------------------------------------*/

// Enable screen one on bank one to recieve SPI commands.

void enableScreen1() {

	digitalWrite(VSPI_CS1, LOW);
	digitalWrite(VSPI_CS2, HIGH);
	digitalWrite(VSPI_CS3, HIGH);
	digitalWrite(VSPI_CS4, HIGH);

} // Close function.

/*-----------------------------------------------------------------*/

// Enable screen two on bank one to recieve SPI commands.

void enableScreen2() {

	digitalWrite(VSPI_CS1, HIGH);
	digitalWrite(VSPI_CS2, LOW);
	digitalWrite(VSPI_CS3, HIGH);
	digitalWrite(VSPI_CS4, HIGH);

} // Close function.

/*-----------------------------------------------------------------*/

// Enable screen three on bank one to recieve SPI commands.

void enableScreen3() {

	digitalWrite(VSPI_CS1, HIGH);
	digitalWrite(VSPI_CS2, HIGH);
	digitalWrite(VSPI_CS3, LOW);
	digitalWrite(VSPI_CS4, HIGH);

} // Close function.

/*-----------------------------------------------------------------*/

// Enable screen four on bank one to recieve SPI commands.

void enableScreen4() {

	digitalWrite(VSPI_CS1, HIGH);
	digitalWrite(VSPI_CS2, HIGH);
	digitalWrite(VSPI_CS3, HIGH);
	digitalWrite(VSPI_CS4, LOW);

} // Close function.

/*-----------------------------------------------------------------*/

// Enable screen five on bank two to recieve SPI commands.

void enableScreen5() {

	digitalWrite(HSPI_CS5, LOW);
	digitalWrite(HSPI_CS6, HIGH);
	digitalWrite(HSPI_CS7, HIGH);
	digitalWrite(HSPI_CS8, HIGH);

} // Close function.

/*-----------------------------------------------------------------*/

// Enable screen six on bank two to recieve SPI commands.

void enableScreen6() {

	digitalWrite(HSPI_CS5, HIGH);
	digitalWrite(HSPI_CS6, LOW);
	digitalWrite(HSPI_CS7, HIGH);
	digitalWrite(HSPI_CS8, HIGH);

} // Close function.

/*-----------------------------------------------------------------*/

// Enable screen seven on bank two to recieve SPI commands.

void enableScreen7() {

	digitalWrite(HSPI_CS5, HIGH);
	digitalWrite(HSPI_CS6, HIGH);
	digitalWrite(HSPI_CS7, LOW);
	digitalWrite(HSPI_CS8, HIGH);

}  // Close function.

/*-----------------------------------------------------------------*/

// Enable screen eight on bank two to recieve SPI commands.

void enableScreen8() {

	digitalWrite(HSPI_CS5, HIGH);
	digitalWrite(HSPI_CS6, HIGH);
	digitalWrite(HSPI_CS7, HIGH);
	digitalWrite(HSPI_CS8, LOW);
}

/*-----------------------------------------------------------------*/
