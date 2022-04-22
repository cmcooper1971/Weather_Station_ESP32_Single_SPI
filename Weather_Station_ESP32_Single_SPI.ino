/*
 Name:		Weather_Station_ESP32_Single_SPI.ino
 Created:	27/02/2021 00:00:00
 Author:	Christopher Cooper
*/

// Libraries.

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <WiFi.h>
#include <ESP32Time.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <ArduinoJSON.h>
#include <Adafruit_MCP23X08.h>		// Additional I/O port expander
#include <SimpleBME280.h>			// Internal environment sensor
#include <XPT2046_Touchscreen.h>	// Touch screen control

#include <Fonts/FreeSans9pt7b.h>	
#include <Fonts/FreeSans12pt7b.h>
#include <EEPROM.h>

#include "getHeading.h"				// Convert headings to N/S/E/W
#include "tftControl.h"				// Enable / disable displays
#include "currentWeatherImage.h"	// Draw main weather image
#include "forecastWeatherImage.h"	// Draw main weather image
#include "drawBitmap.h"				// Draw bitmaps
#include "drawBarGraph.h"			// Draw bar graphs
#include "drawCompass.h"			// Draw compass

#include "colours.h"                // Colours
#include "screenLayout.h"			// Screen layout
#include "icons.h"					// Weather bitmaps
#include "iconsAtmosphere.h"		// Weather bitmaps
#include "iconsClear.h"				// Weather bitmaps
#include "iconsClouds.h"			// Weather bitmaps
#include "iconsDrizzle.h"			// Weather bitmaps
#include "iconsRain.h"				// Weather bitmaps
#include "iconsSnow.h"				// Weather bitmaps
#include "iconsThunderstorms.h"		// Weather bitmaps
#include "iconsMoonPhase.h"			// Weather bitmaps

// TFT SPI Interface for ESP32.

#define VSPI_MISO   19 // MISO - Not needed for display(s)
#define VSPI_MOSI   23 // MOSI
#define VSPI_CLK    18 // SCK
#define VSPI_DC     4  // DC
#define VSPI_RST    13 // RST							

#define VSPI_CS0	36 // This is set to an erroneous pin as to not confuse manual chip selects using digital writes.
#define VSPI_CS1    5  // Screen one chip select
#define VSPI_CS2    17 // Screen two chip select
#define VSPI_CS3    16 // Screen three chip select
#define VSPI_CS4    15 // Screen four chip select
#define VSPI_CS5    26 // Screen five chip select
#define VSPI_CS6    25 // Screen six chip select
#define VSPI_CS7    33 // Screen seven chip select
#define VSPI_CS8    32 // Screen eight chip select
#define touch_CS	0  // Touch chip select

XPT2046_Touchscreen ts(touch_CS);	// Setup touch on TFT4

// MCP23008 Interface.

#define TFT_LED1_CTRL	4 // GPIO MCP23008
#define TFT_LED2_CTRL	5 // GPIO MCP23008
#define TFT_LED3_CTRL	6 // GPIO MCP23008
#define TFT_LED4_CTRL	7 // GPIO MCP23008
#define TFT_LED5_CTRL	1 // GPIO MCP23008
#define TFT_LED6_CTRL	2 // GPIO MCP23008
#define TFT_LED7_CTRL	3 // GPIO MCP23008
#define TFT_LED8_CTRL	0 // GPIO MCP23008

#define MCP23008_RST	27// Reset MCP23008 line

// Other defines.

//#define interruptSWITCH1	1
//#define interruptSWITCH2	3
#define touchSwitch1 

#define SEALEVELPRESSURE_HPA (1013.25)

// Configure switches.

boolean switchOneState = false;
boolean switchOneToggled = false;

boolean switchTwoState = false;
boolean switchTwoToggled = false;

// VSPI Class (default).

Adafruit_ILI9341 tft = Adafruit_ILI9341(VSPI_CS0, VSPI_DC, VSPI_RST); // CS0 is a dummy pin

// MCP23008.

Adafruit_MCP23X08 mcp;

// BME280 Sensor.

SimpleBME280 bme280;
float t, p, h;

// Configure time settings.

ESP32Time rtc;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
int			daylightOffset_sec = 0;
long		LocalTime;
int			localTimeInterval = 60000;

// Web Server configuration.

AsyncWebServer server(80);			// Create AsyncWebServer object on port 80
AsyncEventSource events("/events");	// Create an Event Source on /events

// WiFi Configuration.

boolean wiFiYN;						// WiFi reset
boolean apMode = false;

// Search for parameter in HTTP POST request.

const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "subnet";
const char* PARAM_INPUT_5 = "gateway";
const char* PARAM_INPUT_6 = "dns";

// Variables to save values from HTML form.

String ssid;
String pass;
String ip;
String subnet;
String gateway;
String dns;

// File paths to save input values permanently.

const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";
const char* subnetPath = "/subnet.txt";
const char* gatewayPath = "/gateway.txt";
const char* dnsPath = "/dns.txt";

// Network variables.

IPAddress localIP;
IPAddress Gateway;
IPAddress Subnet;
IPAddress dns1;

// Json Variable to Hold Sensor Readings.

String jsonBuffer;
DynamicJsonDocument weatherData(35788); //35788

// Open Weather - your domain name with URL path or IP address with path.

String openWeatherMapApiKey = "03460ae66bafdbb98dac33a0f7509330";

// Replace with your country code and city.

String city = "Norwich";
String countryCode = "GB";
String latitude = "52.628101";
String longitude = "1.299350";

// Timer variables (check Open Weather).

unsigned long intervalT = 0;
unsigned long intervalTime = 300000;			// Reset to 300,000 when finished with design (5 minutes sleep time)

// Timer variables (check wifi).

unsigned long wiFiR = 0;					// WiFi retry (wiFiR) to attempt connecting to the last known WiFi if connection lost
unsigned long wiFiRI = 60000;				// WiFi retry Interval (wiFiRI) used to retry connecting to the last known WiFi if connection lost
unsigned long previousMillis = 0;			// Used in the WiFI Init function
const long interval = 10000;				// Interval to wait for Wi-Fi connection (milliseconds)

// Timer variables (to update web server).

unsigned long lastTime = 0;
unsigned long timerDelay = 30000;

// Timer variables (changing temperature readings).

boolean flagTempDisplayChange = false;
unsigned long intervalTT = 0;
unsigned long intervalTTime = 3000;

// Weather variables.

boolean dayNight = false;

byte heading;
char headingArray[8][3] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };

// Current weather variables.

unsigned long currentWeatherTime;
double tempNow;
double feelsLikeTempNow;
unsigned int pressureNow;
double humidityNow;
double windSpeedNow;
unsigned int windDirectionNow;
byte uvIndexNow;
unsigned int currentWeatherNow;
String weatherDesCurrent;
double rainLevelNow;
unsigned int snowLevelNow;
unsigned long sunRiseNow;
unsigned long sunSetNow;

// Forecast weather variables.

unsigned long forecastTimeFc1;
double tempMornFc1;
double tempDayFc1;
double tempEvenFc1;
double tempNightFc1;
double tempMinFc1;
double tempMaxFc1;
unsigned int pressureFc1;
double humidityFc1;
double windSpeedFc1;
unsigned int windDirectionFc1;
byte uvIndexFc1;
unsigned int weatherLabelFc1;
String weatherDesFc1;
double rainLevelFc1;
double snowLevelFc1;
unsigned long sunRiseFc1;
unsigned long sunSetFc1;

unsigned long forecastTimeFc2;
double tempMornFc2;
double tempDayFc2;
double tempEvenFc2;
double tempNightFc2;
double tempMinFc2;
double tempMaxFc2;
unsigned int pressureFc2;
double humidityFc2;
double windSpeedFc2;
unsigned int windDirectionFc2;
byte uvIndexFc2;
unsigned int weatherLabelFc2;
String weatherDesFc2;
double rainLevelFc2;
double snowLevelFc2;
unsigned long sunRiseFc2;
unsigned long sunSetFc2;

unsigned long forecastTimeFc3;
double tempMornFc3;
double tempDayFc3;
double tempEvenFc3;
double tempNightFc3;
double tempMinFc3;
double tempMaxFc3;
unsigned int pressureFc3;
double humidityFc3;
double windSpeedFc3;
unsigned int windDirectionFc3;
byte uvIndexFc3;
unsigned int weatherLabelFc3;
String weatherDesFc3;
double rainLevelFc3;
double snowLevelFc3;
unsigned long sunRiseFc3;
unsigned long sunSetFc3;

unsigned long forecastTimeFc4;
double tempMornFc4;
double tempDayFc4;
double tempEvenFc4;
double tempNightFc4;
double tempMinFc4;
double tempMaxFc4;
unsigned int pressureFc4;
double humidityFc4;
double windSpeedFc4;
unsigned int windDirectionFc4;
byte uvIndexFc4;
unsigned int weatherLabelFc4;
String weatherDesFc4;
double rainLevelFc4;
double snowLevelFc4;
unsigned long sunRiseFc4;
unsigned long sunSetFc4;

unsigned long forecastTimeFc5;
double tempMornFc5;
double tempDayFc5;
double tempEvenFc5;
double tempNightFc5;
double tempMinFc5;
double tempMaxFc5;
unsigned int pressureFc5;
double humidityFc5;
double windSpeedFc5;
unsigned int windDirectionFc5;
byte uvIndexFc5;
unsigned int weatherLabelFc5;
String weatherDesFc5;
double rainLevelFc5;
double snowLevelFc5;
unsigned long sunRiseFc5;
unsigned long sunSetFc5;

unsigned long forecastTimeFc6;
double tempMornFc6;
double tempDayFc6;
double tempEvenFc6;
double tempNightFc6;
double tempMinFc6;
double tempMaxFc6;
unsigned int pressureFc6;
double humidityFc6;
double windSpeedFc6;
unsigned int windDirectionFc6;
byte uvIndexFc6;
unsigned int weatherLabelFc6;
String weatherDesFc6;
double rainLevelFc6;
double snowLevelFc6;
unsigned long sunRiseFc6;
unsigned long sunSetFc6;

unsigned long forecastTimeFc7;
double tempMornFc7;
double tempDayFc7;
double tempEvenFc7;
double tempNightFc7;
double tempMinFc7;
double tempMaxFc7;
unsigned int pressureFc7;
double humidityFc7;
double windSpeedFc7;
unsigned int windDirectionFc7;
byte uvIndexFc7;
unsigned int weatherLabelFc7;
String weatherDesFc7;
double rainLevelFc7;
double snowLevelFc7;
unsigned long sunRiseFc7;
unsigned long sunSetFc7;

// Hourly forecast weather variables.

int temperatureHr1;
int temperatureHr2;
int temperatureHr3;
int temperatureHr4;
int temperatureHr5;
int temperatureHr6;
int temperatureHr7;
int temperatureHr8;

double rainFallHr1;
double rainFallHr2;
double rainFallHr3;
double rainFallHr4;
double rainFallHr5;
double rainFallHr6;
double rainFallHr7;
double rainFallHr8;

unsigned int pressureHr1;
unsigned int pressureHr2;
unsigned int pressureHr3;
unsigned int pressureHr4;
unsigned int pressureHr5;
unsigned int pressureHr6;
unsigned int pressureHr7;
unsigned int pressureHr8;

char* pressureLevel[] = { "Low", "Normal", "High" };

// Graphing.

boolean graph_1 = true;
boolean graph_2 = true;
boolean graph_3 = true;
boolean graph_4 = true;
boolean graph_5 = true;
boolean graph_6 = true;
boolean graph_7 = true;
boolean graph_8 = true;

/*-----------------------------------------------------------------*/

// Initialize WiFi.

bool initWiFi() {

	wiFiTitle();

	// If ESP32 inits successfully in station mode, recolour WiFi to red.

	enableScreen1();

	drawBitmap(tft, WIFI_ICON_Y, WIFI_ICON_X, wiFiRed, WIFI_ICON_W, WIFI_ICON_H);

	disableVSPIScreens();

	// Check if settings are available to connect to WiFi.

	if (ssid == "" || ip == "") {
		Serial.println("Undefined SSID or IP address.");
		return false;
	}

	WiFi.mode(WIFI_STA);
	localIP.fromString(ip.c_str());
	Gateway.fromString(gateway.c_str());
	Subnet.fromString(subnet.c_str());
	dns1.fromString(dns.c_str());

	if (!WiFi.config(localIP, Gateway, Subnet, dns1)) {
		Serial.println("STA Failed to configure");
		return false;
	}

	WiFi.begin(ssid.c_str(), pass.c_str());
	Serial.println("Connecting to WiFi...");

	// If ESP32 inits successfully in station mode, recolour WiFi to amber.

	enableScreen1();

	drawBitmap(tft, WIFI_ICON_Y, WIFI_ICON_X, wiFiAmber, WIFI_ICON_W, WIFI_ICON_H);

	disableVSPIScreens();

	unsigned long currentMillis = millis();
	previousMillis = currentMillis;

	while (WiFi.status() != WL_CONNECTED) {
		currentMillis = millis();
		if (currentMillis - previousMillis >= interval) {
			Serial.println("Failed to connect.");
			// If ESP32 fails to connect, recolour WiFi to red.
			enableScreen1();
			drawBitmap(tft, WIFI_ICON_Y, WIFI_ICON_X, wiFiRed, WIFI_ICON_W, WIFI_ICON_H);
			drawBlackBox();
			disableVSPIScreens();
			return false;
		}

	}

	Serial.println(WiFi.localIP());
	Serial.println("");
	Serial.print("RRSI: ");
	Serial.println(WiFi.RSSI());

	// Update TFT with settings.

	enableScreen1();

	tft.setTextColor(BLACK, WHITE);
	tft.setFont();
	tft.setCursor(23, 50);
	tft.print("WiFi Status: ");
	tft.setTextColor(BLUE);
	tft.setCursor(150, 50);
	tft.print(WiFi.status());
	tft.println("");

	tft.setCursor(23, 65);
	tft.setTextColor(BLACK, WHITE);
	tft.print("SSID: ");
	tft.setTextColor(BLUE);
	tft.setCursor(150, 65);
	tft.print(WiFi.SSID());
	tft.println("");

	tft.setCursor(23, 80);
	tft.setTextColor(BLACK, WHITE);
	tft.print("IP Address: ");
	tft.setTextColor(BLUE);
	tft.setCursor(150, 80);
	tft.print(WiFi.localIP());
	tft.println("");

	tft.setCursor(23, 95);
	tft.setTextColor(BLACK, WHITE);
	tft.print("DNS Address: ");
	tft.setTextColor(BLUE);
	tft.setCursor(150, 95);
	tft.print(WiFi.dnsIP());
	tft.println("");

	tft.setCursor(23, 110);
	tft.setTextColor(BLACK, WHITE);
	tft.print("Gateway Address: ");
	tft.setTextColor(BLUE);
	tft.setCursor(150, 110);
	tft.print(WiFi.gatewayIP());
	tft.println("");

	tft.setCursor(23, 125);
	tft.setTextColor(BLACK, WHITE);
	tft.print("Signal Strenght: ");
	tft.setTextColor(BLUE);
	tft.setCursor(150, 125);
	tft.print(WiFi.RSSI());
	tft.println("");

	tft.setCursor(23, 140);
	tft.setTextColor(BLACK, WHITE);
	tft.print("Time Server: ");
	tft.setTextColor(BLUE);
	tft.setCursor(150, 140);
	tft.print(ntpServer);

	// If ESP32 inits successfully in station mode, recolour WiFi to green.

	drawBitmap(tft, WIFI_ICON_Y, WIFI_ICON_X, wiFiGreen, WIFI_ICON_W, WIFI_ICON_H);

	// Update message to advise unit is starting.

	tft.setFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setTextColor(BLACK);
	tft.println("");
	tft.setCursor(20, 175);
	tft.print("Unit is starting...");
	tft.setFont();

	delay(3000);	// Wait a moment.

	tft.fillScreen(WHITE);

	disableVSPIScreens();

	return true;

} // Close function.

/*-----------------------------------------------------------------*/

// Get data from Open Weather.

String httpGETRequest(const char* serverName) {

	WiFiClient client;
	HTTPClient http;

	// Your Domain name with URL path or IP address with path
	http.begin(client, serverName);

	// Send HTTP POST request
	int httpResponseCode = http.GET();

	String payload = "{}";

	if (httpResponseCode > 0) {
		Serial.print("HTTP Response code: ");
		Serial.println(httpResponseCode);
		payload = http.getString();
	}
	else {
		Serial.print("Error code: ");
		Serial.println(httpResponseCode);
	}
	// Free resources
	http.end();

	return payload;

} // Close function.

/*-----------------------------------------------------------------*/

// Return JSON String from sensor Readings

String getJSONReadings() {

	int tempMaxSpeed;			// This needs editing as it was from PetBit.

	//readings["tempMaxSpeed"] = String(tempMaxSpeed);

	//String jsonString = JSON.stringify(readings);
	//return jsonString;

}  // Close function.

/*-----------------------------------------------------------------*/

// Initialize SPIFFS.

void initSPIFFS() {

	if (!SPIFFS.begin(true)) {

		Serial.println("An error has occurred while mounting SPIFFS");
	}

	else
	{
		Serial.println("SPIFFS mounted successfully");
	}

} // Close function.

/*-----------------------------------------------------------------*/

// Read File from SPIFFS.

String readFile(fs::FS& fs, const char* path) {

	Serial.printf("Reading file: %s\r\n", path);

	File file = fs.open(path);

	if (!file || file.isDirectory()) {

		Serial.println("- failed to open file for reading");
		return String();
	}

	String fileContent;

	while (file.available()) {

		fileContent = file.readStringUntil('\n');
		break;
	}

	return fileContent;

} // Close function.

/*-----------------------------------------------------------------*/

// Write file to SPIFFS.

void writeFile(fs::FS& fs, const char* path, const char* message) {

	Serial.printf("Writing file: %s\r\n", path);

	File file = fs.open(path, FILE_WRITE);

	if (!file) {

		Serial.println("- failed to open file for writing");
		return;
	}

	if (file.print(message)) {

		Serial.println("- file written");
	}

	else
	{
		Serial.println("- write failed");
	}

} // Close function.

/*-----------------------------------------------------------------*/

// Get and print time.

void printLocalTime() {

	struct tm timeinfo;

	if (!getLocalTime(&timeinfo)) {

		Serial.println("Failed, time set to default.");

		// Set time manually.

		rtc.setTime(00, 00, 00, 01, 01, 2021);

		enableScreen1();

		tft.setTextColor(BLACK, WHITE);
		tft.setFont();
		tft.setTextSize(1);
		tft.setCursor(5, 230);
		tft.println("Failed, time set to default.");

		disableVSPIScreens();

		return;
	}

	Serial.println("");
	Serial.println(&timeinfo, "%A, %B %d %Y %H:%M %Z");
	Serial.println("");

	// Text block to over write characters from longer dates when date changes and unit has been running.

	enableScreen1();

	tft.setTextColor(BLACK, WHITE);
	tft.setFont();
	tft.setTextSize(1);
	tft.setCursor(150, 230);
	tft.println("                ");

	// Actual date time to display.

	tft.setTextColor(BLACK, WHITE);
	tft.setFont();
	tft.setTextSize(1);
	tft.setCursor(5, 230);
	tft.println(&timeinfo, "%A, %B %d %Y %H:%M");

	disableVSPIScreens();


} // Close function.

/*-----------------------------------------------------------------*/

void setup() {

	// Enable serial mode.

	Serial.begin(115200);

	// Enable touch screen.

	Serial.println(F(""));
	Serial.println(F("Touch start"));

	ts.begin();

	// Enable I2C Devices.

	Serial.println(F("MCP23008 start"));

	mcp.begin_I2C();

	// Enable BME sensor.

	Serial.println(F("BME280 start"));
	Serial.println(F(""));

	bme280.begin();

	// Set pin modes.

	pinMode(VSPI_CS1, OUTPUT); //VSPI CS
	pinMode(VSPI_CS2, OUTPUT); //VSPI CS
	pinMode(VSPI_CS3, OUTPUT); //VSPI CS
	pinMode(VSPI_CS4, OUTPUT); //VSPI CS
	pinMode(VSPI_CS5, OUTPUT); //VSPI CS
	pinMode(VSPI_CS6, OUTPUT); //VSPI CS
	pinMode(VSPI_CS7, OUTPUT); //VSPI CS
	pinMode(VSPI_CS8, OUTPUT); //VSPI CS
	pinMode(touch_CS, OUTPUT); //Touch CS

	//pinMode(interruptSWITCH1, INPUT_PULLUP); //Interupt
	//pinMode(interruptSWITCH2, INPUT_PULLUP); //Interupt

	// Configure interrupt.

	//attachInterrupt(digitalPinToInterrupt(interruptSWITCH1), displayToggle, FALLING);
	//attachInterrupt(digitalPinToInterrupt(interruptSWITCH2), displayBackLight, FALLING);

	// Reset MCP23008 port driver.

	pinMode(MCP23008_RST, OUTPUT);
	delay(10);
	digitalWrite(MCP23008_RST, LOW);
	delay(10);
	digitalWrite(MCP23008_RST, HIGH);

	// Set MCP23008 outputs.

	mcp.pinMode(TFT_LED1_CTRL, OUTPUT);
	mcp.pinMode(TFT_LED2_CTRL, OUTPUT);
	mcp.pinMode(TFT_LED3_CTRL, OUTPUT);
	mcp.pinMode(TFT_LED4_CTRL, OUTPUT);
	mcp.pinMode(TFT_LED5_CTRL, OUTPUT);
	mcp.pinMode(TFT_LED6_CTRL, OUTPUT);
	mcp.pinMode(TFT_LED7_CTRL, OUTPUT);
	mcp.pinMode(TFT_LED8_CTRL, OUTPUT);

	delay(100);

	// Reset MCP23008 lines.

	mcp.digitalWrite(TFT_LED1_CTRL, LOW);
	mcp.digitalWrite(TFT_LED2_CTRL, LOW);
	mcp.digitalWrite(TFT_LED3_CTRL, LOW);
	mcp.digitalWrite(TFT_LED4_CTRL, LOW);
	mcp.digitalWrite(TFT_LED5_CTRL, LOW);
	mcp.digitalWrite(TFT_LED6_CTRL, LOW);
	mcp.digitalWrite(TFT_LED7_CTRL, LOW);
	mcp.digitalWrite(TFT_LED8_CTRL, LOW);

	// Enable all TFT back lights.

	delay(200);
	mcp.digitalWrite(TFT_LED1_CTRL, HIGH);
	delay(200);
	mcp.digitalWrite(TFT_LED2_CTRL, HIGH);
	delay(200);
	mcp.digitalWrite(TFT_LED3_CTRL, HIGH);
	delay(200);
	mcp.digitalWrite(TFT_LED4_CTRL, HIGH);
	delay(200);
	mcp.digitalWrite(TFT_LED5_CTRL, HIGH);
	delay(200);
	mcp.digitalWrite(TFT_LED6_CTRL, HIGH);
	delay(200);
	mcp.digitalWrite(TFT_LED7_CTRL, HIGH);
	delay(200);
	mcp.digitalWrite(TFT_LED8_CTRL, HIGH);

	// Set all SPI chip selects to HIGH to stablise SPI bus.

	disableVSPIScreens();

	delay(10);

	// Set all tft1 chip select outputs low to configure all displays the same using tft.begin.

	digitalWrite(VSPI_CS1, LOW);
	digitalWrite(VSPI_CS2, LOW);
	digitalWrite(VSPI_CS3, LOW);
	digitalWrite(VSPI_CS4, LOW);
	digitalWrite(VSPI_CS5, LOW);
	digitalWrite(VSPI_CS6, LOW);
	digitalWrite(VSPI_CS7, LOW);
	digitalWrite(VSPI_CS8, LOW);
	digitalWrite(touch_CS, LOW);

	delay(10);

	digitalWrite(touch_CS, HIGH);

	// Send screen configuration.

	//tft1.begin();
	tft.begin(40000000); // 40000000 27000000
	tft.setRotation(3);
	tft.setCursor(0, 0);

	// Reset all chip selects high once again.

	disableVSPIScreens();

	// Start touch sensor.

	digitalWrite(touch_CS, HIGH);
	delay(10);

	// Start up screen image and title.

	enableScreen1();
	tft.fillScreen(WHITE);
	disableVSPIScreens();

	enableScreen2();
	tft.fillScreen(WHITE);
	disableVSPIScreens();

	enableScreen3();
	tft.fillScreen(WHITE);
	disableVSPIScreens();

	enableScreen4();
	tft.fillScreen(WHITE);
	disableVSPIScreens();

	enableScreen5();
	tft.fillScreen(WHITE);
	disableVSPIScreens();

	enableScreen6();
	tft.fillScreen(WHITE);
	disableVSPIScreens();

	enableScreen7();
	tft.fillScreen(WHITE);
	disableVSPIScreens();

	enableScreen8();
	tft.fillScreen(WHITE);
	disableVSPIScreens();

	enableScreen1();
	drawBitmap(tft, 48, 96, rainbow, 128, 128);

	tft.setFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setTextColor(BLACK);
	tft.setCursor(96, 200);
	tft.println("Weather Station");
	tft.setFont();
	tft.setTextSize(1);
	tft.setCursor(98, 210);
	tft.println("by Christopher Cooper");

	disableVSPIScreens();

	enableScreen2();
	drawBitmap(tft, 48, 96, rainbow, 128, 128);
	disableVSPIScreens();

	enableScreen3();
	drawBitmap(tft, 48, 96, rainbow, 128, 128);
	disableVSPIScreens();

	enableScreen4();
	drawBitmap(tft, 48, 96, rainbow, 128, 128);
	disableVSPIScreens();

	enableScreen5();
	drawBitmap(tft, 48, 96, rainbow, 128, 128);
	disableVSPIScreens();

	enableScreen6();
	drawBitmap(tft, 48, 96, rainbow, 128, 128);
	disableVSPIScreens();

	enableScreen7();
	drawBitmap(tft, 48, 96, rainbow, 128, 128);
	disableVSPIScreens();

	enableScreen8();
	drawBitmap(tft, 48, 96, rainbow, 128, 128);
	disableVSPIScreens();

	delay(2000);

	// Clear screens.

	enableScreen1();
	tft.fillScreen(WHITE);
	disableVSPIScreens();

	mcp.digitalWrite(TFT_LED2_CTRL, LOW);
	mcp.digitalWrite(TFT_LED3_CTRL, LOW);
	mcp.digitalWrite(TFT_LED4_CTRL, LOW);
	mcp.digitalWrite(TFT_LED5_CTRL, LOW);
	mcp.digitalWrite(TFT_LED6_CTRL, LOW);
	mcp.digitalWrite(TFT_LED7_CTRL, LOW);
	mcp.digitalWrite(TFT_LED8_CTRL, LOW);

	enableScreen2();
	tft.fillScreen(BLACK);
	disableVSPIScreens();

	enableScreen3();
	tft.fillScreen(BLACK);
	disableVSPIScreens();

	enableScreen4();
	tft.fillScreen(BLACK);
	disableVSPIScreens();

	enableScreen5();
	tft.fillScreen(BLACK);
	disableVSPIScreens();

	enableScreen6();
	tft.fillScreen(BLACK);
	disableVSPIScreens();

	enableScreen7();
	tft.fillScreen(BLACK);
	disableVSPIScreens();

	enableScreen8();
	tft.fillScreen(BLACK);
	disableVSPIScreens();

	// Initialize SPIFFS.

	initSPIFFS();

	// Load values saved in SPIFFS.

	ssid = "BT-7FA3K5";									// Remove these lines before final build.
	pass = "iKD94Y3K4Qvkck";
	ip = "192.168.1.200";
	subnet = "255.255.255.0";
	gateway = "192.168.1.254";
	dns = "192.168.1.254";

	writeFile(SPIFFS, ssidPath, ssid.c_str());
	writeFile(SPIFFS, passPath, pass.c_str());
	writeFile(SPIFFS, ipPath, ip.c_str());
	writeFile(SPIFFS, subnetPath, subnet.c_str());
	writeFile(SPIFFS, gatewayPath, gateway.c_str());
	writeFile(SPIFFS, dnsPath, dns.c_str());

	Serial.println();
	ssid = readFile(SPIFFS, ssidPath);
	pass = readFile(SPIFFS, passPath);
	ip = readFile(SPIFFS, ipPath);
	subnet = readFile(SPIFFS, subnetPath);
	gateway = readFile(SPIFFS, gatewayPath);
	dns = readFile(SPIFFS, dnsPath);

	Serial.println();
	Serial.println(ssid);
	Serial.println(pass);
	Serial.println(ip);
	Serial.println(subnet);
	Serial.println(gateway);
	Serial.println(dns);
	Serial.println();

	if (initWiFi()) {

		// Handle the Web Server in Station Mode and route for root / web page.

		server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {

			request->send(SPIFFS, "/index.html", "text/html");
			});

		server.serveStatic("/", SPIFFS, "/");

		// Request for the latest data readings.

		server.on("/readings", HTTP_GET, [](AsyncWebServerRequest* request) {

			String json = getJSONReadings();
			request->send(200, "application/json", json);
			json = String();
			});

		events.onConnect([](AsyncEventSourceClient* client) {

			if (client->lastId()) {
				Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
			}
			});

		server.addHandler(&events);

		server.begin();
	}

	else

	{
		apMode = true;	// Set variable to be true so void loop is by passed and doesnt run until false.

		// WiFi title page.

		wiFiTitle();

		// Initialize the ESP32 in Access Point mode, recolour to WiFI red.

		enableScreen1();

		drawBitmap(tft, WIFI_ICON_Y, WIFI_ICON_X, wiFiRed, WIFI_ICON_W, WIFI_ICON_H);

		disableVSPIScreens();

		// Set Access Point.
		Serial.println("Setting AP (Access Point)");

		// NULL sets an open Access Point.

		WiFi.softAP("WIFI-MANAGER", NULL);

		IPAddress IP = WiFi.softAPIP();
		Serial.print("AP IP address: ");
		Serial.println(IP);

		// Web Server Root URL For WiFi Manager Web Page.

		server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
			request->send(SPIFFS, "/wifimanager.html", "text/html");
			});

		server.serveStatic("/", SPIFFS, "/");

		// Get the parameters submited on the form.

		server.on("/", HTTP_POST, [](AsyncWebServerRequest* request) {
			int params = request->params();
			for (int i = 0; i < params; i++) {
				AsyncWebParameter* p = request->getParam(i);
				if (p->isPost()) {
					// HTTP POST ssid value
					if (p->name() == PARAM_INPUT_1) {
						ssid = p->value().c_str();
						Serial.print("SSID set to: ");
						Serial.println(ssid);
						// Write file to save value
						writeFile(SPIFFS, ssidPath, ssid.c_str());
					}
					// HTTP POST pass value
					if (p->name() == PARAM_INPUT_2) {
						pass = p->value().c_str();
						Serial.print("Password set to: ");
						Serial.println(pass);
						// Write file to save value
						writeFile(SPIFFS, passPath, pass.c_str());
					}
					// HTTP POST ip value
					if (p->name() == PARAM_INPUT_3) {
						ip = p->value().c_str();
						Serial.print("IP Address set to: ");
						Serial.println(ip);
						// Write file to save value
						writeFile(SPIFFS, ipPath, ip.c_str());
					}
					// HTTP POST ip value
					if (p->name() == PARAM_INPUT_4) {
						subnet = p->value().c_str();
						Serial.print("Subnet Address: ");
						Serial.println(subnet);
						// Write file to save value
						writeFile(SPIFFS, subnetPath, subnet.c_str());
					}
					// HTTP POST ip value
					if (p->name() == PARAM_INPUT_5) {
						gateway = p->value().c_str();
						Serial.print("Gateway set to: ");
						Serial.println(gateway);
						// Write file to save value
						writeFile(SPIFFS, gatewayPath, gateway.c_str());
					}
					// HTTP POST ip value
					if (p->name() == PARAM_INPUT_6) {
						dns = p->value().c_str();
						Serial.print("DNS Address set to: ");
						Serial.println(dns);
						// Write file to save value
						writeFile(SPIFFS, dnsPath, dns.c_str());
					}
					//Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
				}
			}

			request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
			delay(3000);

			// After saving the parameters, restart the ESP32

			ESP.restart();
			});

		server.begin();

		enableScreen1();

		tft.fillRect(39, 60, 183, 109, RED);
		tft.drawRect(38, 59, 185, 111, WHITE);
		tft.drawRect(37, 58, 187, 113, WHITE);
		tft.setFont(&FreeSans9pt7b);
		tft.setTextSize(1);
		tft.setTextColor(WHITE);
		tft.setCursor(50, 78);
		tft.print("Access Point Mode");

		tft.setFont();
		tft.setTextColor(WHITE);
		tft.setCursor(50, 90);
		tft.print("Could not connect to WiFi");
		tft.setCursor(50, 106);
		tft.print("1) Using your mobile phone");
		tft.setCursor(50, 118);
		tft.print("2) Connect to WiFI Manager");
		tft.setCursor(50, 130);
		tft.print("3) Browse to 192.168.4.1");
		tft.setCursor(50, 142);
		tft.print("4) Enter network settings");
		tft.setCursor(50, 154);
		tft.print("5) Unit will then restart");

		unsigned long previousMillis = millis();
		unsigned long interval = 120000;

		while (1) {

			// Hold from starting loop while in AP mode.

			unsigned long currentMillis = millis();

			// Restart after 2 minutes in case of failed reconnection with correc WiFi details.

			if (currentMillis - previousMillis >= interval) {

				ESP.restart();

			}

		}

	}

	disableVSPIScreens();

	enableScreen1();

	// Draw border and buttons at start.

	drawBorder();

	for (int x = 20; x < 320; x = x + 20) {

		tft.drawFastVLine(x, 0, 240, BLACK);
	}

	for (int x = 20; x < 240; x = x + 20) {

		tft.drawFastHLine(0, x, 320, BLACK);
	}

	disableVSPIScreens();

	mcp.digitalWrite(TFT_LED2_CTRL, HIGH);
	mcp.digitalWrite(TFT_LED3_CTRL, HIGH);
	mcp.digitalWrite(TFT_LED4_CTRL, HIGH);
	mcp.digitalWrite(TFT_LED5_CTRL, HIGH);
	mcp.digitalWrite(TFT_LED6_CTRL, HIGH);
	mcp.digitalWrite(TFT_LED7_CTRL, HIGH);
	mcp.digitalWrite(TFT_LED8_CTRL, HIGH);

	enableScreen2();
	tft.fillScreen(WHITE);
	disableVSPIScreens();

	enableScreen3();
	tft.fillScreen(WHITE);
	disableVSPIScreens();

	enableScreen4();
	tft.fillScreen(WHITE);
	disableVSPIScreens();

	enableScreen5();
	tft.fillScreen(WHITE);
	disableVSPIScreens();

	enableScreen6();
	tft.fillScreen(WHITE);
	disableVSPIScreens();

	enableScreen7();
	tft.fillScreen(WHITE);
	disableVSPIScreens();

	enableScreen8();
	tft.fillScreen(WHITE);
	disableVSPIScreens();

	// Get weather data.

	getWeatherData();

	// Initialize time and get the time.

	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
	printLocalTime();
	LocalTime = millis();

	// Layout screens & obtain data.

	currentWeatherLayout();

	forecastWeatherLayoutDayX(tft, 2);
	forecastWeatherLayoutDayX(tft, 3);
	forecastWeatherLayoutDayX(tft, 4);

	forecastWeatherLayoutDayX(tft, 5);
	forecastWeatherLayoutDayX(tft, 6);
	forecastWeatherLayoutDayX(tft, 7);
	forecastWeatherLayoutDayX(tft, 8);

	currentWeatherTemp();
	currentWeatherDataDisplay();

	forecastDataDisplayTempDayX(tft, 2, forecastTimeFc1, weatherDesFc1, tempDayFc1, tempNightFc1, tempMinFc1, tempMaxFc1);
	forecastDataDisplayDayX(tft, 2, forecastTimeFc1, pressureFc1, humidityFc1, windSpeedFc1, windDirectionFc1, uvIndexFc1, weatherLabelFc1, rainLevelFc1, sunRiseFc1, sunSetFc1);

	forecastDataDisplayTempDayX(tft, 3, forecastTimeFc2, weatherDesFc2, tempDayFc2, tempNightFc2, tempMinFc2, tempMaxFc2);
	forecastDataDisplayDayX(tft, 3, forecastTimeFc2, pressureFc2, humidityFc2, windSpeedFc2, windDirectionFc2, uvIndexFc2, weatherLabelFc2, rainLevelFc2, sunRiseFc2, sunSetFc2);

	forecastDataDisplayTempDayX(tft, 4, forecastTimeFc3, weatherDesFc3, tempDayFc3, tempNightFc3, tempMinFc3, tempMaxFc3);
	forecastDataDisplayDayX(tft, 4, forecastTimeFc3, pressureFc3, humidityFc3, windSpeedFc3, windDirectionFc3, uvIndexFc3, weatherLabelFc3, rainLevelFc3, sunRiseFc3, sunSetFc3);

	forecastDataDisplayTempDayX(tft, 5, forecastTimeFc4, weatherDesFc4, tempDayFc4, tempNightFc4, tempMinFc4, tempMaxFc4);
	forecastDataDisplayDayX(tft, 5, forecastTimeFc4, pressureFc4, humidityFc4, windSpeedFc4, windDirectionFc4, uvIndexFc4, weatherLabelFc4, rainLevelFc4, sunRiseFc4, sunSetFc4);

	forecastDataDisplayTempDayX(tft, 6, forecastTimeFc5, weatherDesFc5, tempDayFc5, tempNightFc5, tempMinFc5, tempMaxFc5);
	forecastDataDisplayDayX(tft, 6, forecastTimeFc5, pressureFc5, humidityFc5, windSpeedFc5, windDirectionFc5, uvIndexFc5, weatherLabelFc5, rainLevelFc5, sunRiseFc5, sunSetFc5);

	forecastDataDisplayTempDayX(tft, 7, forecastTimeFc6, weatherDesFc6, tempDayFc6, tempNightFc6, tempMinFc6, tempMaxFc6);
	forecastDataDisplayDayX(tft, 7, forecastTimeFc6, pressureFc6, humidityFc6, windSpeedFc6, windDirectionFc6, uvIndexFc6, weatherLabelFc6, rainLevelFc6, sunRiseFc6, sunSetFc6);

	forecastDataDisplayTempDayX(tft, 8, forecastTimeFc7, weatherDesFc7, tempDayFc7, tempNightFc7, tempMinFc7, tempMaxFc7);
	forecastDataDisplayDayX(tft, 8, forecastTimeFc7, pressureFc7, humidityFc7, windSpeedFc7, windDirectionFc7, uvIndexFc7, weatherLabelFc7, rainLevelFc7, sunRiseFc7, sunSetFc7);

} // Close setup.

/*-----------------------------------------------------------------*/

ICACHE_RAM_ATTR void displayToggle() {

	static unsigned long  last_interrupt_time = 0;                  // Function to solve debounce
	unsigned long         interrupt_time = millis();

	if (interrupt_time - last_interrupt_time > 150)
	{
		if (switchOneState == false) {

			switchOneState = true;
			switchOneToggled = true;
			intervalTTime = 0;
		}

		else if (switchOneState == true) {

			switchOneState = false;
			switchOneToggled = true;
			intervalTTime = 0;
		}
	}

	last_interrupt_time = interrupt_time;

} // Close function.

/*-----------------------------------------------------------------*/

ICACHE_RAM_ATTR void displayBackLight() {

	static unsigned long  last_interrupt_time = 0;                  // Function to solve debounce
	unsigned long         interrupt_time = millis();

	if (interrupt_time - last_interrupt_time > 150)
	{
		if (switchTwoState == false) {

			switchTwoState = true;
			switchTwoToggled = true;
		}

		else if (switchTwoState == true) {

			switchTwoState = false;
			switchTwoToggled = true;

		}
	}

	last_interrupt_time = interrupt_time;

} // Close function.

/*-----------------------------------------------------------------*/

void loop() {

	if (ts.touched()) {
		TS_Point p = ts.getPoint();
		Serial.print("Pressure = ");
		Serial.print(p.z);
		Serial.print(", x = ");
		Serial.print(p.x);
		Serial.print(", y = ");
		Serial.print(p.y);
		delay(30);
		Serial.println();

		static unsigned long  last_interrupt_time = 0;                  // This needs to be changed as it was pinched
		unsigned long         interrupt_time = millis();

		if (interrupt_time - last_interrupt_time > 500)
		{
			if (switchOneState == false) {

				switchOneState = true;
				switchOneToggled = true;
				intervalTTime = 0;
			}

			else if (switchOneState == true) {

				switchOneState = false;
				switchOneToggled = true;
				intervalTTime = 0;
			}
		}

	}

	// Check Open Weather at regular intervals.

	if (millis() >= intervalT + intervalTime) {

		// Disconnect Interrupt

		//detachInterrupt(interruptSWITCH1);

		// Update time.

		printLocalTime();

		// Update weather data.

		getWeatherData();

		// Update current weather display.

		currentWeatherTemp();
		currentWeatherDataDisplay();

		// Update forecast weather displays.

		forecastDataDisplayTempDayX(tft, 2, forecastTimeFc1, weatherDesFc1, tempDayFc1, tempNightFc1, tempMinFc1, tempMaxFc1);
		forecastDataDisplayDayX(tft, 2, forecastTimeFc1, pressureFc1, humidityFc1, windSpeedFc1, windDirectionFc1, uvIndexFc1, weatherLabelFc1, rainLevelFc1, sunRiseFc1, sunSetFc1);

		forecastDataDisplayTempDayX(tft, 3, forecastTimeFc2, weatherDesFc2, tempDayFc2, tempNightFc2, tempMinFc2, tempMaxFc2);
		forecastDataDisplayDayX(tft, 3, forecastTimeFc2, pressureFc2, humidityFc2, windSpeedFc2, windDirectionFc2, uvIndexFc2, weatherLabelFc2, rainLevelFc2, sunRiseFc2, sunSetFc2);

		forecastDataDisplayTempDayX(tft, 4, forecastTimeFc3, weatherDesFc3, tempDayFc3, tempNightFc3, tempMinFc3, tempMaxFc3);
		forecastDataDisplayDayX(tft, 4, forecastTimeFc3, pressureFc3, humidityFc3, windSpeedFc3, windDirectionFc3, uvIndexFc3, weatherLabelFc3, rainLevelFc3, sunRiseFc3, sunSetFc3);

		forecastDataDisplayTempDayX(tft, 5, forecastTimeFc4, weatherDesFc4, tempDayFc4, tempNightFc4, tempMinFc4, tempMaxFc4);
		forecastDataDisplayDayX(tft, 5, forecastTimeFc4, pressureFc4, humidityFc4, windSpeedFc4, windDirectionFc4, uvIndexFc4, weatherLabelFc4, rainLevelFc4, sunRiseFc4, sunSetFc4);

		forecastDataDisplayTempDayX(tft, 6, forecastTimeFc5, weatherDesFc5, tempDayFc5, tempNightFc5, tempMinFc5, tempMaxFc5);
		forecastDataDisplayDayX(tft, 6, forecastTimeFc5, pressureFc5, humidityFc5, windSpeedFc5, windDirectionFc5, uvIndexFc5, weatherLabelFc5, rainLevelFc5, sunRiseFc5, sunSetFc5);

		forecastDataDisplayTempDayX(tft, 7, forecastTimeFc6, weatherDesFc6, tempDayFc6, tempNightFc6, tempMinFc6, tempMaxFc6);
		forecastDataDisplayDayX(tft, 7, forecastTimeFc6, pressureFc6, humidityFc6, windSpeedFc6, windDirectionFc6, uvIndexFc6, weatherLabelFc6, rainLevelFc6, sunRiseFc6, sunSetFc6);

		forecastDataDisplayTempDayX(tft, 8, forecastTimeFc7, weatherDesFc7, tempDayFc7, tempNightFc7, tempMinFc7, tempMaxFc7);
		forecastDataDisplayDayX(tft, 8, forecastTimeFc7, pressureFc7, humidityFc7, windSpeedFc7, windDirectionFc7, uvIndexFc7, weatherLabelFc7, rainLevelFc7, sunRiseFc7, sunSetFc7);

		intervalTime = 60000; // After restart, once Setup has loaded, lenghten update interval to 5 mins (this needs changing when build is 100%)
		intervalT = millis();

		//attachInterrupt(digitalPinToInterrupt(interruptSWITCH1), displayToggle, FALLING);

	}

	if (millis() >= intervalTT + intervalTTime) {

		// Disconnect Interrupt

		//detachInterrupt(interruptSWITCH1);

		// Alternate temperature displays, min, max, day, night.

		if (flagTempDisplayChange == false) {

			flagTempDisplayChange = true;
			currentWeatherTemp();

			if (switchOneState == false) {

				if (switchOneToggled == true) {

					enableScreen5();
					tft.fillScreen(WHITE);
					forecastWeatherLayoutDayX(tft, 5);
					forecastDataDisplayDayX(tft, 5, forecastTimeFc4, pressureFc4, humidityFc4, windSpeedFc4, windDirectionFc4, uvIndexFc4, weatherLabelFc4, rainLevelFc4, sunRiseFc4, sunSetFc4);

					enableScreen6();
					tft.fillScreen(WHITE);
					forecastWeatherLayoutDayX(tft, 6);
					forecastDataDisplayDayX(tft, 6, forecastTimeFc5, pressureFc5, humidityFc5, windSpeedFc5, windDirectionFc5, uvIndexFc5, weatherLabelFc5, rainLevelFc5, sunRiseFc5, sunSetFc5);

					enableScreen7();
					tft.fillScreen(WHITE);
					forecastWeatherLayoutDayX(tft, 7);
					forecastDataDisplayDayX(tft, 7, forecastTimeFc6, pressureFc6, humidityFc6, windSpeedFc6, windDirectionFc6, uvIndexFc6, weatherLabelFc6, rainLevelFc6, sunRiseFc6, sunSetFc6);

					enableScreen8();
					tft.fillScreen(WHITE);
					forecastWeatherLayoutDayX(tft, 8);
					forecastDataDisplayDayX(tft, 8, forecastTimeFc7, pressureFc7, humidityFc7, windSpeedFc7, windDirectionFc7, uvIndexFc7, weatherLabelFc7, rainLevelFc7, sunRiseFc7, sunSetFc7);

					switchOneToggled = false;
				}

				forecastDataDisplayTempDayX(tft, 5, forecastTimeFc4, weatherDesFc4, tempDayFc4, tempNightFc4, tempMinFc4, tempMaxFc4);
				forecastDataDisplayTempDayX(tft, 6, forecastTimeFc5, weatherDesFc5, tempDayFc5, tempNightFc5, tempMinFc5, tempMaxFc5);
				forecastDataDisplayTempDayX(tft, 7, forecastTimeFc6, weatherDesFc6, tempDayFc6, tempNightFc6, tempMinFc6, tempMaxFc6);
				forecastDataDisplayTempDayX(tft, 8, forecastTimeFc7, weatherDesFc7, tempDayFc7, tempNightFc7, tempMinFc7, tempMaxFc7);
			}

			forecastDataDisplayTempDayX(tft, 2, forecastTimeFc1, weatherDesFc1, tempDayFc1, tempNightFc1, tempMinFc1, tempMaxFc1);
			forecastDataDisplayTempDayX(tft, 3, forecastTimeFc2, weatherDesFc2, tempDayFc2, tempNightFc2, tempMinFc2, tempMaxFc2);
			forecastDataDisplayTempDayX(tft, 4, forecastTimeFc3, weatherDesFc3, tempDayFc3, tempNightFc3, tempMinFc3, tempMaxFc3);

		}

		else if (flagTempDisplayChange == true) {

			flagTempDisplayChange = false;
			currentWeatherTemp();

			if (switchOneState == false) {

				if (switchOneToggled == true) {

					enableScreen5();
					tft.fillScreen(WHITE);
					forecastWeatherLayoutDayX(tft, 5);
					forecastDataDisplayDayX(tft, 5, forecastTimeFc4, pressureFc4, humidityFc4, windSpeedFc4, windDirectionFc4, uvIndexFc4, weatherLabelFc4, rainLevelFc4, sunRiseFc4, sunSetFc4);

					enableScreen6();
					tft.fillScreen(WHITE);
					forecastWeatherLayoutDayX(tft, 6);
					forecastDataDisplayDayX(tft, 6, forecastTimeFc5, pressureFc5, humidityFc5, windSpeedFc5, windDirectionFc5, uvIndexFc5, weatherLabelFc5, rainLevelFc5, sunRiseFc5, sunSetFc5);

					enableScreen7();
					tft.fillScreen(WHITE);
					forecastWeatherLayoutDayX(tft, 7);
					forecastDataDisplayDayX(tft, 7, forecastTimeFc6, pressureFc6, humidityFc6, windSpeedFc6, windDirectionFc6, uvIndexFc6, weatherLabelFc6, rainLevelFc6, sunRiseFc6, sunSetFc6);

					enableScreen8();
					tft.fillScreen(WHITE);
					forecastWeatherLayoutDayX(tft, 8);
					forecastDataDisplayDayX(tft, 8, forecastTimeFc7, pressureFc7, humidityFc7, windSpeedFc7, windDirectionFc7, uvIndexFc7, weatherLabelFc7, rainLevelFc7, sunRiseFc7, sunSetFc7);


					switchOneToggled = false;
				}

				forecastDataDisplayTempDayX(tft, 5, forecastTimeFc4, weatherDesFc4, tempDayFc4, tempNightFc4, tempMinFc4, tempMaxFc4);
				forecastDataDisplayTempDayX(tft, 6, forecastTimeFc5, weatherDesFc5, tempDayFc5, tempNightFc5, tempMinFc5, tempMaxFc5);
				forecastDataDisplayTempDayX(tft, 7, forecastTimeFc6, weatherDesFc6, tempDayFc6, tempNightFc6, tempMinFc6, tempMaxFc6);
				forecastDataDisplayTempDayX(tft, 8, forecastTimeFc7, weatherDesFc7, tempDayFc7, tempNightFc7, tempMinFc7, tempMaxFc7);
			}

			forecastDataDisplayTempDayX(tft, 2, forecastTimeFc1, weatherDesFc1, tempDayFc1, tempNightFc1, tempMinFc1, tempMaxFc1);
			forecastDataDisplayTempDayX(tft, 3, forecastTimeFc2, weatherDesFc2, tempDayFc2, tempNightFc2, tempMinFc2, tempMaxFc2);
			forecastDataDisplayTempDayX(tft, 4, forecastTimeFc3, weatherDesFc3, tempDayFc3, tempNightFc3, tempMinFc3, tempMaxFc3);

		}

		intervalTTime = 3000; // After restart, once Setup has loaded, lenghten update interval to 5 mins (this needs changing when build is 100%)
		intervalTT = millis();

		//attachInterrupt(digitalPinToInterrupt(interruptSWITCH1), displayToggle, FALLING);

	}

	if (switchOneState == true) {

		if (switchOneToggled == true) {

			// Disconnect Interrupt

			//detachInterrupt(interruptSWITCH1);

			enableScreen5();
			tft.fillScreen(WHITE);
			drawHourlyTempChart(tft);
			disableVSPIScreens();

			enableScreen6();
			tft.fillScreen(WHITE);
			drawHourlyRainChart(tft);
			disableVSPIScreens();

			enableScreen7();
			tft.fillScreen(WHITE);
			drawHourlyPressureChart(tft);
			disableVSPIScreens();

			enableScreen8();
			tft.fillScreen(WHITE);
			drawCompassChart(tft);
			disableVSPIScreens();

			switchOneToggled = false;

			//attachInterrupt(digitalPinToInterrupt(interruptSWITCH1), displayToggle, FALLING);

		}

	}

	if (switchTwoState == true) {

		if (switchTwoToggled == true) {

			//detachInterrupt(interruptSWITCH2);

			mcp.digitalWrite(0, HIGH);
			mcp.digitalWrite(1, HIGH);
			mcp.digitalWrite(2, HIGH);
			mcp.digitalWrite(3, HIGH);
			mcp.digitalWrite(4, HIGH);
			mcp.digitalWrite(5, HIGH);
			mcp.digitalWrite(6, HIGH);
			mcp.digitalWrite(7, HIGH);

			switchTwoToggled = false;

			//attachInterrupt(digitalPinToInterrupt(interruptSWITCH2), displayBackLight, FALLING);

		}

	}

	if (switchTwoState == false) {

		if (switchTwoToggled == true) {

			//detachInterrupt(interruptSWITCH2);

			mcp.digitalWrite(0, LOW);
			mcp.digitalWrite(1, LOW);
			mcp.digitalWrite(2, LOW);
			mcp.digitalWrite(3, LOW);
			mcp.digitalWrite(4, LOW);
			mcp.digitalWrite(5, LOW);
			mcp.digitalWrite(6, LOW);
			mcp.digitalWrite(7, LOW);

			switchTwoToggled = false;

			//attachInterrupt(digitalPinToInterrupt(interruptSWITCH2), displayBackLight, FALLING);

		}

	}

} // Close loop.

/*-----------------------------------------------------------------*/

// Get forecast data.

void getWeatherData() {

	// Open Weather Request.

	String serverPath = "http://api.openweathermap.org/data/2.5/onecall?lat=" + latitude + "&lon=" + longitude + "&APPID=" + openWeatherMapApiKey + "&units=metric" + "&exclude=minutely,alerts";  //,hourly

	jsonBuffer = httpGETRequest(serverPath.c_str());
	Serial.println("");
	Serial.println(jsonBuffer);
	deserializeJson(weatherData, jsonBuffer);
	JsonObject myObject = weatherData.as<JsonObject>();

	// Parse current weather data from Json.

	currentWeatherTime = (myObject["current"]["dt"]);
	daylightOffset_sec = (myObject["timezone_offset"]);
	tempNow = (myObject["current"]["temp"]);
	feelsLikeTempNow = (myObject["current"]["feels_like"]);
	pressureNow = (myObject["current"]["pressure"]);
	humidityNow = (myObject["current"]["humidity"]);
	windSpeedNow = (myObject["current"]["wind_speed"]);
	windDirectionNow = (myObject["current"]["wind_deg"]);
	uvIndexNow = (myObject["current"]["uvi"]);
	currentWeatherNow = (myObject["current"]["weather"][0]["id"]);
	rainLevelNow = (myObject["current"]["rain"]["1h"]);
	snowLevelNow = (myObject["current"]["snow"]);
	sunRiseNow = (myObject["current"]["sunrise"]);
	sunSetNow = (myObject["current"]["sunset"]);

	weatherDesCurrent.clear();
	serializeJson((myObject["current"]["weather"][0]["description"]), weatherDesCurrent);
	weatherDesCurrent.replace('"', ' ');
	weatherDesCurrent.remove(0, 1);
	weatherDesCurrent[0] = toupper(weatherDesCurrent[0]);

	Serial.println(F(""));
	Serial.print(F("Json Time Zone Offset       : "));
	Serial.println(daylightOffset_sec);
	Serial.print(F("Json Variable Current Epoch : "));
	Serial.println(currentWeatherTime);
	Serial.print(F("Json Variable Weather ID    : "));
	Serial.println(currentWeatherNow);
	Serial.print(F("Json Variable Temperature   : "));
	Serial.println(tempNow);
	Serial.print(F("Json Variable Feels Like    : "));
	Serial.println(feelsLikeTempNow);
	Serial.print(F("Json Variable Pressure      : "));
	Serial.println(pressureNow);
	Serial.print(F("Json Variable Humidity      : "));
	Serial.println(humidityNow);
	Serial.print(F("Json Variable Wind Speed    : "));
	Serial.println(windSpeedNow);
	Serial.print(F("Json Variable Wind Direction: "));
	Serial.println(windDirectionNow);
	Serial.print(F("Json Variable UV Index      : "));
	Serial.println(uvIndexNow);
	Serial.print(F("Json Variable Rain Level    : "));
	Serial.println(rainLevelNow);
	Serial.print(F("Json Variable Snow Level    : "));
	Serial.println(snowLevelNow);
	Serial.print(F("Json Variable Sunrise       : "));
	Serial.println(sunRiseNow);
	Serial.print(F("Json Variable Sunset        : "));
	Serial.println(sunSetNow);
	Serial.println(F(""));

	// Parse hourly forecasr weather data from Jason.

	temperatureHr1 = (myObject["hourly"][1]["temp"]);
	temperatureHr2 = (myObject["hourly"][2]["temp"]);
	temperatureHr3 = (myObject["hourly"][3]["temp"]);
	temperatureHr4 = (myObject["hourly"][4]["temp"]);
	temperatureHr5 = (myObject["hourly"][5]["temp"]);
	temperatureHr6 = (myObject["hourly"][6]["temp"]);
	temperatureHr7 = (myObject["hourly"][7]["temp"]);
	temperatureHr8 = (myObject["hourly"][8]["temp"]);

	rainFallHr1 = (myObject["hourly"][1]["rain"]["1h"]);
	rainFallHr2 = (myObject["hourly"][2]["rain"]["1h"]);
	rainFallHr3 = (myObject["hourly"][3]["rain"]["1h"]);
	rainFallHr4 = (myObject["hourly"][4]["rain"]["1h"]);
	rainFallHr5 = (myObject["hourly"][5]["rain"]["1h"]);
	rainFallHr6 = (myObject["hourly"][6]["rain"]["1h"]);
	rainFallHr7 = (myObject["hourly"][7]["rain"]["1h"]);
	rainFallHr8 = (myObject["hourly"][8]["rain"]["1h"]);

	pressureHr1 = (myObject["hourly"][1]["pressure"]);
	pressureHr2 = (myObject["hourly"][2]["pressure"]);
	pressureHr3 = (myObject["hourly"][3]["pressure"]);
	pressureHr4 = (myObject["hourly"][4]["pressure"]);
	pressureHr5 = (myObject["hourly"][5]["pressure"]);
	pressureHr6 = (myObject["hourly"][6]["pressure"]);
	pressureHr7 = (myObject["hourly"][7]["pressure"]);
	pressureHr8 = (myObject["hourly"][8]["pressure"]);

	// Parse forecast weather data from Json.

	forecastTimeFc1 = (myObject["daily"][1]["dt"]);
	tempMornFc1 = (myObject["daily"][1]["temp"]["morn"]);
	tempDayFc1 = (myObject["daily"][1]["temp"]["day"]);
	tempEvenFc1 = (myObject["daily"][1]["temp"]["eve"]);
	tempNightFc1 = (myObject["daily"][1]["temp"]["night"]);
	tempMinFc1 = (myObject["daily"][1]["temp"]["min"]);
	tempMaxFc1 = (myObject["daily"][1]["temp"]["max"]);
	pressureFc1 = (myObject["daily"][1]["pressure"]);
	humidityFc1 = (myObject["daily"][1]["humidity"]);
	windSpeedFc1 = (myObject["daily"][1]["wind_speed"]);
	windDirectionFc1 = (myObject["daily"][1]["wind_deg"]);
	uvIndexFc1 = (myObject["daily"][1]["uvi"]);
	weatherLabelFc1 = (myObject["daily"][1]["weather"][0]["id"]);
	rainLevelFc1 = (myObject["daily"][1]["rain"]);
	snowLevelFc1 = (myObject["daily"][1]["snow"]);
	sunRiseFc1 = (myObject["daily"][1]["sunrise"]);
	sunSetFc1 = (myObject["daily"][1]["sunset"]);

	weatherDesFc1.clear();
	serializeJson((myObject["daily"][1]["weather"][0]["description"]), weatherDesFc1);
	weatherDesFc1.replace('"', ' ');
	weatherDesFc1.remove(0, 1);
	weatherDesFc1[0] = toupper(weatherDesFc1[0]);

	forecastTimeFc2 = (myObject["daily"][2]["dt"]);
	tempMornFc2 = (myObject["daily"][2]["temp"]["morn"]);
	tempDayFc2 = (myObject["daily"][2]["temp"]["day"]);
	tempEvenFc2 = (myObject["daily"][2]["temp"]["eve"]);
	tempNightFc2 = (myObject["daily"][2]["temp"]["night"]);
	tempMinFc2 = (myObject["daily"][2]["temp"]["min"]);
	tempMaxFc2 = (myObject["daily"][2]["temp"]["max"]);
	pressureFc2 = (myObject["daily"][2]["pressure"]);
	humidityFc2 = (myObject["daily"][2]["humidity"]);
	windSpeedFc2 = (myObject["daily"][2]["wind_speed"]);
	windDirectionFc2 = (myObject["daily"][2]["wind_deg"]);
	uvIndexFc2 = (myObject["daily"][2]["uvi"]);
	weatherLabelFc2 = (myObject["daily"][2]["weather"][0]["id"]);
	rainLevelFc2 = (myObject["daily"][2]["rain"]);
	snowLevelFc2 = (myObject["daily"][2]["snow"]);
	sunRiseFc2 = (myObject["daily"][2]["sunrise"]);
	sunSetFc2 = (myObject["daily"][2]["sunset"]);

	weatherDesFc2.clear();
	serializeJson((myObject["daily"][2]["weather"][0]["description"]), weatherDesFc2);
	weatherDesFc2.replace('"', ' ');
	weatherDesFc2.remove(0, 1);
	weatherDesFc2[0] = toupper(weatherDesFc2[0]);

	forecastTimeFc3 = (myObject["daily"][3]["dt"]);
	tempMornFc3 = (myObject["daily"][3]["temp"]["morn"]);
	tempDayFc3 = (myObject["daily"][3]["temp"]["day"]);
	tempEvenFc3 = (myObject["daily"][3]["temp"]["eve"]);
	tempNightFc3 = (myObject["daily"][3]["temp"]["night"]);
	tempMinFc3 = (myObject["daily"][3]["temp"]["min"]);
	tempMaxFc3 = (myObject["daily"][3]["temp"]["max"]);
	pressureFc3 = (myObject["daily"][3]["pressure"]);
	humidityFc3 = (myObject["daily"][3]["humidity"]);
	windSpeedFc3 = (myObject["daily"][3]["wind_speed"]);
	windDirectionFc3 = (myObject["daily"][3]["wind_deg"]);
	uvIndexFc3 = (myObject["daily"][3]["uvi"]);
	weatherLabelFc3 = (myObject["daily"][3]["weather"][0]["id"]);
	rainLevelFc3 = (myObject["daily"][3]["rain"]);
	snowLevelFc3 = (myObject["daily"][3]["snow"]);
	sunRiseFc3 = (myObject["daily"][3]["sunrise"]);
	sunSetFc3 = (myObject["daily"][3]["sunset"]);

	weatherDesFc3.clear();
	serializeJson((myObject["daily"][3]["weather"][0]["description"]), weatherDesFc3);
	weatherDesFc3.replace('"', ' ');
	weatherDesFc3.remove(0, 1);
	weatherDesFc3[0] = toupper(weatherDesFc3[0]);

	forecastTimeFc4 = (myObject["daily"][4]["dt"]);
	tempMornFc4 = (myObject["daily"][4]["temp"]["morn"]);
	tempDayFc4 = (myObject["daily"][4]["temp"]["day"]);
	tempEvenFc4 = (myObject["daily"][4]["temp"]["eve"]);
	tempNightFc4 = (myObject["daily"][4]["temp"]["night"]);
	tempMinFc4 = (myObject["daily"][4]["temp"]["min"]);
	tempMaxFc4 = (myObject["daily"][4]["temp"]["max"]);
	pressureFc4 = (myObject["daily"][4]["pressure"]);
	humidityFc4 = (myObject["daily"][4]["humidity"]);
	windSpeedFc4 = (myObject["daily"][4]["wind_speed"]);
	windDirectionFc4 = (myObject["daily"][4]["wind_deg"]);
	uvIndexFc4 = (myObject["daily"][4]["uvi"]);
	weatherLabelFc4 = (myObject["daily"][4]["weather"][0]["id"]);
	rainLevelFc4 = (myObject["daily"][4]["rain"]);
	snowLevelFc4 = (myObject["daily"][4]["snow"]);
	sunRiseFc4 = (myObject["daily"][4]["sunrise"]);
	sunSetFc4 = (myObject["daily"][4]["sunset"]);

	weatherDesFc4.clear();
	serializeJson((myObject["daily"][4]["weather"][0]["description"]), weatherDesFc4);
	weatherDesFc4.replace('"', ' ');
	weatherDesFc4.remove(0, 1);
	weatherDesFc4[0] = toupper(weatherDesFc4[0]);

	forecastTimeFc5 = (myObject["daily"][5]["dt"]);
	tempMornFc5 = (myObject["daily"][5]["temp"]["morn"]);
	tempDayFc5 = (myObject["daily"][5]["temp"]["day"]);
	tempEvenFc5 = (myObject["daily"][5]["temp"]["eve"]);
	tempNightFc5 = (myObject["daily"][5]["temp"]["night"]);
	tempMinFc5 = (myObject["daily"][5]["temp"]["min"]);
	tempMaxFc5 = (myObject["daily"][5]["temp"]["max"]);
	pressureFc5 = (myObject["daily"][5]["pressure"]);
	humidityFc5 = (myObject["daily"][5]["humidity"]);
	windSpeedFc5 = (myObject["daily"][5]["wind_speed"]);
	windDirectionFc5 = (myObject["daily"][5]["wind_deg"]);
	uvIndexFc5 = (myObject["daily"][5]["uvi"]);
	weatherLabelFc5 = (myObject["daily"][5]["weather"][0]["id"]);
	rainLevelFc5 = (myObject["daily"][5]["rain"]);
	snowLevelFc5 = (myObject["daily"][5]["snow"]);
	sunRiseFc5 = (myObject["daily"][5]["sunrise"]);
	sunSetFc5 = (myObject["daily"][5]["sunset"]);

	weatherDesFc5.clear();
	serializeJson((myObject["daily"][5]["weather"][0]["description"]), weatherDesFc5);
	weatherDesFc5.replace('"', ' ');
	weatherDesFc5.remove(0, 1);
	weatherDesFc5[0] = toupper(weatherDesFc5[0]);

	forecastTimeFc6 = (myObject["daily"][6]["dt"]);
	tempMornFc6 = (myObject["daily"][6]["temp"]["morn"]);
	tempDayFc6 = (myObject["daily"][6]["temp"]["day"]);
	tempEvenFc6 = (myObject["daily"][6]["temp"]["eve"]);
	tempNightFc6 = (myObject["daily"][6]["temp"]["night"]);
	tempMinFc6 = (myObject["daily"][6]["temp"]["min"]);
	tempMaxFc6 = (myObject["daily"][6]["temp"]["max"]);
	pressureFc6 = (myObject["daily"][6]["pressure"]);
	humidityFc6 = (myObject["daily"][6]["humidity"]);
	windSpeedFc6 = (myObject["daily"][6]["wind_speed"]);
	windDirectionFc6 = (myObject["daily"][6]["wind_deg"]);
	uvIndexFc6 = (myObject["daily"][6]["uvi"]);
	weatherLabelFc6 = (myObject["daily"][6]["weather"][0]["id"]);
	rainLevelFc6 = (myObject["daily"][6]["rain"]);
	snowLevelFc6 = (myObject["daily"][6]["snow"]);
	sunRiseFc6 = (myObject["daily"][6]["sunrise"]);
	sunSetFc6 = (myObject["daily"][6]["sunset"]);

	weatherDesFc6.clear();
	serializeJson((myObject["daily"][6]["weather"][0]["description"]), weatherDesFc6);
	weatherDesFc6.replace('"', ' ');
	weatherDesFc6.remove(0, 1);
	weatherDesFc6[0] = toupper(weatherDesFc6[0]);

	forecastTimeFc7 = (myObject["daily"][7]["dt"]);
	tempMornFc7 = (myObject["daily"][7]["temp"]["morn"]);
	tempDayFc7 = (myObject["daily"][7]["temp"]["day"]);
	tempEvenFc7 = (myObject["daily"][7]["temp"]["eve"]);
	tempNightFc7 = (myObject["daily"][7]["temp"]["night"]);
	tempMinFc7 = (myObject["daily"][7]["temp"]["min"]);
	tempMaxFc7 = (myObject["daily"][7]["temp"]["max"]);
	pressureFc7 = (myObject["daily"][7]["pressure"]);
	humidityFc7 = (myObject["daily"][7]["humidity"]);
	windSpeedFc7 = (myObject["daily"][7]["wind_speed"]);
	windDirectionFc7 = (myObject["daily"][7]["wind_deg"]);
	uvIndexFc7 = (myObject["daily"][7]["uvi"]);
	weatherLabelFc7 = (myObject["daily"][7]["weather"][0]["id"]);
	rainLevelFc7 = (myObject["daily"][7]["rain"]);
	snowLevelFc7 = (myObject["daily"][7]["snow"]);
	sunRiseFc7 = (myObject["daily"][7]["sunrise"]);
	sunSetFc7 = (myObject["daily"][7]["sunset"]);

	weatherDesFc7.clear();
	serializeJson((myObject["daily"][7]["weather"][0]["description"]), weatherDesFc7);
	weatherDesFc7.replace('"', ' ');
	weatherDesFc7.remove(0, 1);
	weatherDesFc7[0] = toupper(weatherDesFc7[0]);

	//Serial.println(F(""));
	//Serial.print(F("Json Variable Daily Epoch : "));
	//Serial.println(forecastTimeFc1);
	//Serial.print(F("Json Variable Daily Weather ID 1  : "));
	//Serial.println(weatherLabelFc1);
	//Serial.print(F("Json Variable Daily Weather ID 2  : "));
	//Serial.println(weatherLabelFc2);
	//Serial.print(F("Json Variable Daily Weather ID 3  : "));
	//Serial.println(weatherLabelFc3);
	//Serial.print(F("Json Variable Daily Weather ID 4  : "));
	//Serial.println(weatherLabelFc4);
	//Serial.print(F("Json Variable Daily Weather ID 5  : "));
	//Serial.println(weatherLabelFc5);
	//Serial.print(F("Json Variable Daily Weather ID 6  : "));
	//Serial.println(weatherLabelFc6);
	//Serial.print(F("Json Variable Daily Weather ID 7  : "));
	//Serial.println(weatherLabelFc7);

	//Serial.print(F("Json Variable Daily Weather Des   : "));
	//Serial.println(weatherDesFc4);

	//Serial.print(F("Json Variable Daily Temp Morn     : "));
	//Serial.println(tempMornFc4);
	//Serial.print(F("Json Variable Daily Temp Day      : "));
	//Serial.println(tempDayFc4);
	//Serial.print(F("Json Variable Daily Temp Eve      : "));
	//Serial.println(tempEvenFc4);
	//Serial.print(F("Json Variable Daily Temp Night    : "));
	//Serial.println(tempNightFc4);
	//Serial.print(F("Json Variable Daily Temp Min      : "));
	//Serial.println(tempMinFc4);
	//Serial.print(F("Json Variable Daily Temp Max      : "));
	//Serial.println(tempMaxFc4);
	//Serial.print(F("Json Variable Daily Pressure      : "));
	//Serial.println(pressureFc4);
	//Serial.print(F("Json Variable Daily Humidity      : "));
	//Serial.println(humidityFc4);
	//Serial.print(F("Json Variable Daily Wind Speed    : "));
	//Serial.println(windSpeedFc4);
	//Serial.print(F("Json Variable Daily Wind Direction: "));
	//Serial.println(windDirectionFc4);
	//Serial.print(F("Json Variable Daily UV Undex      : "));
	//Serial.println(uvIndexFc4);
	//
	//Serial.print(F("Json Variable Daily Rain Level 1  : "));
	//Serial.println(rainLevelFc1);
	//Serial.print(F("Json Variable Daily Rain Level 2  : "));
	//Serial.println(rainLevelFc2);
	//Serial.print(F("Json Variable Daily Rain Level 3  : "));
	//Serial.println(rainLevelFc3);
	//Serial.print(F("Json Variable Daily Rain Level 4  : "));
	//Serial.println(rainLevelFc4);
	//Serial.print(F("Json Variable Daily Rain Level 5  : "));
	//Serial.println(rainLevelFc5);
	//Serial.print(F("Json Variable Daily Rain Level 6  : "));
	//Serial.println(rainLevelFc6);
	//Serial.print(F("Json Variable Daily Rain Level 7  : "));
	//Serial.println(rainLevelFc7);
	//
	//Serial.print(F("Json Variable Daily Sunrise       : "));
	//Serial.println(sunRiseFc1);
	//Serial.print(F("Json Variable Daily Sunset        : "));
	//Serial.println(sunSetFc1);

} // Close function.

/*-----------------------------------------------------------------*/

// Display current weather layout.

void currentWeatherLayout() {

	enableScreen1();

	drawBitmap(tft, TEMPERATUREICONC_Y, TEMPERATUREICONC_X, thermonmeterC, TEMPERATUREICONC_W, TEMPERATUREICONC_H);

	drawBitmap(tft, SUNRISE_Y, SUNRISE_X, sunrise, SUNRISE_W, SUNRISE_H);

	drawBitmap(tft, SUNSET_Y, SUNSET_X, sunset, SUNSET_W, SUNSET_H);

	drawBitmap(tft, MOONPHASE_Y, MOONPHASE_X, lastQuarterMoon, MOONPHASE_W, MOONPHASE_H);

	tft.setFont();
	tft.setTextSize(0);
	tft.setTextColor(BLACK);
	tft.setCursor(112, 215);
	tft.printf("Moon");

	tft.setFont();

	drawBitmap(tft, WINDSPEEDICON_Y, WINDSPEEDICON_X, windSpeed, WINDSPEEDICON_W, WINDSPEEDICON_H);

	drawBitmap(tft, WINDDIRECTIONICON_Y, WINDDIRECTIONICON_X, weatherVain, WINDDIRECTIONICON_W, WINDDIRECTIONICON_H);

	drawBitmap(tft, PRESSUREICON_Y, PRESSUREICON_X, pressure, PRESSUREICON_W, PRESSUREICON_H);

	drawBitmap(tft, RAINSNOWICON_Y, RAINSNOWICON_X, rainSnowFall, RAINSNOWICON_W, RAINSNOWICON_H);

	drawBitmap(tft, UVICON_Y, UVICON_X, uv, UVICON_W, UVICON_H);

	drawBitmap(tft, HUMIDITYICON_Y, HUMIDITYICON_X, humidity, HUMIDITYICON_W, HUMIDITYICON_H);

	disableVSPIScreens();

} // Close function.

/*-----------------------------------------------------------------*/

// Display current weather data.

void currentWeatherDataDisplay() {

	// Open Weather Request.

	//String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey + "&units=metric";
	//String serverPath = "http://api.openweathermap.org/data/2.5/onecall?lat=" + latitude + "&lon=" + longitude + "&APPID=" + openWeatherMapApiKey + "&units=metric" + "&exclude=minutely,hourly,alerts";

	//jsonBuffer = httpGETRequest(serverPath.c_str());
	//Serial.println("");
	//Serial.println(jsonBuffer);
	//deserializeJson(weatherData, jsonBuffer);
	//JsonObject myObject = weatherData.as<JsonObject>();
	//Serial.println(sunSetNow);

	// 	sprintf(tempMaxSpeedDate, "%02u/%02u/%02u at %02u:%02u", tempDay, tempMonth, tempYear, tempHour, tempMinute);

	char tempTemp[5];
	char tempFeelsLikeTemp[5];
	char tempHumidity[5];

	sprintf(tempTemp, "%2.0f%c%c", tempNow, 42, 'C'); // 42 is ASCII for degrees
	sprintf(tempFeelsLikeTemp, "%2.0f%c%c", feelsLikeTempNow, 42, 'C');
	sprintf(tempHumidity, "%2.0f", humidityNow);

	if (currentWeatherTime <= sunRiseNow) {

		dayNight = false;
	}

	else if (currentWeatherTime >= sunSetNow) {

		dayNight = false;
	}

	else dayNight = true;

	enableScreen1();

	currentWeatherImage(tft, currentWeatherNow, dayNight);

	disableVSPIScreens();

	Serial.print("Day Night: ");
	Serial.println(dayNight);

	enableScreen1();

	tft.fillRect(WINDSPEEDVALUE_X, WINDSPEEDVALUE_Y - 32, 50, 150, WHITE);		// Cover vertical values column 1
	tft.fillRect(WINDDIRECTION_X, WINDSPEEDVALUE_Y - 32, 50, 150, WHITE);		// Cover vertical values column 2
	tft.fillRect(TEMPERATUREVALUE_X, 30, 110, 40, WHITE);

	tft.setTextColor(BLACK, WHITE);
	tft.setFont(&FreeSans9pt7b);
	tft.setCursor(TEMPERATUREVALUE_X, TEMPERATUREVALUE_Y);
	tft.print(tempTemp);
	tft.setCursor(FEELSLIKETEMPERATUREICON_X, TEMPERATUREVALUE_Y);
	tft.print(tempFeelsLikeTemp);
	tft.setFont();
	tft.setTextSize(0);
	tft.setCursor(FEELSLIKETEMPERATUREICON_X + 5, TEMPERATUREVALUE_Y + 5);
	tft.print("Feels Like");

	tft.setFont(&FreeSans9pt7b);
	tft.setCursor(PRESSUREVALUE_X, PRESSUREVALUE_Y);
	tft.print(pressureNow);
	tft.setFont();
	tft.setTextSize(0);

	if (pressureNow > 999) {

		tft.setCursor(PRESSUREVALUE_X + 5, PRESSUREVALUE_Y + 5);
		tft.print("hPa");
	}

	else {
		tft.setCursor(PRESSUREVALUE_X + 2, PRESSUREVALUE_Y + 5);
		tft.print("hPa");
	}

	tft.setFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setCursor(HUMIDITYVALUE_X, HUMIDITYVALUE_Y);
	tft.print(tempHumidity);
	tft.print("%");

	tft.setFont(&FreeSans9pt7b);
	tft.setCursor(WINDSPEEDVALUE_X, WINDSPEEDVALUE_Y);
	tft.print(windSpeedNow);
	tft.setFont();
	tft.setTextSize(0);

	if (windSpeedNow > 19.9) {

		tft.setCursor(WINDSPEEDVALUE_X + 2, WINDSPEEDVALUE_Y + 5);
		tft.print("m/ps");
	}

	else if (windSpeedNow > 9.9) {

		tft.setCursor(WINDSPEEDVALUE_X + 5, WINDSPEEDVALUE_Y + 5);
		tft.print("m/ps");
	}

	else if (windSpeedNow > 1.9) {

		tft.setCursor(WINDSPEEDVALUE_X + 2, WINDSPEEDVALUE_Y + 5);
		tft.print("m/ps");
	}

	else if (windSpeedNow < 1) {
		tft.setCursor(WINDSPEEDVALUE_X + 2, WINDSPEEDVALUE_Y + 5);
		tft.print("m/ps");
	}

	else {
		tft.setCursor(WINDSPEEDVALUE_X + 5, WINDSPEEDVALUE_Y + 5);
		tft.print("m/ps");
	}

	heading = getHeadingReturn(windDirectionNow);
	tft.setFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setCursor(WINDDIRECTION_X, WINDDIRECTION_Y);
	tft.print(headingArray[heading]);

	tft.setCursor(UVVALUE_X, UVVALUE_Y);
	tft.print(uvIndexNow);
	tft.setCursor(RAINSNOWVALUE_X, RAINSNOWVALUE_Y);
	tft.print(rainLevelNow);
	tft.setFont();
	tft.setTextSize(0);

	if (rainLevelNow > 19.9) {

		tft.setCursor(RAINSNOWVALUE_X + 2, RAINSNOWVALUE_Y + 5);
		tft.print("mm");
	}

	else if (rainLevelNow > 9.9) {

		tft.setCursor(RAINSNOWVALUE_X + 5, RAINSNOWVALUE_Y + 5);
		tft.print("mm");
	}

	else if (rainLevelNow > 1.9) {

		tft.setCursor(RAINSNOWVALUE_X + 2, RAINSNOWVALUE_Y + 5);
		tft.print("mm");
	}

	else if (rainLevelNow < 1) {
		tft.setCursor(RAINSNOWVALUE_X + 2, RAINSNOWVALUE_Y + 5);
		tft.print("mm");
	}

	else {
		tft.setCursor(RAINSNOWVALUE_X + 5, RAINSNOWVALUE_Y + 5);
		tft.print("mm");
	}

	// Extract sunrise time.

	time_t sunRise = sunRiseNow;
	struct tm  ts;
	char       buf[80];

	// Format time example, "ddd yyyy-mm-dd hh:mm:ss zzz" - strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);

	ts = *localtime(&sunRise);
	strftime(buf, sizeof(buf), "%H:%M", &ts);

	Serial.println("");
	Serial.print("Check conversion of sunrise time: ");
	Serial.printf("%s\n", buf);
	Serial.println("");

	tft.setFont();
	tft.setTextSize(0);
	tft.setTextColor(BLACK);
	tft.setCursor(11, 215);
	tft.printf("%s\n", buf);

	// Extract sunset time.

	time_t sunSet = sunSetNow;

	ts = *localtime(&sunSet);
	strftime(buf, sizeof(buf), "%H:%M", &ts);

	Serial.print("Check conversion of sunset time : ");
	Serial.printf("%s\n", buf);
	Serial.println("");

	tft.setFont();
	tft.setTextSize(0);
	tft.setTextColor(BLACK);
	tft.setCursor(61, 215);
	tft.printf("%s\n", buf);

	tft.setFont();

	disableVSPIScreens();

} // Close function.

/*-----------------------------------------------------------------*/

void currentWeatherTemp() {

	enableScreen1();

	if (flagTempDisplayChange == false) {

		tft.fillRect(0, 0, 320, 25, WHITE);

		tft.setFont(&FreeSans9pt7b);
		tft.setTextSize(1);
		tft.setTextColor(BLACK, WHITE);
		tft.setCursor(5, 20);
		tft.println("Current Weather");
		bme280.update();
		t = bme280.getT();
		h = bme280.getH();
		tft.setCursor(150, 20);			// Remove later
		tft.print("T: ");				// Remove later
		tft.print(t);					// Remove later
		tft.print(" H: ");				// Remove later
		tft.print(h);					// Remove later
		tft.setFont();					// Remove later

	}

	else if (flagTempDisplayChange == true) {

		enableScreen1();

		tft.fillRect(0, 0, 320, 25, WHITE);

		tft.setFont(&FreeSans9pt7b);
		tft.setTextSize(1);
		tft.setTextColor(BLACK, WHITE);
		tft.setCursor(5, 20);
		tft.print(weatherDesCurrent);
		bme280.update();
		t = bme280.getT();
		h = bme280.getH();
		tft.setCursor(150, 20);			// Remove later
		tft.print("T: ");				// Remove later
		tft.print(t);					// Remove later
		tft.print(" H: ");				// Remove later
		tft.print(h);					// Remove later
		tft.setFont();					// Remove later

	}

	disableVSPIScreens();

} // Close function.

/*-----------------------------------------------------------------*/

// Display forecast weather layout day X.

void forecastWeatherLayoutDayX(Adafruit_ILI9341& tft, byte screen) {

	if (screen == 1) {

		enableScreen1();
	}

	else if (screen == 2) {

		enableScreen2();
	}

	else if (screen == 3) {

		enableScreen3();
	}

	else if (screen == 4) {

		enableScreen4();
	}

	else if (screen == 5) {

		enableScreen5();
	}

	else if (screen == 6) {

		enableScreen6();
	}

	else if (screen == 7) {

		enableScreen7();
	}

	else if (screen == 8) {

		enableScreen8();
	}

	drawBitmap(tft, TEMPERATUREICONC_Y, TEMPERATUREICONC_X, thermonmeterC, TEMPERATUREICONC_W, TEMPERATUREICONC_H);

	drawBitmap(tft, SUNRISE_Y, SUNRISE_X, sunrise, SUNRISE_W, SUNRISE_H);

	drawBitmap(tft, SUNSET_Y, SUNSET_X, sunset, SUNSET_W, SUNSET_H);

	tft.setFont();
	tft.setTextSize(0);
	tft.setTextColor(BLACK);
	tft.setCursor(112, 215);
	tft.printf("Moon");

	tft.setFont();

	drawBitmap(tft, WINDSPEEDICON_Y, WINDSPEEDICON_X, windSpeed, WINDSPEEDICON_W, WINDSPEEDICON_H);

	drawBitmap(tft, WINDDIRECTIONICON_Y, WINDDIRECTIONICON_X, weatherVain, WINDDIRECTIONICON_W, WINDDIRECTIONICON_H);

	drawBitmap(tft, PRESSUREICON_Y, PRESSUREICON_X, pressure, PRESSUREICON_W, PRESSUREICON_H);

	drawBitmap(tft, RAINSNOWICON_Y, RAINSNOWICON_X, rainSnowFall, RAINSNOWICON_W, RAINSNOWICON_H);

	drawBitmap(tft, UVICON_Y, UVICON_X, uv, UVICON_W, UVICON_H);

	drawBitmap(tft, HUMIDITYICON_Y, HUMIDITYICON_X, humidity, HUMIDITYICON_W, HUMIDITYICON_H);

	disableVSPIScreens();

} // Close function.

/*-----------------------------------------------------------------*/

// Display forecast data day X.

void forecastDataDisplayDayX(Adafruit_ILI9341& tft, byte screen, unsigned long forecastTimeFc, unsigned int pressureFc, double humidityFc, double windSpeedFc, unsigned int windDirectionFc, byte uvIndexFc, unsigned int weatherLabelFc, double rainLevelFc, unsigned long sunRiseFc, unsigned long sunSetFc) {

	if (screen == 1) {

		enableScreen1();
	}

	else if (screen == 2) {

		enableScreen2();
	}

	else if (screen == 3) {

		enableScreen3();
	}

	else if (screen == 4) {

		enableScreen4();
	}

	else if (screen == 5) {

		enableScreen5();
	}

	else if (screen == 6) {

		enableScreen6();
	}

	else if (screen == 7) {

		enableScreen7();
	}

	else if (screen == 8) {

		enableScreen8();
	}

	// 	sprintf(tempMaxSpeedDate, "%02u/%02u/%02u at %02u:%02u", tempDay, tempMonth, tempYear, tempHour, tempMinute);

	//char tempTemp[5];
	//char tempTempNight[5];
	char tempHumidity[5];

	//sprintf(tempTemp, "%2.0f%c%c", tempDayFc1, 42, 'C'); // 42 is ASCII for degrees
	//sprintf(tempTempNight, "%2.0f%c%c", tempNightFc1, 42, 'C');
	sprintf(tempHumidity, "%2.0f", humidityFc);

	forecastWeatherImage(tft, weatherLabelFc);

	tft.fillRect(WINDSPEEDVALUE_X, WINDSPEEDVALUE_Y - 32, 50, 150, WHITE);		// Cover vertical values column 1
	tft.fillRect(WINDDIRECTION_X, WINDSPEEDVALUE_Y - 32, 50, 150, WHITE);		// Cover vertical values column 2

	tft.setFont(&FreeSans9pt7b);
	tft.setCursor(PRESSUREVALUE_X, PRESSUREVALUE_Y);
	tft.print(pressureFc);
	tft.setFont();
	tft.setTextSize(0);

	if (pressureFc > 999) {

		tft.setCursor(PRESSUREVALUE_X + 5, PRESSUREVALUE_Y + 5);
		tft.print("hPa");
	}

	else {
		tft.setCursor(PRESSUREVALUE_X + 2, PRESSUREVALUE_Y + 5);
		tft.print("hPa");
	}

	tft.setFont(&FreeSans9pt7b);
	tft.setCursor(HUMIDITYVALUE_X, HUMIDITYVALUE_Y);
	tft.print(tempHumidity);
	tft.print("%");

	tft.setCursor(WINDSPEEDVALUE_X, WINDSPEEDVALUE_Y);
	tft.print(windSpeedFc);
	tft.setFont();
	tft.setTextSize(0);

	if (windSpeedFc > 19.9) {

		tft.setCursor(WINDSPEEDVALUE_X + 2, WINDSPEEDVALUE_Y + 5);
		tft.print("m/ps");
	}

	else if (windSpeedFc > 9.9) {

		tft.setCursor(WINDSPEEDVALUE_X + 5, WINDSPEEDVALUE_Y + 5);
		tft.print("m/ps");
	}

	else if (windSpeedFc > 1.9) {

		tft.setCursor(WINDSPEEDVALUE_X + 2, WINDSPEEDVALUE_Y + 5);
		tft.print("m/ps");
	}

	else if (windSpeedFc < 1) {
		tft.setCursor(WINDSPEEDVALUE_X + 2, WINDSPEEDVALUE_Y + 5);
		tft.print("m/ps");
	}

	else {
		tft.setCursor(WINDSPEEDVALUE_X + 5, WINDSPEEDVALUE_Y + 5);
		tft.print("m/ps");
	}

	heading = getHeadingReturn(windDirectionFc);
	tft.setFont(&FreeSans9pt7b);
	tft.setCursor(WINDDIRECTION_X, WINDDIRECTION_Y);
	tft.print(headingArray[heading]);

	tft.setCursor(UVVALUE_X, UVVALUE_Y);
	tft.setFont(&FreeSans9pt7b);
	tft.print(uvIndexFc);
	tft.setCursor(RAINSNOWVALUE_X, RAINSNOWVALUE_Y);
	tft.print(rainLevelFc);
	tft.setFont();
	tft.setTextSize(0);

	if (rainLevelFc > 19.9) {

		tft.setCursor(RAINSNOWVALUE_X + 2, RAINSNOWVALUE_Y + 5);
		tft.print("mm");
	}

	else if (rainLevelFc > 9.9) {

		tft.setCursor(RAINSNOWVALUE_X + 5, RAINSNOWVALUE_Y + 5);
		tft.print("mm");
	}

	else if (rainLevelFc > 1.9) {

		tft.setCursor(RAINSNOWVALUE_X + 2, RAINSNOWVALUE_Y + 5);
		tft.print("mm");
	}

	else if (rainLevelFc < 1) {
		tft.setCursor(RAINSNOWVALUE_X + 2, RAINSNOWVALUE_Y + 5);
		tft.print("mm");
	}

	else {
		tft.setCursor(RAINSNOWVALUE_X + 5, RAINSNOWVALUE_Y + 5);
		tft.print("mm");
	}

	// Extract sunrise time.

	time_t sunRise = sunRiseFc;
	struct	tm  ts;
	char	buf[80];

	// Format time example, "ddd yyyy-mm-dd hh:mm:ss zzz" - strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);

	ts = *localtime(&sunRise);
	strftime(buf, sizeof(buf), "%H:%M", &ts);

	Serial.println("");
	Serial.print("Check conversion of sunrise time: ");
	Serial.printf(buf);
	Serial.println("");

	tft.setFont();
	tft.setTextSize(0);
	tft.setTextColor(BLACK);
	tft.setCursor(11, 215);
	tft.printf("%s\n", buf);

	// Extract sunset time.

	time_t sunSet = sunSetFc;

	ts = *localtime(&sunSet);
	strftime(buf, sizeof(buf), "%H:%M", &ts);

	Serial.print("Check conversion of sunset time : ");
	Serial.printf(buf);
	Serial.println("");

	tft.setFont();
	tft.setTextSize(0);
	tft.setTextColor(BLACK);
	tft.setCursor(61, 215);
	tft.printf("%s\n", buf);

	tft.setFont();

	//time_t forecastDate = forecastTimeFc;

	//ts = *localtime(&forecastDate);
	//strftime(buf, sizeof(buf), "%A, %B %d %Y", &ts);

	//// Text block to over write characters from longer dates when date changes and unit has been running.

	//tft.setTextColor(BLACK, WHITE);
	//tft.setFont();
	//tft.setTextSize(1);
	//tft.setCursor(150, 230);
	//tft.println("                ");

	//// Actual date time to display.

	//tft.setTextColor(BLACK, WHITE);
	//tft.setFont();
	//tft.setTextSize(1);
	//tft.setCursor(5, 230);
	//tft.println(buf);

	disableVSPIScreens();

} // Close function.

/*-----------------------------------------------------------------*/

// Display forecast day / night / min / max temperature day X.

void forecastDataDisplayTempDayX(Adafruit_ILI9341& tft, byte screen, unsigned long forecastTimeFc, String weatherDesFc, double tempDayFc, double tempNightFc, double tempMinFc, double tempMaxFc) {

	if (screen == 1) {

		enableScreen1();
	}

	else if (screen == 2) {

		enableScreen2();
	}

	else if (screen == 3) {

		enableScreen3();
	}

	else if (screen == 4) {

		enableScreen4();
	}

	else if (screen == 5) {

		enableScreen5();
	}

	else if (screen == 6) {

		enableScreen6();
	}

	else if (screen == 7) {

		enableScreen7();
	}

	else if (screen == 8) {

		enableScreen8();
	}

	struct	tm  ts;
	char	bufDay[30];

	time_t forecastDate = forecastTimeFc;

	ts = *localtime(&forecastDate);
	strftime(bufDay, sizeof(bufDay), "%A, %B %d", &ts);

	char tempTemp[5];
	char tempTempNight[5];
	char tempTempMin[5];
	char tempTempMax[5];

	sprintf(tempTemp, "%2.0f%c%c", tempDayFc, 42, 'C'); // 42 is ASCII for degrees
	sprintf(tempTempNight, "%2.0f%c%c", tempNightFc, 42, 'C');
	sprintf(tempTempMin, "%2.0f%c%c", tempMinFc, 42, 'C'); // 42 is ASCII for degrees
	sprintf(tempTempMax, "%2.0f%c%c", tempMaxFc, 42, 'C');

	if (flagTempDisplayChange == false) {

		tft.fillRect(TEMPERATUREVALUE_X, 30, 110, 40, WHITE);
		tft.fillRect(0, 0, 320, 25, WHITE);

		tft.setTextColor(BLACK, WHITE);
		tft.setFont(&FreeSans9pt7b);
		tft.setCursor(TEMPERATUREVALUE_X, TEMPERATUREVALUE_Y);
		tft.print(tempTemp);
		tft.setFont();
		tft.setTextSize(0);
		tft.setCursor(TEMPERATUREVALUE_X + 5, TEMPERATUREVALUE_Y + 5);
		tft.print("Day");
		tft.setFont(&FreeSans9pt7b);
		tft.setCursor(FEELSLIKETEMPERATUREICON_X, TEMPERATUREVALUE_Y);
		tft.print(tempTempNight);
		tft.setFont();
		tft.setTextSize(0);
		tft.setCursor(FEELSLIKETEMPERATUREICON_X + 5, TEMPERATUREVALUE_Y + 5);
		tft.print("Night");

		tft.setFont(&FreeSans9pt7b);
		tft.setTextSize(1);
		tft.setTextColor(BLACK, WHITE);
		tft.setCursor(5, 20);
		tft.print(bufDay);
		tft.setFont();

	}

	else if (flagTempDisplayChange == true) {

		tft.fillRect(TEMPERATUREVALUE_X, 30, 110, 40, WHITE);
		tft.fillRect(0, 0, 320, 25, WHITE);

		tft.setTextColor(BLUE);
		tft.setFont(&FreeSans9pt7b);
		tft.setCursor(TEMPERATUREVALUE_X, TEMPERATUREVALUE_Y);
		tft.print(tempTempMin);
		tft.setFont();
		tft.setTextSize(0);
		tft.setCursor(TEMPERATUREVALUE_X + 5, TEMPERATUREVALUE_Y + 5);
		tft.print("Min");
		tft.setTextColor(RED);
		tft.setFont(&FreeSans9pt7b);
		tft.setCursor(FEELSLIKETEMPERATUREICON_X, TEMPERATUREVALUE_Y);
		tft.print(tempTempMax);
		tft.setFont();
		tft.setTextSize(0);
		tft.setCursor(FEELSLIKETEMPERATUREICON_X + 5, TEMPERATUREVALUE_Y + 5);
		tft.print("Max");
		tft.setTextColor(BLACK);

		tft.setFont(&FreeSans9pt7b);
		tft.setTextSize(1);
		tft.setTextColor(BLACK, WHITE);
		tft.setCursor(5, 20);
		tft.print(weatherDesFc);
		tft.setFont();

	}

	disableVSPIScreens();

} // Close function.

/*-----------------------------------------------------------------*/

// WiFi title page.

void wiFiTitle() {

	// WiFi title screen.

	enableScreen1();

	tft.setFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setTextColor(BLACK, WHITE);
	tft.setCursor(13, 26);
	tft.println("Setting up WiFi");
	tft.setFont();

	tft.setTextColor(BLACK, WHITE);
	tft.setFont();
	tft.setCursor(23, 50);
	tft.print("WiFi Status: ");
	tft.setCursor(23, 65);
	tft.print("SSID: ");
	tft.setCursor(23, 80);
	tft.print("IP Address: ");
	tft.setCursor(23, 95);
	tft.print("DNS Address: ");
	tft.setCursor(23, 110);
	tft.print("Gateway Address: ");
	tft.setCursor(23, 125);
	tft.print("Signal Strenght: ");
	tft.setCursor(23, 140);
	tft.print("Time Server: ");

	disableVSPIScreens();

} // Close function.

/*-----------------------------------------------------------------*/

// Draw hourly temperature chart.

void drawHourlyTempChart(Adafruit_ILI9341& tft) {

	tft.setFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setTextColor(BLACK, WHITE);
	tft.setCursor(5, 20);
	tft.print("Temperature");
	tft.setFont();

	drawBitmap(tft, 40, 228, temperatureLrgBlk, 64, 64);

	tft.setTextSize(1);
	tft.setTextColor(GREY, WHITE);
	tft.setCursor(60, 228);
	tft.println("Current + hr");
	tft.setFont();

	tft.setFont(&FreeSans12pt7b);
	tft.setTextColor(BLACK, WHITE);
	tft.setCursor(215, 135);
	tft.println("Current");

	if (temperatureHr1 > 9) {

		tft.setFont(&FreeSans12pt7b);
		tft.setTextColor(BLACK, WHITE);
		tft.setCursor(240, 160);
		tft.println(temperatureHr1);
	}

	else if (temperatureHr1 >= 0) {

		tft.setFont(&FreeSans12pt7b);
		tft.setTextColor(BLACK, WHITE);
		tft.setCursor(247, 160);
		tft.println(temperatureHr1);
	}

	else if (temperatureHr1 <= -10) {

		tft.setFont(&FreeSans12pt7b);
		tft.setTextColor(BLACK, WHITE);
		tft.setCursor(235, 160);
		tft.println(temperatureHr1);
	}


	else if (temperatureHr1 <= 0) {

		tft.setFont(&FreeSans12pt7b);
		tft.setTextColor(BLACK, WHITE);
		tft.setCursor(243, 160);
		tft.println(temperatureHr1);
	}

	tft.setFont();

	int loVal = -30;
	int hiVal = 45;


	if (temperatureHr1 > 15) {

		loVal = 10;
		hiVal = 45;
	}

	else if (temperatureHr1 > -10 && temperatureHr1 <= 15) {

		loVal = -10;
		hiVal = 25;
	}

	else if (temperatureHr1 < -10) {

		loVal = -30;
		hiVal = 5;
	}


	drawBarChartV1(tft, 1, 20, 210, 10, 170, loVal, hiVal, 5, temperatureHr1, 2, 0, LTBLUE, WHITE, GREY, GREY, WHITE, "C", graph_1);
	drawBarChartV1(tft, 1, 40, 210, 10, 170, loVal, hiVal, 5, temperatureHr2, 2, 0, LTBLUE, WHITE, GREY, GREY, WHITE, "+1", graph_2);
	drawBarChartV1(tft, 1, 60, 210, 10, 170, loVal, hiVal, 5, temperatureHr3, 2, 0, LTBLUE, WHITE, GREY, GREY, WHITE, "+2", graph_3);
	drawBarChartV1(tft, 1, 80, 210, 10, 170, loVal, hiVal, 5, temperatureHr4, 2, 0, LTBLUE, WHITE, GREY, GREY, WHITE, "+3", graph_4);
	drawBarChartV1(tft, 1, 100, 210, 10, 170, loVal, hiVal, 5, temperatureHr5, 2, 0, LTBLUE, WHITE, GREY, GREY, WHITE, "+4", graph_5);
	drawBarChartV1(tft, 1, 120, 210, 10, 170, loVal, hiVal, 5, temperatureHr6, 2, 0, LTBLUE, WHITE, GREY, GREY, WHITE, "+5", graph_6);
	drawBarChartV1(tft, 1, 140, 210, 10, 170, loVal, hiVal, 5, temperatureHr7, 2, 0, LTBLUE, WHITE, GREY, GREY, WHITE, "+6", graph_7);
	drawBarChartV2(tft, 1, 160, 210, 10, 170, loVal, hiVal, 5, temperatureHr8, 2, 0, LTBLUE, WHITE, GREY, GREY, WHITE, "+7", graph_8);

	disableVSPIScreens();

} // Close function.

/*-----------------------------------------------------------------*/

// Draw hourly rain fall chart.

void drawHourlyRainChart(Adafruit_ILI9341& tft) {

	tft.setFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setTextColor(BLACK, WHITE);
	tft.setCursor(5, 20);
	tft.print("Rain Fall");
	tft.setFont();

	drawBitmap(tft, 40, 228, rainFallLrgBlk, 64, 64);

	tft.setTextSize(1);
	tft.setTextColor(GREY, WHITE);
	tft.setCursor(202, 206);
	tft.println("mm");
	tft.setFont();

	tft.setTextSize(1);
	tft.setTextColor(GREY, WHITE);
	tft.setCursor(60, 228);
	tft.println("Current + hr");
	tft.setFont();

	if (rainFallHr1 > 100) {

		rainFallHr1 = 100;
	}

	tft.setFont(&FreeSans12pt7b);
	tft.setTextColor(BLACK, WHITE);
	tft.setCursor(235, 135);
	tft.println(rainFallHr1);
	tft.setCursor(240, 160);
	tft.println("mm");
	tft.setFont();

	tft.setTextSize(0);

	// Convert Double to Int

	int rainFallHr1Int = rainFallHr1 + 0.5;
	int rainFallHr2Int = rainFallHr2 + 0.5;
	int rainFallHr3Int = rainFallHr3 + 0.5;
	int rainFallHr4Int = rainFallHr4 + 0.5;
	int rainFallHr5Int = rainFallHr5 + 0.5;
	int rainFallHr6Int = rainFallHr6 + 0.5;
	int rainFallHr7Int = rainFallHr7 + 0.5;
	int rainFallHr8Int = rainFallHr8 + 0.5;

	// Insert scale change including intervals

	drawBarChartV1(tft, 1, 20, 210, 10, 170, 0, 10, 1, rainFallHr1Int, 2, 0, LTBLUE, WHITE, GREY, GREY, WHITE, "C", graph_1);
	drawBarChartV1(tft, 1, 40, 210, 10, 170, 0, 10, 1, rainFallHr2Int, 2, 0, LTBLUE, WHITE, GREY, GREY, WHITE, "+1", graph_2);
	drawBarChartV1(tft, 1, 60, 210, 10, 170, 0, 10, 1, rainFallHr3Int, 2, 0, LTBLUE, WHITE, GREY, GREY, WHITE, "+2", graph_3);
	drawBarChartV1(tft, 1, 80, 210, 10, 170, 0, 10, 1, rainFallHr4Int, 2, 0, LTBLUE, WHITE, GREY, GREY, WHITE, "+3", graph_4);
	drawBarChartV1(tft, 1, 100, 210, 10, 170, 0, 10, 1, rainFallHr5Int, 2, 0, LTBLUE, WHITE, GREY, GREY, WHITE, "+4", graph_5);
	drawBarChartV1(tft, 1, 120, 210, 10, 170, 0, 10, 1, rainFallHr6Int, 2, 0, LTBLUE, WHITE, GREY, GREY, WHITE, "+5", graph_6);
	drawBarChartV1(tft, 1, 140, 210, 10, 170, 0, 10, 1, rainFallHr7Int, 2, 0, LTBLUE, WHITE, GREY, GREY, WHITE, "+6", graph_7);
	drawBarChartV2(tft, 1, 160, 210, 10, 170, 0, 10, 1, rainFallHr8Int, 2, 0, LTBLUE, WHITE, GREY, GREY, WHITE, "+7", graph_8);

	disableVSPIScreens();

} // Close function.

/*-----------------------------------------------------------------*/

// Draw hourly pressure chart.

void drawHourlyPressureChart(Adafruit_ILI9341& tft) {

	tft.setFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setTextColor(BLACK, WHITE);
	tft.setCursor(5, 20);
	tft.print("Air Pressure is ");
	tft.setFont();

	drawBitmap(tft, 40, 228, pressureLrgBlk, 64, 64);

	tft.setTextSize(1);
	tft.setTextColor(GREY, WHITE);
	tft.setCursor(60, 228);
	tft.println("Current + hr");
	tft.setFont();

	if (pressureHr1 > 999) {

		tft.setFont(&FreeSans12pt7b);
		tft.setTextColor(BLACK, WHITE);
		tft.setCursor(235, 135);
		tft.println(pressureHr1);
		tft.setFont();
	}

	else {

		tft.setFont(&FreeSans12pt7b);
		tft.setTextColor(BLACK, WHITE);
		tft.setCursor(242, 135);
		tft.println(pressureHr1);
		tft.setFont();
	}

	tft.setTextSize(1);
	tft.setTextColor(BLACK, WHITE);
	tft.setCursor(250, 145);
	tft.println("mbar");
	tft.setFont();

	if (pressureHr1 > 1022) {

		tft.setFont(&FreeSans12pt7b);
		tft.setTextColor(RED, WHITE);
		tft.setCursor(238, 180);
		tft.println(pressureLevel[2]);
		tft.setFont();

	}

	else if (pressureHr1 < 1009) {

		tft.setFont(&FreeSans12pt7b);
		tft.setTextColor(BLUE, WHITE);
		tft.setCursor(238, 180);
		tft.println(pressureLevel[0]);
		tft.setFont();

	}

	else {

		tft.setFont(&FreeSans12pt7b);
		tft.setTextColor(BLACK, WHITE);
		tft.setCursor(222, 180);
		tft.println(pressureLevel[1]);
		tft.setFont();

	}

	int tempPressure;
	tempPressure = pressureHr3 - pressureHr1;

	Serial.print("Pressure Differece = ");
	Serial.println(tempPressure);
	Serial.println("");

	if (tempPressure <= 0) {

		Serial.println("");
		Serial.print("Pressure is falling function");

		if (tempPressure == 0 || tempPressure == -1) {

			tft.setFont(&FreeSans9pt7b);
			tft.setTextColor(BLACK, WHITE);
			tft.setCursor(128, 20);
			tft.println("steady");
			tft.setFont();
		}

		else if (tempPressure == -2 || tempPressure == -3 || tempPressure == -4 || tempPressure == -5) {

			tft.setFont(&FreeSans9pt7b);
			tft.setTextColor(BLACK, WHITE);
			tft.setCursor(128, 20);
			tft.println("falling");
			tft.setFont();
		}

		else if (tempPressure >= -6) {

			tft.setFont(&FreeSans9pt7b);
			tft.setTextColor(BLACK, WHITE);
			tft.setCursor(128, 20);
			tft.println("falling rapidly");
			tft.setFont();
		}
	}

	else if (tempPressure > 0) {

		Serial.println("");
		Serial.print("Pressure is rising function");

		if (tempPressure == 1) {

			tft.setFont(&FreeSans9pt7b);
			tft.setTextColor(BLACK, WHITE);
			tft.setCursor(128, 20);
			tft.println("steady");
			tft.setFont();
		}

		else if (tempPressure == 2 || tempPressure == 3 || tempPressure == 4 || tempPressure == 5) {

			tft.setFont(&FreeSans9pt7b);
			tft.setTextColor(BLACK, WHITE);
			tft.setCursor(128, 20);
			tft.println("rising");
			tft.setFont();
		}

		else if (tempPressure >= 6) {

			tft.setFont(&FreeSans9pt7b);
			tft.setTextColor(BLACK, WHITE);
			tft.setCursor(128, 20);
			tft.println("rising rapidly");
			tft.setFont();
		}
	}

	tft.setTextSize(0);

	int loVal = 900;
	int hiVal = 1100;


	if (pressureHr1 > 1020) {

		loVal = 1000;
		hiVal = 1100;
	}

	else if (pressureHr1 > 960 && pressureHr1 < 1020) {

		loVal = 950;
		hiVal = 1050;
	}

	else if (pressureHr1 < 961) {

		loVal = 900;
		hiVal = 1000;
	}

	drawBarChartV1(tft, 4, 20, 210, 10, 170, loVal, hiVal, 20, pressureHr1, 2, 0, LTBLUE, WHITE, GREY, GREY, WHITE, "C", graph_1);
	drawBarChartV1(tft, 4, 40, 210, 10, 170, loVal, hiVal, 20, pressureHr2, 2, 0, LTBLUE, WHITE, GREY, GREY, WHITE, "+1", graph_2);
	drawBarChartV1(tft, 4, 60, 210, 10, 170, loVal, hiVal, 20, pressureHr3, 2, 0, LTBLUE, WHITE, GREY, GREY, WHITE, "+2", graph_3);
	drawBarChartV1(tft, 4, 80, 210, 10, 170, loVal, hiVal, 20, pressureHr4, 2, 0, LTBLUE, WHITE, GREY, GREY, WHITE, "+3", graph_4);
	drawBarChartV1(tft, 4, 100, 210, 10, 170, loVal, hiVal, 20, pressureHr5, 2, 0, LTBLUE, WHITE, GREY, GREY, WHITE, "+4", graph_5);
	drawBarChartV1(tft, 4, 120, 210, 10, 170, loVal, hiVal, 20, pressureHr6, 2, 0, LTBLUE, WHITE, GREY, GREY, WHITE, "+5", graph_6);
	drawBarChartV1(tft, 4, 140, 210, 10, 170, loVal, hiVal, 20, pressureHr7, 2, 0, LTBLUE, WHITE, GREY, GREY, WHITE, "+6", graph_7);
	drawBarChartV2(tft, 4, 160, 210, 10, 170, loVal, hiVal, 20, pressureHr8, 2, 0, LTBLUE, WHITE, GREY, GREY, WHITE, "+7", graph_8);

	disableVSPIScreens();

} // Close function.

/*-----------------------------------------------------------------*/

// Draw main compass screen.

void drawCompassChart(Adafruit_ILI9341& tft) {

tft.fillScreen(WHITE);

tft.setFont(&FreeSans9pt7b);
tft.setTextSize(1);
tft.setTextColor(BLACK, WHITE);
tft.setCursor(5, 20);
tft.print("Wind Direction");
tft.setFont();

drawBitmap(tft, 40, 228, weatherVainLrgBlk, 64, 64);

if (windDirectionNow > 99) {

	tft.setFont(&FreeSans12pt7b);
	tft.setTextColor(BLACK, WHITE);
	tft.setCursor(235, 135);
	tft.println(windDirectionNow);
	tft.setFont();
}

else {

	tft.setFont(&FreeSans12pt7b);
	tft.setTextColor(BLACK, WHITE);
	tft.setCursor(242, 135);
	tft.println(windDirectionNow);
	tft.setFont();
}


drawCompass(tft);
drawNeedle(tft, windDirectionNow);

} // Close function.

/*-----------------------------------------------------------------*/

// Draw borders.

void drawBorder() {

	// Draw layout borders.

	enableScreen1();
	tft.drawRect(FRAME1_X, FRAME1_Y, FRAME1_W, FRAME1_H, WHITE);
	disableVSPIScreens();
	//tft.drawRect(FRAME2_X, FRAME2_Y, FRAME2_W, FRAME2_H, WHITE);

} // Close function.

/*-----------------------------------------------------------------*/

// Draw black boxes when screens change.

void drawBlackBox() {

	// Clear screen by using a black box.
	enableScreen1();
	tft.fillRect(FRAME2_X + 1, FRAME2_Y + 25, FRAME2_W - 2, FRAME2_H - 40, BLACK);		// This covers only the graphs and charts, not the system icons to save refresh flicker.
	tft.fillRect(FRAME2_X + 1, FRAME2_Y + 1, FRAME2_W - 90, FRAME2_H - 200, BLACK);		// Ths covers the title text per page
	disableVSPIScreens();

} // Close function.

/*-----------------------------------------------------------------*/
