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

#include <Fonts/FreeSans9pt7b.h>	
#include <Fonts/FreeSans12pt7b.h>

#include "getHeading.h"				// Convert headings to N/S/E/W
#include "tftControl.h"				// Enable / disable displays
#include "currentWeatherImage.h"	// Draw main weather image
#include "forecastWeatherImage.h"	// Draw main weather image
#include "drawBitmap.h"				// Draw bitmaps
#include "drawBarGraph.h"			// Draw bar graphs
#include "drawCompass.h"			// Draw compass

#include "colours.h"                // Colours
#include "screenLayout.h"			// Screen layout
#include "iconsStart.h"				// Start up bitmaps
#include "icons.h"					// Screen bitmaps
#include "iconsAtmosphere.h"		// Weather bitmaps
#include "iconsClear.h"				// Weather bitmaps
#include "iconsClouds.h"			// Weather bitmaps
#include "iconsDrizzle.h"			// Weather bitmaps
#include "iconsRain.h"				// Weather bitmaps
#include "iconsSnow.h"				// Weather bitmaps
#include "iconsThunderstorms.h"		// Weather bitmaps
#include "iconsMoonPhase.h"			// Moonphase bitmaps

// TFT SPI Interface for ESP32.

#define VSPI_MISO   19 // MISO - Not needed for display(s)
#define VSPI_MOSI   23 // MOSI
#define VSPI_CLK    18 // SCK
#define VSPI_DC     4  // DC
#define VSPI_RST    13 // RST							

#define VSPI_CS0	36 // This is set to an erroneous pin as to not confuse manual chip selects using digital writes
#define VSPI_CS1    5  // Screen one chip select
#define VSPI_CS2    17 // Screen two chip select
#define VSPI_CS3    16 // Screen three chip select
#define VSPI_CS4    15 // Screen four chip select
#define VSPI_CS5    26 // Screen five chip select
#define VSPI_CS6    25 // Screen six chip select
#define VSPI_CS7    33 // Screen seven chip select
#define VSPI_CS8    32 // Screen eight chip select

#define espTouchBackLight	14			// ESP touch pin - Backlight switch
#define espTouchDailyHourly	12			// ESP touch pin - Daily / hourly switch
#define espTouchMetricImperial	2		// ESP touch pin - Metric / imperial switch

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

// Http error status.

byte sensorT = 0;

// Configure switches.

boolean dailyHourlyState = false;
boolean dailyHourlyToggled = false;

boolean switchTwoState = false;
boolean switchTwoToggled = false;

boolean backLightState = true;
boolean backLightToggled = false;

boolean metricImperialState = true;
boolean metricImperialToggled = false;

// VSPI Class (default).

Adafruit_ILI9341 tft = Adafruit_ILI9341(VSPI_CS0, VSPI_DC, VSPI_RST); // CS0 is a dummy pin

// MCP23008.

Adafruit_MCP23X08 mcp;

// Configure time settings.

ESP32Time rtc;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
int			daylightOffset_sec = 0;

// Web Server configuration.

AsyncWebServer server(80);			// Create AsyncWebServer object on port 80
AsyncEventSource events("/events");	// Create an Event Source on /events

// Search for parameter in HTTP POST request.

const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "openwk";
const char* PARAM_INPUT_4 = "latitude";
const char* PARAM_INPUT_5 = "longitude";

// Variables to save values from HTML form.

String ssid;			// WiFi SSID
String pass;			// WiFi password
String openwk;			// Openweather API key

// File paths to save input values permanently.

const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* openWKPath = "/openwk.txt";
const char* latitudePath = "/latitude.txt";
const char* longitudePath = "/longitude.txt";

// Network variables.

IPAddress localIP;
IPAddress Gateway;
IPAddress Subnet;
IPAddress dns1;

// Json Variable to Hold Sensor Readings.

String jsonBuffer;
DynamicJsonDocument weatherData(35788); //35788

// Forecast weather location.

String latitude;	// "52.628101";
String longitude;	// "1.299350";

// Timer debounce.

long lastDebounceTimeBkL = 0;				// Debounce timing for backlight
const long debounceDelayBkL = 200;			// Debounce delay timing for backlight

long lastDebounceTimeDH = 0;				// Debounce timing for day hourly change
const long debounceDelayDH = 200;			// Debounce delay timing for day hourly change

long lastDebounceTimeMI = 0;				// Debounce timing for metric imperial change
const long debounceDelayMI = 200;			// Debounce delay timing for metric imperial change

// Timer variables (check Open Weather).

unsigned long intervalTGetData = 0;
const unsigned long intervalPGetData = 900000; // Reset to every 15 minutes

// Timer variables (update time).

unsigned long intervalUT = 0;
const unsigned long intervalPUT = 60000;	// Update every 1 minute

// Timer variables (check wifi).

unsigned long wiFiR = 180000;				// WiFi retry (wiFiR) to attempt connecting to the last known WiFi if connection lost
unsigned long wiFiRI = 60000;				// WiFi retry Interval (wiFiRI) used to retry connecting to the last known WiFi if connection lost
unsigned long previousMillis = 0;			// Used in the WiFI Init function
const long interval = 10000;				// Interval to wait for Wi-Fi connection (milliseconds)

// Timer variables (changing temperature readings).

boolean flagTempDisplayChange = false;
unsigned long intervalTTempRotate = 0;
const unsigned long intervalPTemp = 5000;

// Timer variables (sleep screens).

unsigned long sleepScreenTime;
const unsigned long sleepScreensP = 600000;

// Touch variables.

const int touchThreshold = 20;
int touchValueBackLight = 25;
int touchValueDailyHourly = 25;
int touchValueMetrixImperial = 25;

// Weather variables.

#define windMph (2.237)

// Day or night flag for main current image setting.

boolean dayNight = false;

// Wind heading array.

byte heading;
char headingArray[8][3] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };

// Wind speed description array.

char windSpeedDescription[14][17] = { " calm", " light air", " light breeze", " gentle breeze", " moderate breeze", " freesh breeze", " strong breeze", " near gale", " gale", " sever gale", " storm", " violent storm", " hurricane" };

// Current weather variables.

unsigned long currentWeatherTime;
double tempNow;
double feelsLikeTempNow;
unsigned int pressureNow;
double humidityNow;
double windSpeedNow;
int windDirectionNow;
byte uvIndexNow;
unsigned int currentWeatherNow;
String weatherDesCurrent;
double rainPopNow;
double rainLevelNow;
double snowLevelNow;
unsigned long sunRiseNow;
unsigned long sunSetNow;
double moonPhaseNow;

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
double rainPopFc1;
double rainLevelFc1;
double snowLevelFc1;
unsigned long sunRiseFc1;
unsigned long sunSetFc1;
double moonPhaseFc1;

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
double rainPopFc2;
double rainLevelFc2;
double snowLevelFc2;
unsigned long sunRiseFc2;
unsigned long sunSetFc2;
double moonPhaseFc2;

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
double rainPopFc3;
double rainLevelFc3;
double snowLevelFc3;
unsigned long sunRiseFc3;
unsigned long sunSetFc3;
double moonPhaseFc3;

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
double rainPopFc4;
double rainLevelFc4;
double snowLevelFc4;
unsigned long sunRiseFc4;
unsigned long sunSetFc4;
double moonPhaseFc4;

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
double rainPopFc5;
double rainLevelFc5;
double snowLevelFc5;
unsigned long sunRiseFc5;
unsigned long sunSetFc5;
double moonPhaseFc5;

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
double rainPopFc6;
double rainLevelFc6;
double snowLevelFc6;
unsigned long sunRiseFc6;
unsigned long sunSetFc6;
double moonPhaseFc6;

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
double rainPopFc7;
double rainLevelFc7;
double snowLevelFc7;
unsigned long sunRiseFc7;
unsigned long sunSetFc7;
double moonPhaseFc7;

// Hourly forecast weather variables.

double temperatureHr1;
double temperatureHr2;
double temperatureHr3;
double temperatureHr4;
double temperatureHr5;
double temperatureHr6;
double temperatureHr7;
double temperatureHr8;

double rainFallHr1;
double rainFallHr2;
double rainFallHr3;
double rainFallHr4;
double rainFallHr5;
double rainFallHr6;
double rainFallHr7;
double rainFallHr8;

double rainChartScale;

unsigned int pressureHr1;
unsigned int pressureHr2;
unsigned int pressureHr3;
unsigned int pressureHr4;
unsigned int pressureHr5;
unsigned int pressureHr6;
unsigned int pressureHr7;
unsigned int pressureHr8;

// Pressure description array.

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

	delay(500);

	disableVSPIScreens();

	// Check if settings are available to connect to WiFi.

	//	if (ssid == "" || ip == "") {						Removed this code and replaced with below, from Static to DHCP IP config.
	//	Serial.println("Undefined SSID or IP address.");
	//	return false;
	//	}

	if (ssid == "" || pass == "" || openwk == "") {
		// Serial.println("Undefined SSID, Password or Open Weather Key.");
		return false;
	}

	WiFi.mode(WIFI_STA);

	// localIP.fromString(ip.c_str());						Removed this code as static to DHCP for IP config.
	// Gateway.fromString(gateway.c_str());					Removed this code as static to DHCP for IP config.
	// Subnet.fromString(subnet.c_str());					Removed this code as static to DHCP for IP config.
	// dns1.fromString(dns.c_str());						Removed this code as static to DHCP for IP config.

	if (!WiFi.config(localIP, Gateway, Subnet, dns1)) {
		Serial.println("STA Failed to configure");
		return false;
	}

	WiFi.begin(ssid.c_str(), pass.c_str());

	// Serial.println("Connecting to WiFi...");

	// If ESP32 inits successfully in station mode, recolour WiFi to red.

	enableScreen1();

	drawBitmap(tft, WIFI_ICON_Y, WIFI_ICON_X, wiFiRed, WIFI_ICON_W, WIFI_ICON_H);

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
			disableVSPIScreens();
			return false;
		}

	}

	// Serial.println(WiFi.localIP());
	// Serial.println("");
	// Serial.print("RRSI: ");
	// Serial.println(WiFi.RSSI());

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

	delay(2000);	// Wait a moment.

	tft.fillScreen(WHITE);

	disableVSPIScreens();

	return true;

} // Close function.

/*-----------------------------------------------------------------*/

// Get data from Open Weather.

String httpGETRequest(const char* serverName) {

	WiFiClient client;
	HTTPClient http;

	// Your Domain name with URL path or IP address with path.

	http.begin(client, serverName);

	// Send HTTP POST request.

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

	// HTTP Response codes:
	// 0 = Blue = 429 = Calls exceeds account limit
	// 1 = Red = 401 = API key problem
	// 2 = Yellow = 404 = Incorrect city name, ZIP or city ID
	// 3 = Orange = 50X = You have to contact Open Weather
	// 4 = Green = 200 = Correct reponse

	if (httpResponseCode == 429) {

		sensorT = 0; // Blue
	}

	else if (httpResponseCode == 401) {

		sensorT = 1; // Red
	}

	else if (httpResponseCode == 404) {

		sensorT = 2; // Yellow
	}

	else if (httpResponseCode > 499 && httpResponseCode < 505) {

		sensorT = 3; // Orange
	}

	else if (httpResponseCode == 200) {

		sensorT = 4; // Green
	}

	else {
		sensorT = 5; // Green
	}

	// Free resources.

	http.end();

	drawHttpError();

	return payload;

} // Close function.

/*-----------------------------------------------------------------*/

// Return JSON String from sensor Readings.

// Not using in this sketch as not running a webserver.

String getJSONReadings() {

	// String jsonString = JSON.stringify(readings);
	// return jsonString;

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

		// Serial.println("Failed, time set to default.");

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

	// Serial.println("");
	// Serial.println(&timeinfo, "%A, %B %d %Y %H:%M %Z");
	// Serial.println("");

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

	// Enable I2C Devices.

	// Serial.println(F("MCP23008 start"));

	mcp.begin_I2C();

	// Set pin modes.

	pinMode(VSPI_CS1, OUTPUT); //VSPI CS
	pinMode(VSPI_CS2, OUTPUT); //VSPI CS
	pinMode(VSPI_CS3, OUTPUT); //VSPI CS
	pinMode(VSPI_CS4, OUTPUT); //VSPI CS
	pinMode(VSPI_CS5, OUTPUT); //VSPI CS
	pinMode(VSPI_CS6, OUTPUT); //VSPI CS
	pinMode(VSPI_CS7, OUTPUT); //VSPI CS
	pinMode(VSPI_CS8, OUTPUT); //VSPI CS
	pinMode(espTouchBackLight, INPUT);		//Touch CS
	pinMode(espTouchDailyHourly, INPUT);	//Touch CS
	pinMode(espTouchMetricImperial, INPUT); //Touch CS

	touchValueBackLight = touchRead(espTouchBackLight);
	touchValueDailyHourly = touchRead(espTouchDailyHourly);
	touchValueMetrixImperial = touchRead(espTouchMetricImperial);

	// Serial.println("Touch Values:");
	// Serial.println(touchValueBackLight);
	// Serial.println(touchValueDailyHourly);
	// Serial.println(touchValueMetrixImperial);
	// Serial.println("");

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

	// Send screen configuration.

	tft.begin(40000000); // 40000000 27000000
	tft.setRotation(3);
	tft.setCursor(0, 0);

	// Reset all chip selects high once again.

	disableVSPIScreens();

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
	drawBitmap(tft, 48, 96, fswindsock, 128, 128);
	disableVSPIScreens();

	enableScreen3();
	drawBitmap(tft, 48, 96, fsatmospheric, 128, 128);
	disableVSPIScreens();

	enableScreen4();
	drawBitmap(tft, 48, 96, fshumidity, 128, 128);
	disableVSPIScreens();

	enableScreen5();
	drawBitmap(tft, 48, 96, fsrainfall, 128, 128);
	disableVSPIScreens();

	enableScreen6();
	drawBitmap(tft, 48, 96, fstemperature, 128, 128);
	disableVSPIScreens();

	enableScreen7();
	drawBitmap(tft, 48, 96, fsuv, 128, 128);
	disableVSPIScreens();

	enableScreen8();
	drawBitmap(tft, 48, 96, fsweathervain, 128, 128);
	disableVSPIScreens();

	delay(4000);

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

	// ssid = "";									// Remove these lines before final build.
	// pass = "";
	// ip = "192.168.1.200";
	// subnet = "255.255.255.0";
	// gateway = "192.168.1.254";
	// dns = "192.168.1.254";

	// writeFile(SPIFFS, ssidPath, ssid.c_str());
	// writeFile(SPIFFS, passPath, pass.c_str());
	// writeFile(SPIFFS, ipPath, ip.c_str());
	// writeFile(SPIFFS, subnetPath, subnet.c_str());
	// writeFile(SPIFFS, gatewayPath, gateway.c_str());
	// writeFile(SPIFFS, dnsPath, dns.c_str());

	// ssid = "blank";
	// pass = "blank";
	// ip = "blank";
	// subnet = "blank";
	// gateway = "blank";
	// dns = "blank";
	// openwk= "Test";

	// writeFile(SPIFFS, ssidPath, ssid.c_str());
	// writeFile(SPIFFS, passPath, pass.c_str());
	// writeFile(SPIFFS, ipPath, ip.c_str());
	// writeFile(SPIFFS, subnetPath, subnet.c_str());
	// writeFile(SPIFFS, gatewayPath, gateway.c_str());
	// writeFile(SPIFFS, dnsPath, dns.c_str());
	// writeFile(SPIFFS, openWKPath, openwk.c_str());

	// Serial.println();

	ssid = readFile(SPIFFS, ssidPath);
	pass = readFile(SPIFFS, passPath);
	openwk = readFile(SPIFFS, openWKPath);
	latitude = readFile(SPIFFS, latitudePath);
	longitude = readFile(SPIFFS, longitudePath);

	// Serial.println();
	// Serial.println(ssid);
	// Serial.println(pass);
	// Serial.println(ip);
	// Serial.println(subnet);
	// Serial.println(gateway);
	// Serial.println(dns);
	// Serial.println(openwk);
	// Serial.println(latitude);
	// Serial.println(longitude);
	// Serial.println();

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
				//Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
			}
			});

		server.addHandler(&events);

		server.begin();
	}

	else

	{	// WiFi title page.

		wiFiTitle();

		// Initialize the ESP32 in Access Point mode, recolour to WiFI red.

		enableScreen1();

		drawBitmap(tft, WIFI_ICON_Y, WIFI_ICON_X, wiFiRed, WIFI_ICON_W, WIFI_ICON_H);

		disableVSPIScreens();

		// Set Access Point.
		// Serial.println("Setting AP (Access Point)");

		// NULL sets an open Access Point.

		WiFi.softAP("WIFI-MANAGER", NULL);

		IPAddress IP = WiFi.softAPIP();
		// Serial.print("AP IP address: ");
		// Serial.println(IP);

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
					// Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());

					// HTTP POST pass value
					if (p->name() == PARAM_INPUT_2) {
						pass = p->value().c_str();
						Serial.print("Password set to: ");
						Serial.println(pass);
						// Write file to save value
						writeFile(SPIFFS, passPath, pass.c_str());
					}
					// Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());

					// HTTP POST ip value
					if (p->name() == PARAM_INPUT_3) {
						openwk = p->value().c_str();
						Serial.print("Open Weather API Set to: ");
						Serial.println(openwk);
						// Write file to save value
						writeFile(SPIFFS, openWKPath, openwk.c_str());
					}
					// Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());

					if (p->name() == PARAM_INPUT_4) {
						latitude = p->value().c_str();
						Serial.print("Latitude Set to: ");
						Serial.println(latitude);
						// Write file to save value
						writeFile(SPIFFS, latitudePath, latitude.c_str());
					}
					// Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());

					if (p->name() == PARAM_INPUT_5) {
						longitude = p->value().c_str();
						Serial.print("Longitude Set to: ");
						Serial.println(longitude);
						// Write file to save value
						writeFile(SPIFFS, longitudePath, longitude.c_str());
					}
					// Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
				}
			}

			request->send(200, "text/plain", "Done. ESP will restart, connect to your router");
			// request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
			delay(3000);

			// After saving the parameters, restart the ESP32.

			ESP.restart();
			});

		server.begin();

		wiFiApMode();

		unsigned long previousMillis = millis();
		unsigned long interval = 300000;

		while (1) {

			// Hold from starting loop while in AP mode.

			unsigned long currentMillis = millis();

			// Restart after 5 minutes in case of failed reconnection with correc WiFi details.

			if (currentMillis - previousMillis >= interval) {

				ESP.restart();

			}

		}

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

	// Layout screens & obtain data.

	dataDisplayCurrentWeatherLayout();
	dataDisplayCurrentWeatherAlternateData();
	dataDisplayCurrentWeatherDayX(moonPhaseNow);

	dataDisplayForecastWeatherLayoutDayX(tft, 2);
	dataDisplayForecastAlternateData(tft, 2, forecastTimeFc1, weatherDesFc1, tempDayFc1, tempNightFc1, tempMinFc1, tempMaxFc1, rainLevelFc1, rainPopFc1, windSpeedFc1, snowLevelFc1);
	dataDisplayForecastDayX(tft, 2, forecastTimeFc1, pressureFc1, humidityFc1, windSpeedFc1, windDirectionFc1, uvIndexFc1, weatherLabelFc1, rainLevelFc1, snowLevelFc1, sunRiseFc1, sunSetFc1, moonPhaseFc1);

	dataDisplayForecastWeatherLayoutDayX(tft, 3);
	dataDisplayForecastAlternateData(tft, 3, forecastTimeFc2, weatherDesFc2, tempDayFc2, tempNightFc2, tempMinFc2, tempMaxFc2, rainLevelFc2, rainPopFc2, windSpeedFc2, snowLevelFc2);
	dataDisplayForecastDayX(tft, 3, forecastTimeFc2, pressureFc2, humidityFc2, windSpeedFc2, windDirectionFc2, uvIndexFc2, weatherLabelFc2, rainLevelFc2, snowLevelFc2, sunRiseFc2, sunSetFc2, moonPhaseFc2);

	dataDisplayForecastWeatherLayoutDayX(tft, 4);
	dataDisplayForecastAlternateData(tft, 4, forecastTimeFc3, weatherDesFc3, tempDayFc3, tempNightFc3, tempMinFc3, tempMaxFc3, rainLevelFc3, rainPopFc3, windSpeedFc3, snowLevelFc3);
	dataDisplayForecastDayX(tft, 4, forecastTimeFc3, pressureFc3, humidityFc3, windSpeedFc3, windDirectionFc3, uvIndexFc3, weatherLabelFc3, rainLevelFc3, snowLevelFc3, sunRiseFc3, sunSetFc3, moonPhaseFc3);

	dataDisplayForecastWeatherLayoutDayX(tft, 5);
	dataDisplayForecastAlternateData(tft, 5, forecastTimeFc4, weatherDesFc4, tempDayFc4, tempNightFc4, tempMinFc4, tempMaxFc4, rainLevelFc4, rainPopFc4, windSpeedFc4, snowLevelFc4);
	dataDisplayForecastDayX(tft, 5, forecastTimeFc4, pressureFc4, humidityFc4, windSpeedFc4, windDirectionFc4, uvIndexFc4, weatherLabelFc4, rainLevelFc4, snowLevelFc4, sunRiseFc4, sunSetFc4, moonPhaseFc4);

	dataDisplayForecastWeatherLayoutDayX(tft, 6);
	dataDisplayForecastAlternateData(tft, 6, forecastTimeFc5, weatherDesFc5, tempDayFc5, tempNightFc5, tempMinFc5, tempMaxFc5, rainLevelFc5, rainPopFc5, windSpeedFc5, snowLevelFc5);
	dataDisplayForecastDayX(tft, 6, forecastTimeFc5, pressureFc5, humidityFc5, windSpeedFc5, windDirectionFc5, uvIndexFc5, weatherLabelFc5, rainLevelFc5, snowLevelFc5, sunRiseFc5, sunSetFc5, moonPhaseFc5);

	dataDisplayForecastWeatherLayoutDayX(tft, 7);
	dataDisplayForecastAlternateData(tft, 7, forecastTimeFc6, weatherDesFc6, tempDayFc6, tempNightFc6, tempMinFc6, tempMaxFc6, rainLevelFc6, rainPopFc6, windSpeedFc6, snowLevelFc6);
	dataDisplayForecastDayX(tft, 7, forecastTimeFc6, pressureFc6, humidityFc6, windSpeedFc6, windDirectionFc6, uvIndexFc6, weatherLabelFc6, rainLevelFc6, snowLevelFc6, sunRiseFc6, sunSetFc6, moonPhaseFc6);

	dataDisplayForecastWeatherLayoutDayX(tft, 8);
	dataDisplayForecastAlternateData(tft, 8, forecastTimeFc7, weatherDesFc7, tempDayFc7, tempNightFc7, tempMinFc7, tempMaxFc7, rainLevelFc7, rainPopFc7, windSpeedFc7, snowLevelFc7);
	dataDisplayForecastDayX(tft, 8, forecastTimeFc7, pressureFc7, humidityFc7, windSpeedFc7, windDirectionFc7, uvIndexFc7, weatherLabelFc7, rainLevelFc7, snowLevelFc7, sunRiseFc7, sunSetFc7, moonPhaseFc7);

	sleepScreenTime = millis() + sleepScreensP;

	// Http error circle.

	tft.drawCircle(SENSOR_ICON_X, SENSOR_ICON_Y, SENSOR_ICON_R, BLACK);

} // Close setup.

/*-----------------------------------------------------------------*/

void loop() {

	// Read capacitive touch.

	touchValueBackLight = touchRead(espTouchBackLight);

	// Set back light switch.

	if (touchValueBackLight < touchThreshold) {

		if ((millis() - lastDebounceTimeBkL) > debounceDelayBkL)
		{
			if (backLightState == false) {

				backLightState = true;
				backLightToggled = true;
				lastDebounceTimeBkL = millis();
			}

			else if (backLightState == true) {

				backLightState = false;
				backLightToggled = true;
				lastDebounceTimeBkL = millis();
			}

			sleepScreenTime = millis();
		}
	}

	// Read capacitive touch.

	touchValueDailyHourly = touchRead(espTouchDailyHourly);
	
	// Set daily or hourly view.

	if (touchValueDailyHourly < touchThreshold) {

		if ((millis() - lastDebounceTimeDH) > debounceDelayDH)
		{
			if (dailyHourlyState == false) {

				dailyHourlyState = true;
				dailyHourlyToggled = true;
				lastDebounceTimeDH = millis();
			}

			else if (dailyHourlyState == true) {

				dailyHourlyState = false;
				dailyHourlyToggled = true;
				lastDebounceTimeDH = millis();
			}

			sleepScreenTime = millis();
		}
	}

	// Read capactive touch.
	
	touchValueMetrixImperial = touchRead(espTouchMetricImperial);

	// Set metrix or imperial.
	
	if (touchValueMetrixImperial < touchThreshold) {

		if ((millis() - lastDebounceTimeMI) > debounceDelayMI)
		{
			if (metricImperialState == false) {

				metricImperialState = true;
				metricImperialToggled = true;
				lastDebounceTimeMI = millis();
			}

			else if (metricImperialState == true) {

				metricImperialState = false;
				metricImperialToggled = true;
				lastDebounceTimeMI = millis();
			}

			sleepScreenTime = millis();
		}

	}

	// Check if settings to be reset.

	if (touchValueBackLight < touchThreshold && touchValueDailyHourly < touchThreshold && touchValueMetrixImperial < touchThreshold) {

		factoryReset();

	}

	// Update time each minute.

	if (millis() >= intervalUT + intervalPUT) {

		printLocalTime();

		intervalUT = millis();
	
	}

	// Check Open Weather at regular intervals (1 hour as free license is limited per day and month).

	if (millis() >= intervalTGetData + intervalPGetData) {

		// Update time.

		printLocalTime();

		// Update weather data.

		getWeatherData();

		// Update displays.

		dataDisplayCurrentWeatherLayout();
		dataDisplayCurrentWeatherAlternateData();
		dataDisplayCurrentWeatherDayX(moonPhaseNow);

		dataDisplayForecastWeatherLayoutDayX(tft, 2);
		dataDisplayForecastAlternateData(tft, 2, forecastTimeFc1, weatherDesFc1, tempDayFc1, tempNightFc1, tempMinFc1, tempMaxFc1, rainLevelFc1, rainPopFc1, windSpeedFc1, snowLevelFc1);
		dataDisplayForecastDayX(tft, 2, forecastTimeFc1, pressureFc1, humidityFc1, windSpeedFc1, windDirectionFc1, uvIndexFc1, weatherLabelFc1, rainLevelFc1, snowLevelFc1, sunRiseFc1, sunSetFc1, moonPhaseFc1);

		dataDisplayForecastWeatherLayoutDayX(tft, 3);
		dataDisplayForecastAlternateData(tft, 3, forecastTimeFc2, weatherDesFc2, tempDayFc2, tempNightFc2, tempMinFc2, tempMaxFc2, rainLevelFc2, rainPopFc2, windSpeedFc2, snowLevelFc2);
		dataDisplayForecastDayX(tft, 3, forecastTimeFc2, pressureFc2, humidityFc2, windSpeedFc2, windDirectionFc2, uvIndexFc2, weatherLabelFc2, rainLevelFc2, snowLevelFc2, sunRiseFc2, sunSetFc2, moonPhaseFc2);

		dataDisplayForecastWeatherLayoutDayX(tft, 4);
		dataDisplayForecastAlternateData(tft, 4, forecastTimeFc3, weatherDesFc3, tempDayFc3, tempNightFc3, tempMinFc3, tempMaxFc3, rainLevelFc4, rainPopFc3, windSpeedFc3, snowLevelFc3);
		dataDisplayForecastDayX(tft, 4, forecastTimeFc3, pressureFc3, humidityFc3, windSpeedFc3, windDirectionFc3, uvIndexFc3, weatherLabelFc3, rainLevelFc3, snowLevelFc3, sunRiseFc3, sunSetFc3, moonPhaseFc3);

		if (dailyHourlyState == false) {

			dataDisplayForecastWeatherLayoutDayX(tft, 5);
			dataDisplayForecastAlternateData(tft, 5, forecastTimeFc4, weatherDesFc4, tempDayFc4, tempNightFc4, tempMinFc4, tempMaxFc4, rainLevelFc4, rainPopFc4, windSpeedFc4, snowLevelFc4);
			dataDisplayForecastDayX(tft, 5, forecastTimeFc4, pressureFc4, humidityFc4, windSpeedFc4, windDirectionFc4, uvIndexFc4, weatherLabelFc4, rainLevelFc4, snowLevelFc4, sunRiseFc4, sunSetFc4, moonPhaseFc4);

			dataDisplayForecastWeatherLayoutDayX(tft, 6);
			dataDisplayForecastAlternateData(tft, 6, forecastTimeFc5, weatherDesFc5, tempDayFc5, tempNightFc5, tempMinFc5, tempMaxFc5, rainLevelFc5, rainPopFc5, windSpeedFc5, snowLevelFc5);
			dataDisplayForecastDayX(tft, 6, forecastTimeFc5, pressureFc5, humidityFc5, windSpeedFc5, windDirectionFc5, uvIndexFc5, weatherLabelFc5, rainLevelFc5, snowLevelFc5, sunRiseFc5, sunSetFc5, moonPhaseFc5);

			dataDisplayForecastWeatherLayoutDayX(tft, 7);
			dataDisplayForecastAlternateData(tft, 7, forecastTimeFc6, weatherDesFc6, tempDayFc6, tempNightFc6, tempMinFc6, tempMaxFc6, rainLevelFc6, rainPopFc6, windSpeedFc6, snowLevelFc6);
			dataDisplayForecastDayX(tft, 7, forecastTimeFc6, pressureFc6, humidityFc6, windSpeedFc6, windDirectionFc6, uvIndexFc6, weatherLabelFc6, rainLevelFc6, snowLevelFc6, sunRiseFc6, sunSetFc6, moonPhaseFc6);

			dataDisplayForecastWeatherLayoutDayX(tft, 8);
			dataDisplayForecastAlternateData(tft, 8, forecastTimeFc7, weatherDesFc7, tempDayFc7, tempNightFc7, tempMinFc7, tempMaxFc7, rainLevelFc7, rainPopFc7, windSpeedFc7, snowLevelFc7);
			dataDisplayForecastDayX(tft, 8, forecastTimeFc7, pressureFc7, humidityFc7, windSpeedFc7, windDirectionFc7, uvIndexFc7, weatherLabelFc7, rainLevelFc7, snowLevelFc7, sunRiseFc7, sunSetFc7, moonPhaseFc7);

		}

		// Update displays as they are set, i.e. Daily or Hourly views

		else {

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

		}

		intervalTGetData = millis();

	}

	// Cycle displays for temperature, rain fall and likelyhood of precipitation.

	if (millis() >= intervalTTempRotate + intervalPTemp) {

		// Alternate temperature displays, min, max, day, night.

		if (flagTempDisplayChange == true) {

			flagTempDisplayChange = false;
		}

		else flagTempDisplayChange = true;

		dataDisplayCurrentWeatherAlternateData();

		// Check if daily / hourly flag set and change displays.

		if (dailyHourlyState == false) {

			if (dailyHourlyToggled == true) {

				enableScreen5();
				tft.fillScreen(WHITE);
				dataDisplayForecastWeatherLayoutDayX(tft, 5);
				dataDisplayForecastDayX(tft, 5, forecastTimeFc4, pressureFc4, humidityFc4, windSpeedFc4, windDirectionFc4, uvIndexFc4, weatherLabelFc4, rainLevelFc4, snowLevelFc4, sunRiseFc4, sunSetFc4, moonPhaseFc4);

				enableScreen6();
				tft.fillScreen(WHITE);
				dataDisplayForecastWeatherLayoutDayX(tft, 6);
				dataDisplayForecastDayX(tft, 6, forecastTimeFc5, pressureFc5, humidityFc5, windSpeedFc5, windDirectionFc5, uvIndexFc5, weatherLabelFc5, rainLevelFc5, snowLevelFc5, sunRiseFc5, sunSetFc5, moonPhaseFc5);

				enableScreen7();
				tft.fillScreen(WHITE);
				dataDisplayForecastWeatherLayoutDayX(tft, 7);
				dataDisplayForecastDayX(tft, 7, forecastTimeFc6, pressureFc6, humidityFc6, windSpeedFc6, windDirectionFc6, uvIndexFc6, weatherLabelFc6, rainLevelFc6, snowLevelFc6, sunRiseFc6, sunSetFc6, moonPhaseFc6);

				enableScreen8();
				tft.fillScreen(WHITE);
				dataDisplayForecastWeatherLayoutDayX(tft, 8);
				dataDisplayForecastDayX(tft, 8, forecastTimeFc7, pressureFc7, humidityFc7, windSpeedFc7, windDirectionFc7, uvIndexFc7, weatherLabelFc7, rainLevelFc7, snowLevelFc7, sunRiseFc7, sunSetFc7, moonPhaseFc7);

				dailyHourlyToggled = false;

			}

			dataDisplayForecastAlternateData(tft, 5, forecastTimeFc4, weatherDesFc4, tempDayFc4, tempNightFc4, tempMinFc4, tempMaxFc4, rainLevelFc4, rainPopFc4, windSpeedFc4, snowLevelFc4);
			dataDisplayForecastAlternateData(tft, 6, forecastTimeFc5, weatherDesFc5, tempDayFc5, tempNightFc5, tempMinFc5, tempMaxFc5, rainLevelFc5, rainPopFc5, windSpeedFc5, snowLevelFc5);
			dataDisplayForecastAlternateData(tft, 7, forecastTimeFc6, weatherDesFc6, tempDayFc6, tempNightFc6, tempMinFc6, tempMaxFc6, rainLevelFc6, rainPopFc6, windSpeedFc6, snowLevelFc6);
			dataDisplayForecastAlternateData(tft, 8, forecastTimeFc7, weatherDesFc7, tempDayFc7, tempNightFc7, tempMinFc7, tempMaxFc7, rainLevelFc7, rainPopFc7, windSpeedFc7, snowLevelFc7);

		}

		dataDisplayForecastAlternateData(tft, 2, forecastTimeFc1, weatherDesFc1, tempDayFc1, tempNightFc1, tempMinFc1, tempMaxFc1, rainLevelFc1, rainPopFc1, windSpeedFc1, snowLevelFc1);
		dataDisplayForecastAlternateData(tft, 3, forecastTimeFc2, weatherDesFc2, tempDayFc2, tempNightFc2, tempMinFc2, tempMaxFc2, rainLevelFc2, rainPopFc2, windSpeedFc2, snowLevelFc2);
		dataDisplayForecastAlternateData(tft, 4, forecastTimeFc3, weatherDesFc3, tempDayFc3, tempNightFc3, tempMinFc3, tempMaxFc3, rainLevelFc3, rainPopFc3, windSpeedFc3, snowLevelFc3);

		intervalTTempRotate = millis();

	}

	if (dailyHourlyState == true && dailyHourlyToggled == true) {

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

		dailyHourlyToggled = false;

	}

	else if (dailyHourlyState == false && dailyHourlyToggled == true) {

		enableScreen5();
		tft.fillScreen(WHITE);
		dataDisplayForecastWeatherLayoutDayX(tft, 5);
		dataDisplayForecastDayX(tft, 5, forecastTimeFc4, pressureFc4, humidityFc4, windSpeedFc4, windDirectionFc4, uvIndexFc4, weatherLabelFc4, rainLevelFc4, snowLevelFc4, sunRiseFc4, sunSetFc4, moonPhaseFc4);
		dataDisplayForecastAlternateData(tft, 5, forecastTimeFc4, weatherDesFc4, tempDayFc4, tempNightFc4, tempMinFc4, tempMaxFc4, rainLevelFc4, rainPopFc4, windSpeedFc4, snowLevelFc4);

		enableScreen6();
		tft.fillScreen(WHITE);
		dataDisplayForecastWeatherLayoutDayX(tft, 6);
		dataDisplayForecastDayX(tft, 6, forecastTimeFc5, pressureFc5, humidityFc5, windSpeedFc5, windDirectionFc5, uvIndexFc5, weatherLabelFc5, rainLevelFc5, snowLevelFc5, sunRiseFc5, sunSetFc5, moonPhaseFc5);
		dataDisplayForecastAlternateData(tft, 6, forecastTimeFc5, weatherDesFc5, tempDayFc5, tempNightFc5, tempMinFc5, tempMaxFc5, rainLevelFc5, rainPopFc5, windSpeedFc5, snowLevelFc5);

		enableScreen7();
		tft.fillScreen(WHITE);
		dataDisplayForecastWeatherLayoutDayX(tft, 7);
		dataDisplayForecastDayX(tft, 7, forecastTimeFc6, pressureFc6, humidityFc6, windSpeedFc6, windDirectionFc6, uvIndexFc6, weatherLabelFc6, rainLevelFc6, snowLevelFc6, sunRiseFc6, sunSetFc6, moonPhaseFc6);
		dataDisplayForecastAlternateData(tft, 7, forecastTimeFc6, weatherDesFc6, tempDayFc6, tempNightFc6, tempMinFc6, tempMaxFc6, rainLevelFc6, rainPopFc6, windSpeedFc6, snowLevelFc6);

		enableScreen8();
		tft.fillScreen(WHITE);
		dataDisplayForecastWeatherLayoutDayX(tft, 8);
		dataDisplayForecastDayX(tft, 8, forecastTimeFc7, pressureFc7, humidityFc7, windSpeedFc7, windDirectionFc7, uvIndexFc7, weatherLabelFc7, rainLevelFc7, snowLevelFc7, sunRiseFc7, sunSetFc7, moonPhaseFc7);
		dataDisplayForecastAlternateData(tft, 8, forecastTimeFc7, weatherDesFc7, tempDayFc7, tempNightFc7, tempMinFc7, tempMaxFc7, rainLevelFc7, rainPopFc7, windSpeedFc7, snowLevelFc7);

		dailyHourlyToggled = false;

	}

	// Detect if metric / imperial change.

	if (metricImperialToggled == true) {

		metricImperialSwitch();

		metricImperialToggled = false;
	}

	// Detect if backlight control change.

	if (backLightToggled == true) {

		controlBackLight();

		backLightToggled = false;

	}

	// Auto turn off displays after sleep period is reached.

	if (millis() >= sleepScreenTime + sleepScreensP) {

		backLightState = false;
		controlBackLight();

	}

	// Retry connecting to WiFi if connection is lost.

	unsigned long wiFiRC = millis();

	if ((WiFi.status() != WL_CONNECTED) && (wiFiRC + wiFiR >= wiFiRI)) {

		WiFi.disconnect();
		WiFi.reconnect();
		wiFiRI = wiFiRC;
	}

} // Close loop.

/*-----------------------------------------------------------------*/

// Get forecast data.

void getWeatherData() {

	// Open Weather Request. 

	String serverPath = "http://api.openweathermap.org/data/2.5/onecall?lat=" + latitude + "&lon=" + longitude + "&APPID=" + openwk + "&units=metric" + "&exclude=minutely,alerts";  //,hourly

	// String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey + "&units=metric";
	// String serverPath = "http://api.openweathermap.org/data/2.5/onecall?lat=" + latitude + "&lon=" + longitude + "&APPID=" + openWeatherMapApiKey + "&units=metric" + "&exclude=minutely,hourly,alerts";

	jsonBuffer = httpGETRequest(serverPath.c_str());
	Serial.println("");
	//Serial.println(jsonBuffer);
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
	rainPopNow = (myObject["daily"][0]["pop"]["1h"]);
	rainLevelNow = (myObject["current"]["rain"]["1h"]);
	sunRiseNow = (myObject["current"]["sunrise"]);
	sunSetNow = (myObject["current"]["sunset"]);
	moonPhaseNow = (myObject["daily"][0]["moon_phase"]);

	weatherDesCurrent.clear();
	serializeJson((myObject["current"]["weather"][0]["description"]), weatherDesCurrent);
	weatherDesCurrent.replace('"', ',');
	weatherDesCurrent.remove(0, 1);
	weatherDesCurrent[0] = toupper(weatherDesCurrent[0]);

	//Serial.println(F(""));
	//Serial.print(F("Json Time Zone Offset       : "));
	//Serial.println(daylightOffset_sec);
	//Serial.print(F("Json Variable Current Epoch : "));
	//Serial.println(currentWeatherTime);
	//Serial.print(F("Json Variable Weather ID    : "));
	//Serial.println(currentWeatherNow);
	//Serial.print(F("Json Variable Temperature   : "));
	//Serial.println(tempNow);
	//Serial.print(F("Json Variable Feels Like    : "));
	//Serial.println(feelsLikeTempNow);
	//Serial.print(F("Json Variable Pressure      : "));
	//Serial.println(pressureNow);
	//Serial.print(F("Json Variable Humidity      : "));
	//Serial.println(humidityNow);
	//Serial.print(F("Json Variable Wind Speed    : "));
	//Serial.println(windSpeedNow);
	//Serial.print(F("Json Variable Wind Direction: "));
	//Serial.println(windDirectionNow);
	//Serial.print(F("Json Variable UV Index      : "));
	//Serial.println(uvIndexNow);
	//Serial.print(F("Json Variable Rain Level    : "));
	//Serial.println(rainLevelNow);
	//Serial.print(F("Json Variable Snow Level    : "));
	//Serial.println(snowLevelNow);
	//Serial.print(F("Json Variable Sunrise       : "));
	//Serial.println(sunRiseNow);
	//Serial.print(F("Json Variable Sunset        : "));
	//Serial.println(sunSetNow);
	//Serial.print(F("Json Variable Moon Phase    : "));
	//Serial.println(moonPhaseNow);
	//Serial.println(F(""));

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

	//Serial.print(F("Json Variable Hourly Rain : "));
	//Serial.println(rainFallHr1);
	//Serial.println(rainFallHr2);
	//Serial.println(rainFallHr3);
	//Serial.println(rainFallHr4);
	//Serial.println(rainFallHr5);
	//Serial.println(rainFallHr6);
	//Serial.println(rainFallHr7);
	//Serial.println(rainFallHr8);
	//Serial.println(F(""));

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
	rainPopFc1 = (myObject["daily"][1]["pop"]);
	rainLevelFc1 = (myObject["daily"][1]["rain"]);
	snowLevelFc1 = (myObject["daily"][1]["snow"]);
	sunRiseFc1 = (myObject["daily"][1]["sunrise"]);
	sunSetFc1 = (myObject["daily"][1]["sunset"]);
	moonPhaseFc1 = (myObject["daily"][1]["moon_phase"]);

	weatherDesFc1.clear();
	serializeJson((myObject["daily"][1]["weather"][0]["description"]), weatherDesFc1);
	weatherDesFc1.replace('"', ',');
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
	rainPopFc2 = (myObject["daily"][2]["pop"]);
	rainLevelFc2 = (myObject["daily"][2]["rain"]);
	snowLevelFc2 = (myObject["daily"][2]["snow"]);
	sunRiseFc2 = (myObject["daily"][2]["sunrise"]);
	sunSetFc2 = (myObject["daily"][2]["sunset"]);
	moonPhaseFc2 = (myObject["daily"][2]["moon_phase"]);

	weatherDesFc2.clear();
	serializeJson((myObject["daily"][2]["weather"][0]["description"]), weatherDesFc2);
	weatherDesFc2.replace('"', ',');
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
	rainPopFc3 = (myObject["daily"][3]["pop"]);
	rainLevelFc3 = (myObject["daily"][3]["rain"]);
	snowLevelFc3 = (myObject["daily"][3]["snow"]);
	sunRiseFc3 = (myObject["daily"][3]["sunrise"]);
	sunSetFc3 = (myObject["daily"][3]["sunset"]);
	moonPhaseFc3 = (myObject["daily"][3]["moon_phase"]);

	weatherDesFc3.clear();
	serializeJson((myObject["daily"][3]["weather"][0]["description"]), weatherDesFc3);
	weatherDesFc3.replace('"', ',');
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
	rainPopFc4 = (myObject["daily"][4]["pop"]);
	rainLevelFc4 = (myObject["daily"][4]["rain"]);
	snowLevelFc4 = (myObject["daily"][4]["snow"]);
	sunRiseFc4 = (myObject["daily"][4]["sunrise"]);
	sunSetFc4 = (myObject["daily"][4]["sunset"]);
	moonPhaseFc4 = (myObject["daily"][4]["moon_phase"]);

	weatherDesFc4.clear();
	serializeJson((myObject["daily"][4]["weather"][0]["description"]), weatherDesFc4);
	weatherDesFc4.replace('"', ',');
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
	rainPopFc5 = (myObject["daily"][5]["pop"]);
	rainLevelFc5 = (myObject["daily"][5]["rain"]);
	snowLevelFc5 = (myObject["daily"][5]["snow"]);
	sunRiseFc5 = (myObject["daily"][5]["sunrise"]);
	sunSetFc5 = (myObject["daily"][5]["sunset"]);
	moonPhaseFc5 = (myObject["daily"][5]["moon_phase"]);

	weatherDesFc5.clear();
	serializeJson((myObject["daily"][5]["weather"][0]["description"]), weatherDesFc5);
	weatherDesFc5.replace('"', ',');
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
	rainPopFc6 = (myObject["daily"][6]["pop"]);
	rainLevelFc6 = (myObject["daily"][6]["rain"]);
	snowLevelFc6 = (myObject["daily"][6]["snow"]);
	sunRiseFc6 = (myObject["daily"][6]["sunrise"]);
	sunSetFc6 = (myObject["daily"][6]["sunset"]);
	moonPhaseFc6 = (myObject["daily"][6]["moon_phase"]);

	weatherDesFc6.clear();
	serializeJson((myObject["daily"][6]["weather"][0]["description"]), weatherDesFc6);
	weatherDesFc6.replace('"', ',');
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
	rainPopFc7 = (myObject["daily"][7]["pop"]);
	rainLevelFc7 = (myObject["daily"][7]["rain"]);
	snowLevelFc7 = (myObject["daily"][7]["snow"]);
	sunRiseFc7 = (myObject["daily"][7]["sunrise"]);
	sunSetFc7 = (myObject["daily"][7]["sunset"]);
	moonPhaseFc7 = (myObject["daily"][7]["moon_phase"]);

	weatherDesFc7.clear();
	serializeJson((myObject["daily"][7]["weather"][0]["description"]), weatherDesFc7);
	weatherDesFc7.replace('"', ',');
	weatherDesFc7.remove(0, 1);
	weatherDesFc7[0] = toupper(weatherDesFc7[0]);

	//Serial.print(F("Json Variable Moon Phase    : "));
	//Serial.println(moonPhaseNow);
	//Serial.println(moonPhaseFc1);
	//Serial.println(moonPhaseFc2);
	//Serial.println(moonPhaseFc3);
	//Serial.println(moonPhaseFc4);
	//Serial.println(moonPhaseFc5);
	//Serial.println(moonPhaseFc6);
	//Serial.println(moonPhaseFc7);
	//Serial.println(F(""));

	//Serial.print(F("Json Variable Rain PoP: "));
	//Serial.println(rainPopNow);
	//Serial.println(rainPopFc1);
	//Serial.println(rainPopFc2);
	//Serial.println(rainPopFc3);
	//Serial.println(rainPopFc4);
	//Serial.println(rainPopFc5);
	//Serial.println(rainPopFc6);
	//Serial.println(rainPopFc7);
	//Serial.println(F(""));

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

void dataDisplayCurrentWeatherLayout() {

	enableScreen1();

	// Set icon to match metrix imperial switich

	if (metricImperialState == true) {

		drawBitmap(tft, TEMPERATUREICONC_Y, TEMPERATUREICONC_X, thermonmeterC, TEMPERATUREICONC_W, TEMPERATUREICONC_H);

	}

	else drawBitmap(tft, TEMPERATUREICONC_Y, TEMPERATUREICONC_X, thermonmeterF, TEMPERATUREICONC_W, TEMPERATUREICONC_H);

	// Draw bitmaps.

	drawBitmap(tft, SUNRISE_Y, SUNRISE_X, sunrise, SUNRISE_W, SUNRISE_H);

	drawBitmap(tft, SUNSET_Y, SUNSET_X, sunset, SUNSET_W, SUNSET_H);

	tft.setFont();
	tft.setTextSize(0);
	tft.setTextColor(BLACK);
	tft.setCursor(112, 215);
	tft.printf("Moon");

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

void dataDisplayCurrentWeatherDayX(double moonPhaseFc) {

	// Format humidity readings for display.

	char tempHumidity[5];
	sprintf(tempHumidity, "%2.0f", humidityNow);

	// Set day/night for main image change.

	if (currentWeatherTime <= sunRiseNow) {

		dayNight = false;
	}

	else if (currentWeatherTime >= sunSetNow) {

		dayNight = false;
	}

	else dayNight = true;

	enableScreen1();

	// Main weather image.

	currentWeatherImage(tft, currentWeatherNow, dayNight);

	disableVSPIScreens();

	// Serial.print("Day Night: ");
	// Serial.println(dayNight);

	enableScreen1();

	tft.fillRect(WINDSPEEDVALUE_X, WINDSPEEDVALUE_Y - 32, 50, 150, WHITE);		// Cover vertical values column 1
	tft.fillRect(WINDDIRECTION_X, WINDSPEEDVALUE_Y - 32, 50, 150, WHITE);		// Cover vertical values column 2
	tft.fillRect(TEMPERATUREVALUE_X, 30, 110, 40, WHITE);

	// Temperature section.

	// Formatting arrays for displays.

	char tempTemp[5];
	char tempFeelsLikeTemp[5];
	double tempTempCF;
	double tempTempFLCF;

	// Check if settings are metric or imperial.

	if (metricImperialState == false) {

		// Convert celsius to fahrenheit.

		tempTempCF = (tempNow * 9 / 5) + 32;
		tempTempFLCF = (feelsLikeTempNow * 9 / 5) + 32;

		// Format temperature data for display

		sprintf(tempTemp, "%2.0f%c%c", tempTempCF, 42, 'F'); // 42 is ASCII for degrees
		sprintf(tempFeelsLikeTemp, "%2.0f%c%c", tempTempFLCF, 42, 'F');
		drawBitmap(tft, TEMPERATUREICONC_Y, TEMPERATUREICONC_X, thermonmeterF, TEMPERATUREICONC_W, TEMPERATUREICONC_H);
	}

	else if (metricImperialState == true) {

		sprintf(tempTemp, "%2.0f%c%c", tempNow, 42, 'C'); // 42 is ASCII for degrees
		sprintf(tempFeelsLikeTemp, "%2.0f%c%c", feelsLikeTempNow, 42, 'C');
		drawBitmap(tft, TEMPERATUREICONC_Y, TEMPERATUREICONC_X, thermonmeterC, TEMPERATUREICONC_W, TEMPERATUREICONC_H);

	}

	// Display data.

	tft.setTextSize(0);
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

	// Pressure section.

	tft.setFont(&FreeSans9pt7b);
	tft.setCursor(PRESSUREVALUE_X, PRESSUREVALUE_Y);
	tft.print(pressureNow);
	tft.setFont();
	tft.setTextSize(0);
	tft.setCursor(PRESSUREVALUE_X + 4, PRESSUREVALUE_Y + 5);
	tft.print("mbar");

	// Humidity section.

	tft.setFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setCursor(HUMIDITYVALUE_X, HUMIDITYVALUE_Y);
	tft.print(tempHumidity);

	// Wind speed section.

	// Convert metres per section to miles per hour.

	double windSpeedMph;
	windSpeedMph = windSpeedNow * windMph;

	// Display wind speed.

	tft.setFont(&FreeSans9pt7b);
	tft.setCursor(WINDSPEEDVALUE_X, WINDSPEEDVALUE_Y);
	tft.print(windSpeedMph, 1);
	tft.setFont();
	tft.setTextSize(0);
	tft.setCursor(WINDSPEEDVALUE_X + 4, WINDSPEEDVALUE_Y + 5);
	tft.print("mph");

	// Wind direction section.

	heading = getHeadingReturn(windDirectionNow);
	tft.setFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setCursor(WINDDIRECTION_X, WINDDIRECTION_Y);
	tft.print(headingArray[heading]);

	// UV section.

	tft.setCursor(UVVALUE_X, UVVALUE_Y);
	tft.print(uvIndexNow);

	// Rain & snow level section.

	tft.fillRect(RAINSNOWVALUE_X, RAINSNOWVALUE_Y - 15, 45, 30, WHITE);

	if (snowLevelNow > 0) {

		tft.setFont(&FreeSans9pt7b);
		tft.setTextSize(1);
		tft.setCursor(RAINSNOWVALUE_X, RAINSNOWVALUE_Y);
		tft.print(snowLevelNow, 1);
		tft.setFont();
		tft.setTextSize(0);
		tft.setCursor(RAINSNOWVALUE_X + 4, RAINSNOWVALUE_Y + 5);
		tft.setTextColor(BLACK);
		tft.print("mm Snow");

	}

	else {

		tft.setFont(&FreeSans9pt7b);
		tft.setTextSize(1);
		tft.setCursor(RAINSNOWVALUE_X, RAINSNOWVALUE_Y);
		tft.print(rainLevelNow, 1);
		tft.setFont();
		tft.setTextSize(0);
		tft.setCursor(RAINSNOWVALUE_X + 4, RAINSNOWVALUE_Y + 5);
		tft.setTextColor(BLACK);
		tft.print("mm");

	}

	// Extract sunrise time.

	time_t sunRise = sunRiseNow;
	struct tm  ts;
	char       buf[80];

	// Format time example, "ddd yyyy-mm-dd hh:mm:ss zzz" - strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);

	ts = *localtime(&sunRise);
	strftime(buf, sizeof(buf), "%H:%M", &ts);

	// Serial.println("");
	// Serial.print("Check conversion of sunrise time: ");
	// Serial.printf("%s\n", buf);
	// Serial.println("");

	tft.setFont();
	tft.setTextSize(0);
	tft.setTextColor(BLACK, WHITE);
	tft.setCursor(11, 215);
	tft.printf("%s\n", buf);

	// Extract sunset time.

	time_t sunSet = sunSetNow;

	ts = *localtime(&sunSet);
	strftime(buf, sizeof(buf), "%H:%M", &ts);

	// Serial.print("Check conversion of sunset time : ");
	// Serial.printf("%s\n", buf);
	// Serial.println("");

	tft.setFont();
	tft.setTextSize(0);
	tft.setTextColor(BLACK, WHITE);
	tft.setCursor(61, 215);
	tft.printf("%s\n", buf);

	tft.setFont();

	// Moon phase section.

	drawDailyMoonPhase(moonPhaseFc);

	disableVSPIScreens();

} // Close function.

/*-----------------------------------------------------------------*/

void dataDisplayCurrentWeatherAlternateData() {

	enableScreen1();

	if (flagTempDisplayChange == false) {

		tft.fillRect(0, 0, 320, 25, WHITE);
		tft.setFont(&FreeSans9pt7b);
		tft.setTextSize(1);
		tft.setTextColor(BLACK, WHITE);
		tft.setCursor(5, 20);
		tft.println("Current Weather");

		// Rain & snow level section.

		tft.fillRect(RAINSNOWVALUE_X, RAINSNOWVALUE_Y - 15, 45, 30, WHITE);

		if (snowLevelNow > 0) {

			drawBitmap(tft, RAINSNOWICON_Y, RAINSNOWICON_X, snowflake, RAINSNOWICON_W, RAINSNOWICON_H);

			tft.setFont(&FreeSans9pt7b);
			tft.setTextSize(1);
			tft.setCursor(RAINSNOWVALUE_X, RAINSNOWVALUE_Y);
			tft.print(snowLevelNow, 1);
			tft.setFont();
			tft.setTextSize(0);
			tft.setCursor(RAINSNOWVALUE_X + 4, RAINSNOWVALUE_Y + 5);
			tft.setTextColor(BLACK);
			tft.print("mm Snow");

		}

		else {

			drawBitmap(tft, RAINSNOWICON_Y, RAINSNOWICON_X, rainSnowFall, RAINSNOWICON_W, RAINSNOWICON_H);

			tft.setFont(&FreeSans9pt7b);
			tft.setTextSize(1);
			tft.setCursor(RAINSNOWVALUE_X, RAINSNOWVALUE_Y);
			tft.print(rainLevelNow, 1);
			tft.setFont();
			tft.setTextSize(0);
			tft.setCursor(RAINSNOWVALUE_X + 4, RAINSNOWVALUE_Y + 5);
			tft.setTextColor(BLACK);
			tft.print("mm");

		}

	}

	else if (flagTempDisplayChange == true) {

		enableScreen1();

		tft.fillRect(0, 0, 320, 25, WHITE);

		tft.setFont(&FreeSans9pt7b);
		tft.setTextSize(1);
		tft.setTextColor(BLACK, WHITE);
		tft.setCursor(5, 20);
		tft.print(weatherDesCurrent);

		// Convert metres per section to miles per hour.

		double windSpeedMph;
		windSpeedMph = windSpeedNow * windMph;

		// Set text for wind speed description.

		if (windSpeedMph <= 1.16) {

			tft.print(windSpeedDescription[0]);
		}

		else if (windSpeedMph <= 3.45) {

			tft.print(windSpeedDescription[1]);
		}

		else if (windSpeedMph <= 6.90) {

			tft.print(windSpeedDescription[2]);
		}

		else if (windSpeedMph <= 11.50) {

			tft.print(windSpeedDescription[3]);
		}

		else if (windSpeedMph <= 18.41) {

			tft.print(windSpeedDescription[4]);
		}

		else if (windSpeedMph <= 24.16) {

			tft.print(windSpeedDescription[5]);
		}

		else if (windSpeedMph <= 31.07) {

			tft.print(windSpeedDescription[6]);
		}

		else if (windSpeedMph <= 37.97) {

			tft.print(windSpeedDescription[7]);
		}

		else if (windSpeedMph <= 46.03) {

			tft.print(windSpeedDescription[8]);
		}

		else if (windSpeedMph <= 54.08) {

			tft.print(windSpeedDescription[9]);
		}

		else if (windSpeedMph <= 63.29) {

			tft.print(windSpeedDescription[10]);
		}

		else if (windSpeedMph <= 72.49) {

			tft.print(windSpeedDescription[11]);
		}

		else {

			tft.print(windSpeedDescription[12]);
		}

		// Rain level alternates with rain probability.

		byte rainPopTemp;

		rainPopTemp = rainPopNow * 100;

		drawBitmap(tft, RAINSNOWICON_Y, RAINSNOWICON_X, precipitation, RAINSNOWICON_W, RAINSNOWICON_H);

		tft.fillRect(RAINSNOWVALUE_X, RAINSNOWVALUE_Y - 15, 45, 30, WHITE);

		tft.setFont(&FreeSans9pt7b);
		tft.setTextSize(1);
		tft.setCursor(RAINSNOWVALUE_X, RAINSNOWVALUE_Y);
		tft.print(rainPopTemp);
		tft.setFont();
		tft.setTextSize(0);
		tft.setCursor(RAINSNOWVALUE_X + 4, RAINSNOWVALUE_Y + 5);
		tft.setTextColor(BLACK);
		tft.print("%");
	}

	disableVSPIScreens();

} // Close function.

/*-----------------------------------------------------------------*/

// Display forecast weather layout day X.

void dataDisplayForecastWeatherLayoutDayX(Adafruit_ILI9341& tft, byte screen) {

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

	// Set icon to match metrix imperial switich

	if (metricImperialState == true) {

		drawBitmap(tft, TEMPERATUREICONC_Y, TEMPERATUREICONC_X, thermonmeterC, TEMPERATUREICONC_W, TEMPERATUREICONC_H);

	}

	else drawBitmap(tft, TEMPERATUREICONC_Y, TEMPERATUREICONC_X, thermonmeterF, TEMPERATUREICONC_W, TEMPERATUREICONC_H);

	// Draw bitmaps.

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

void dataDisplayForecastDayX(Adafruit_ILI9341& tft, byte screen, unsigned long forecastTimeFc, unsigned int pressureFc, double humidityFc, double windSpeedFc, unsigned int windDirectionFc, byte uvIndexFc, unsigned int weatherLabelFc, double rainLevelFc, double snowLevelFc, unsigned long sunRiseFc, unsigned long sunSetFc, double moonPhaseFc) {

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

	// Format humidity readings for display.

	char tempHumidity[5];
	sprintf(tempHumidity, "%2.0f", humidityFc);

	// Main weather image.

	forecastWeatherImage(tft, weatherLabelFc);

	tft.fillRect(WINDSPEEDVALUE_X, WINDSPEEDVALUE_Y - 32, 50, 150, WHITE);		// Cover vertical values column 1
	tft.fillRect(WINDDIRECTION_X, WINDSPEEDVALUE_Y - 32, 50, 150, WHITE);		// Cover vertical values column 2

	// Pressure section.

	tft.setFont(&FreeSans9pt7b);
	tft.setCursor(PRESSUREVALUE_X, PRESSUREVALUE_Y);
	tft.print(pressureFc);
	tft.setFont();
	tft.setTextSize(0);
	tft.setCursor(PRESSUREVALUE_X + 4, PRESSUREVALUE_Y + 5);
	tft.print("mbar");

	// Humidity section.

	tft.setFont(&FreeSans9pt7b);
	tft.setCursor(HUMIDITYVALUE_X, HUMIDITYVALUE_Y);
	tft.print(tempHumidity);

	// Wind speed section.

	// Convert metres per section to miles per hour.

	double windSpeedMph;
	windSpeedMph = windSpeedFc * windMph;

	tft.setCursor(WINDSPEEDVALUE_X, WINDSPEEDVALUE_Y);
	tft.print(windSpeedMph, 1);
	tft.setFont();
	tft.setTextSize(0);
	tft.setCursor(WINDSPEEDVALUE_X + 4, WINDSPEEDVALUE_Y + 5);
	tft.print("mph");

	// Wind direction section.

	heading = getHeadingReturn(windDirectionFc);
	tft.setFont(&FreeSans9pt7b);
	tft.setCursor(WINDDIRECTION_X, WINDDIRECTION_Y);
	tft.print(headingArray[heading]);

	// UV section.

	tft.setCursor(UVVALUE_X, UVVALUE_Y);
	tft.setFont(&FreeSans9pt7b);
	tft.print(uvIndexFc);

	// Rain & snow level section.

	tft.fillRect(RAINSNOWVALUE_X, RAINSNOWVALUE_Y - 15, 45, 30, WHITE);

	if (snowLevelFc > 0) {

		tft.setFont(&FreeSans9pt7b);
		tft.setTextSize(1);
		tft.setCursor(RAINSNOWVALUE_X, RAINSNOWVALUE_Y);
		tft.print(snowLevelFc, 1);
		tft.setFont();
		tft.setTextSize(0);
		tft.setCursor(RAINSNOWVALUE_X + 4, RAINSNOWVALUE_Y + 5);
		tft.setTextColor(BLACK);
		tft.print("mm Snow");

	}

	else {

		tft.setFont(&FreeSans9pt7b);
		tft.setTextSize(1);
		tft.setCursor(RAINSNOWVALUE_X, RAINSNOWVALUE_Y);
		tft.print(rainLevelFc, 1);
		tft.setFont();
		tft.setTextSize(0);
		tft.setCursor(RAINSNOWVALUE_X + 4, RAINSNOWVALUE_Y + 5);
		tft.setTextColor(BLACK);
		tft.print("mm");

	}

	// Extract sunrise time.

	time_t sunRise = sunRiseFc;
	struct	tm  ts;
	char	buf[80];

	// Format time example, "ddd yyyy-mm-dd hh:mm:ss zzz" - strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);

	ts = *localtime(&sunRise);
	strftime(buf, sizeof(buf), "%H:%M", &ts);

	// Serial.println("");
	// Serial.print("Check conversion of sunrise time: ");
	// Serial.printf(buf);
	// Serial.println("");

	tft.setFont();
	tft.setTextSize(0);
	tft.setTextColor(BLACK, WHITE);
	tft.setCursor(11, 215);
	tft.printf("%s\n", buf);

	// Extract sunset time.

	time_t sunSet = sunSetFc;

	ts = *localtime(&sunSet);
	strftime(buf, sizeof(buf), "%H:%M", &ts);

	// Serial.print("Check conversion of sunset time : ");
	// Serial.printf(buf);
	// Serial.println("");

	tft.setFont();
	tft.setTextSize(0);
	tft.setTextColor(BLACK, WHITE);
	tft.setCursor(61, 215);
	tft.printf("%s\n", buf);

	tft.setFont();

	// Moon phase section.

	drawDailyMoonPhase(moonPhaseFc);

	disableVSPIScreens();

} // Close function.

/*-----------------------------------------------------------------*/

// Display forecast day / night / min / max temperature day X.

void dataDisplayForecastAlternateData(Adafruit_ILI9341& tft, byte screen, unsigned long forecastTimeFc, String weatherDesFc, double tempDayFc, double tempNightFc, double tempMinFc, double tempMaxFc, double rainLevelFc, double rainPopFc, double windSpeedFc, double snowLevelFc) {

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

	// Set day and date and format.

	struct	tm  ts;
	char	bufDay[30];

	time_t forecastDate = forecastTimeFc;

	ts = *localtime(&forecastDate);
	strftime(bufDay, sizeof(bufDay), "%A, %B %d", &ts);

	// Check if metric or imperial to set temperature icons.

	if (metricImperialState == true) {

		drawBitmap(tft, TEMPERATUREICONC_Y, TEMPERATUREICONC_X, thermonmeterC, TEMPERATUREICONC_W, TEMPERATUREICONC_H);

	}

	else drawBitmap(tft, TEMPERATUREICONC_Y, TEMPERATUREICONC_X, thermonmeterF, TEMPERATUREICONC_W, TEMPERATUREICONC_H);

	// Formatting arrays for displays.

	char tempTemp[5];
	char tempTempNight[5];
	char tempTempMin[5];
	char tempTempMax[5];

	double tempTempCF;
	double tempTempCNF;
	double tempTempCMinF;
	double tempTempCMaxF;

	// Check if settings are metric or imperial.

	if (metricImperialState == false) {

		// Convert celsius to fahrenheit.

		tempTempCF = (tempDayFc * 9 / 5) + 32;
		tempTempCNF = (tempNightFc * 9 / 5) + 32;
		tempTempCMinF = (tempMinFc * 9 / 5) + 32;
		tempTempCMaxF = (tempMaxFc * 9 / 5) + 32;

		// Format temperature data for display


		sprintf(tempTemp, "%2.0f%c%c", tempTempCF, 42, 'F'); // 42 is ASCII for degrees
		sprintf(tempTempNight, "%2.0f%c%c", tempTempCNF, 42, 'F');
		sprintf(tempTempMin, "%2.0f%c%c", tempTempCMinF, 42, 'F'); // 42 is ASCII for degrees
		sprintf(tempTempMax, "%2.0f%c%c", tempTempCMaxF, 42, 'F');

	}

	else if (metricImperialState == true) {


		sprintf(tempTemp, "%2.0f%c%c", tempDayFc, 42, 'C'); // 42 is ASCII for degrees
		sprintf(tempTempNight, "%2.0f%c%c", tempNightFc, 42, 'C');
		sprintf(tempTempMin, "%2.0f%c%c", tempMinFc, 42, 'C'); // 42 is ASCII for degrees
		sprintf(tempTempMax, "%2.0f%c%c", tempMaxFc, 42, 'C');

	}

	// Display data, alternate between temperatures.

	if (flagTempDisplayChange == false) {

		tft.fillRect(TEMPERATUREVALUE_X, 30, 110, 40, WHITE);
		tft.fillRect(0, 0, 320, 25, WHITE);

		tft.setTextSize(0);
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

		// Rain & snow level section.

		tft.fillRect(RAINSNOWVALUE_X, RAINSNOWVALUE_Y - 15, 45, 30, WHITE);

		if (snowLevelFc > 0) {

			drawBitmap(tft, RAINSNOWICON_Y, RAINSNOWICON_X, snowflake, RAINSNOWICON_W, RAINSNOWICON_H);

			tft.setFont(&FreeSans9pt7b);
			tft.setTextSize(1);
			tft.setCursor(RAINSNOWVALUE_X, RAINSNOWVALUE_Y);
			tft.print(snowLevelFc, 1);
			tft.setFont();
			tft.setTextSize(0);
			tft.setCursor(RAINSNOWVALUE_X + 4, RAINSNOWVALUE_Y + 5);
			tft.setTextColor(BLACK);
			tft.print("mm Snow");

		}

		else {

			drawBitmap(tft, RAINSNOWICON_Y, RAINSNOWICON_X, rainSnowFall, RAINSNOWICON_W, RAINSNOWICON_H);

			tft.setFont(&FreeSans9pt7b);
			tft.setTextSize(1);
			tft.setCursor(RAINSNOWVALUE_X, RAINSNOWVALUE_Y);
			tft.print(rainLevelFc, 1);
			tft.setFont();
			tft.setTextSize(0);
			tft.setCursor(RAINSNOWVALUE_X + 4, RAINSNOWVALUE_Y + 5);
			tft.setTextColor(BLACK);
			tft.print("mm");

		}

	}

	// Now alternate.

	else if (flagTempDisplayChange == true) {

		tft.fillRect(TEMPERATUREVALUE_X, 30, 110, 40, WHITE);
		tft.fillRect(0, 0, 320, 25, WHITE);

		tft.setTextSize(0);
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

		// Convert metres per section to miles per hour.

		double windSpeedMph;
		windSpeedMph = windSpeedFc * windMph;

		// Set text for wind speed description.

		if (windSpeedMph <= 1.16) {

			tft.println(windSpeedDescription[0]);
		}

		else if (windSpeedMph <= 3.45) {

			tft.println(windSpeedDescription[1]);
		}

		else if (windSpeedMph <= 6.90) {

			tft.println(windSpeedDescription[2]);
		}

		else if (windSpeedMph <= 11.50) {

			tft.println(windSpeedDescription[3]);
		}

		else if (windSpeedMph <= 18.41) {

			tft.println(windSpeedDescription[4]);
		}

		else if (windSpeedMph <= 24.16) {

			tft.println(windSpeedDescription[5]);
		}

		else if (windSpeedMph <= 31.07) {

			tft.println(windSpeedDescription[6]);
		}

		else if (windSpeedMph <= 37.97) {

			tft.println(windSpeedDescription[7]);
		}

		else if (windSpeedMph <= 46.03) {

			tft.println(windSpeedDescription[8]);
		}

		else if (windSpeedMph <= 54.08) {

			tft.println(windSpeedDescription[9]);
		}

		else if (windSpeedMph <= 63.29) {

			tft.println(windSpeedDescription[10]);
		}

		else if (windSpeedMph <= 72.49) {

			tft.println(windSpeedDescription[11]);
		}

		else {

			tft.println(windSpeedDescription[12]);
		}

		// Rain level alternates with rain probability.

		byte rainPopTemp;

		rainPopTemp = rainPopFc * 100;

		drawBitmap(tft, RAINSNOWICON_Y, RAINSNOWICON_X, precipitation, RAINSNOWICON_W, RAINSNOWICON_H);

		tft.fillRect(RAINSNOWVALUE_X, RAINSNOWVALUE_Y - 15, 45, 30, WHITE);

		tft.setFont(&FreeSans9pt7b);
		tft.setTextSize(1);
		tft.setCursor(RAINSNOWVALUE_X, RAINSNOWVALUE_Y);
		tft.print(rainPopTemp);
		tft.setFont();
		tft.setTextSize(0);
		tft.setCursor(RAINSNOWVALUE_X + 4, RAINSNOWVALUE_Y + 5);
		tft.setTextColor(BLACK);
		tft.print("%");

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

	tft.setFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setTextColor(BLACK);
	tft.println("");
	tft.setCursor(20, 175);
	tft.print("Unit is starting...");
	tft.setFont();

	disableVSPIScreens();

} // Close function.

/*-----------------------------------------------------------------*/

// WiFi title page.

void wiFiApMode() {

	// WiFi AP mode title screen.

	enableScreen1();

	tft.fillScreen(WHITE);

	tft.setFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setTextColor(BLACK);
	tft.setCursor(13, 26);
	tft.println("Switching Wi-Fi Mode");
	tft.setFont();

	delay(2000);

	tft.fillScreen(WHITE);

	tft.setFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setTextColor(BLACK);
	tft.setCursor(13, 26);
	tft.println("Access Point Mode");
	tft.setFont();

	tft.fillRect(39, 60, 240, 139, LTRED);
	tft.drawRect(38, 59, 242, 141, BLACK);
	tft.setFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setTextColor(WHITE);
	tft.setCursor(50, 78);
	tft.print("Follow these instructions:");

	tft.setFont();
	tft.setTextColor(WHITE);
	tft.setCursor(50, 90);
	tft.print("We could not connect to last Wi-Fi");
	tft.setCursor(50, 106);
	tft.print("1) Using your mobile phone Wi-Fi");
	tft.setCursor(50, 118);
	tft.print("2) Connect to WiFI-Manager network");
	tft.setCursor(50, 130);
	tft.print("3) Browse to web address 192.168.4.1");
	tft.setCursor(50, 142);
	tft.print("4) Enter network settings");
	tft.setCursor(50, 154);
	tft.print("5) Unit will auto restart when saved");
	tft.setCursor(50, 166);
	tft.print("");
	tft.setCursor(50, 185);
	tft.print("Call Christopher Cooper for Support");

	drawBitmap(tft, WIFI_ICON_Y, WIFI_ICON_X, wiFiAp, WIFI_ICON_W, WIFI_ICON_H);

	disableVSPIScreens();

} // Close function.

/*-----------------------------------------------------------------*/

// Draw hourly temperature chart.

void drawHourlyTempChart(Adafruit_ILI9341& tft) {

	// Title.

	tft.setFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setTextColor(BLACK, WHITE);
	tft.setCursor(5, 20);
	tft.print("Hourly Temperature");
	tft.setFont();

	double tempTemp;

	if (metricImperialState == true) {

		drawBitmap(tft, 40, 228, temperatureCLrgBlk, 64, 64);
		tempTemp = temperatureHr1;

		if (tempTemp < 10.00 && tempTemp > -10.00) {

			tft.setFont(&FreeSans12pt7b);
			tft.setTextColor(BLACK, WHITE);
			tft.setCursor(233, 135);
			tft.println(tempTemp);
			tft.setFont();
		}

		if (tempTemp >= 10.00) {

			tft.setFont(&FreeSans12pt7b);
			tft.setTextColor(BLACK, WHITE);
			tft.setCursor(230, 135);
			tft.println(tempTemp);
			tft.setFont();
		}

	}

	else if (metricImperialState == false) {

		drawBitmap(tft, 40, 228, temperatureFLrgBlk, 64, 64);
		tempTemp = (temperatureHr1 * 9 / 5) + 32;

		tft.setFont(&FreeSans12pt7b);
		tft.setTextColor(BLACK, WHITE);
		tft.setCursor(228, 135);
		tft.println(tempTemp);
		tft.setFont();
	}

	tft.setTextSize(1);
	tft.setTextColor(BLACK, WHITE);
	tft.setCursor(60, 228);
	tft.println("Current + hr");
	tft.setFont();

	tft.setFont();
	tft.setTextSize(1);
	tft.setTextColor(BLACK, WHITE);
	tft.setCursor(235, 145);
	tft.println("Degrees");
	tft.setFont();

	int loVal;
	int hiVal;

	double tempTemp1;
	double tempTemp2;
	double tempTemp3;
	double tempTemp4;
	double tempTemp5;
	double tempTemp6;
	double tempTemp7;
	double tempTemp8;

	// Adjust graph scales for celsius.

	if (metricImperialState == true) {

		tempTemp1 = temperatureHr1;
		tempTemp2 = temperatureHr2;
		tempTemp3 = temperatureHr3;
		tempTemp4 = temperatureHr4;
		tempTemp5 = temperatureHr5;
		tempTemp6 = temperatureHr6;
		tempTemp7 = temperatureHr7;
		tempTemp8 = temperatureHr8;

		if (tempTemp1 > 18) {

			loVal = 10;
			hiVal = 50;
		}

		else if (tempTemp1 > -10 && tempTemp1 <= 18) {

			loVal = -10;
			hiVal = 30;
		}

		else if (tempTemp1 < -10) {

			loVal = -30;
			hiVal = 10;
		}

	}

	// Adjust graph scales for fahrenheit.

	else {

		tempTemp1 = (temperatureHr1 * 9 / 5) + 32;
		tempTemp2 = (temperatureHr2 * 9 / 5) + 32;
		tempTemp3 = (temperatureHr3 * 9 / 5) + 32;
		tempTemp4 = (temperatureHr4 * 9 / 5) + 32;
		tempTemp5 = (temperatureHr5 * 9 / 5) + 32;
		tempTemp6 = (temperatureHr6 * 9 / 5) + 32;
		tempTemp7 = (temperatureHr7 * 9 / 5) + 32;
		tempTemp8 = (temperatureHr8 * 9 / 5) + 32;

		if (tempTemp1 > -11.2 && tempTemp1 < 32.1) {

			loVal = -15;
			hiVal = 45;
		}

		else if (tempTemp1 > 32.00 && tempTemp1 < 77.00) {

			loVal = 25;
			hiVal = 85;
		}

		else if (tempTemp1 > 77.00 && tempTemp1 < 122.00) {

			loVal = 65;
			hiVal = 120;
		}

	}

	drawBarChartV1(tft, 1, 20, 210, 10, 170, loVal, hiVal, 5, tempTemp1, 2, 0, BLUE, WHITE, BLACK, BLACK, WHITE, "C", 0, graph_1);
	drawBarChartV1(tft, 1, 40, 210, 10, 170, loVal, hiVal, 5, tempTemp2, 2, 0, BLUE, WHITE, BLACK, BLACK, WHITE, "+1", 0, graph_2);
	drawBarChartV1(tft, 1, 60, 210, 10, 170, loVal, hiVal, 5, tempTemp3, 2, 0, BLUE, WHITE, BLACK, BLACK, WHITE, "+2", 0, graph_3);
	drawBarChartV1(tft, 1, 80, 210, 10, 170, loVal, hiVal, 5, tempTemp4, 2, 0, BLUE, WHITE, BLACK, BLACK, WHITE, "+3", 0, graph_4);
	drawBarChartV1(tft, 1, 100, 210, 10, 170, loVal, hiVal, 5, tempTemp5, 2, 0, BLUE, WHITE, BLACK, BLACK, WHITE, "+4", 0, graph_5);
	drawBarChartV1(tft, 1, 120, 210, 10, 170, loVal, hiVal, 5, tempTemp6, 2, 0, BLUE, WHITE, BLACK, BLACK, WHITE, "+5", 0, graph_6);
	drawBarChartV1(tft, 1, 140, 210, 10, 170, loVal, hiVal, 5, tempTemp7, 2, 0, BLUE, WHITE, BLACK, BLACK, WHITE, "+6", 0, graph_7);
	drawBarChartV2(tft, 1, 160, 210, 10, 170, loVal, hiVal, 5, tempTemp8, 2, 0, BLUE, WHITE, BLACK, BLACK, WHITE, "+7", 0, graph_8);

	disableVSPIScreens();

} // Close function.

/*-----------------------------------------------------------------*/

// Draw hourly rain fall chart.

void drawHourlyRainChart(Adafruit_ILI9341& tft) {

	// Title.

	tft.setFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setTextColor(BLACK, WHITE);
	tft.setCursor(5, 20);
	tft.print("Hourly Rain Fall");
	tft.setFont();

	drawBitmap(tft, 40, 228, rainFallLrgBlk, 64, 64);

	tft.setTextSize(1);
	tft.setTextColor(BLACK, WHITE);
	tft.setCursor(202, 206);
	tft.println("mm");
	tft.setFont();

	tft.setTextSize(1);
	tft.setTextColor(BLACK, WHITE);
	tft.setCursor(60, 228);
	tft.println("Current + hr");
	tft.setFont();

	if (rainFallHr1 > 100) {

		rainFallHr1 = 100;
	}

	tft.setFont(&FreeSans12pt7b);
	tft.setTextColor(BLACK, WHITE);
	tft.setCursor(240, 135);
	tft.println(rainFallHr1);
	tft.setFont();

	tft.setTextSize(1);
	tft.setTextColor(BLACK, WHITE);
	tft.setCursor(255, 145);
	tft.println("mm");
	tft.setFont();

	tft.setTextSize(0);

	// Set hourly chart scale.

	if (rainFallHr1 > rainChartScale) {

		rainChartScale = rainFallHr1;
	}

	if (rainFallHr2 > rainChartScale) {

		rainChartScale = rainFallHr2;
	}


	if (rainFallHr3 > rainChartScale) {

		rainChartScale = rainFallHr3;
	}


	if (rainFallHr4 > rainChartScale) {

		rainChartScale = rainFallHr4;
	}


	if (rainFallHr5 > rainChartScale) {

		rainChartScale = rainFallHr5;
	}


	if (rainFallHr6 > rainChartScale) {

		rainChartScale = rainFallHr6;
	}


	if (rainFallHr7 > rainChartScale) {

		rainChartScale = rainFallHr7;
	}


	if (rainFallHr8 > rainChartScale) {

		rainChartScale = rainFallHr8;
	}

	int hiVal;
	double increments;

	if (rainChartScale <= 10) {

		hiVal = 10;
		increments = 1;
	}

	else if (rainChartScale <= 25) {

		hiVal = 25;
		increments = 5;
	}

	else if (rainChartScale <= 50) {

		hiVal = 50;
		increments = 5;
	}

	else if (rainChartScale <= 100) {

		hiVal = 100;
		increments = 10;
	}

	// Insert scale change including intervals.

	drawBarChartV1(tft, 1, 20, 210, 10, 170, 0, hiVal, increments, rainFallHr1, 1, 0, BLUE, WHITE, BLACK, BLACK, WHITE, "C", 1, graph_1);
	drawBarChartV1(tft, 1, 40, 210, 10, 170, 0, hiVal, increments, rainFallHr2, 1, 0, BLUE, WHITE, BLACK, BLACK, WHITE, "+1", 1, graph_2);
	drawBarChartV1(tft, 1, 60, 210, 10, 170, 0, hiVal, increments, rainFallHr3, 1, 0, BLUE, WHITE, BLACK, BLACK, WHITE, "+2", 1, graph_3);
	drawBarChartV1(tft, 1, 80, 210, 10, 170, 0, hiVal, increments, rainFallHr4, 1, 0, BLUE, WHITE, BLACK, BLACK, WHITE, "+3", 1, graph_4);
	drawBarChartV1(tft, 1, 100, 210, 10, 170, 0, hiVal, increments, rainFallHr5, 1, 0, BLUE, WHITE, BLACK, BLACK, WHITE, "+4", 1, graph_5);
	drawBarChartV1(tft, 1, 120, 210, 10, 170, 0, hiVal, increments, rainFallHr6, 1, 0, BLUE, WHITE, BLACK, BLACK, WHITE, "+5", 1, graph_6);
	drawBarChartV1(tft, 1, 140, 210, 10, 170, 0, hiVal, increments, rainFallHr7, 1, 0, BLUE, WHITE, BLACK, BLACK, WHITE, "+6", 1, graph_7);
	drawBarChartV2(tft, 1, 160, 210, 10, 170, 0, hiVal, increments, rainFallHr8, 1, 0, BLUE, WHITE, BLACK, BLACK, WHITE, "+7", 1, graph_8);

	disableVSPIScreens();

} // Close function.

/*-----------------------------------------------------------------*/

// Draw hourly pressure chart.

void drawHourlyPressureChart(Adafruit_ILI9341& tft) {

	// Title.

	tft.setFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setTextColor(BLACK, WHITE);
	tft.setCursor(5, 20);
	tft.print("Hourly Pressure - ");
	tft.setFont();

	drawBitmap(tft, 40, 228, pressureLrgBlk, 64, 64);

	tft.setTextSize(1);
	tft.setTextColor(BLACK, WHITE);
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

	// Serial.print("Pressure Differece = ");
	// Serial.println(tempPressure);
	// Serial.println("");

	if (tempPressure <= 0) {

		// Serial.println("");
		// Serial.print("Pressure is falling function");

		if (tempPressure == 0 || tempPressure == -1) {

			tft.setFont(&FreeSans9pt7b);
			tft.setTextColor(BLACK, WHITE);
			tft.setCursor(150, 20);
			tft.println("Steady");
			tft.setFont();
		}

		else if (tempPressure == -2 || tempPressure == -3 || tempPressure == -4 || tempPressure == -5) {

			tft.setFont(&FreeSans9pt7b);
			tft.setTextColor(BLACK, WHITE);
			tft.setCursor(150, 20);
			tft.println("Falling");
			tft.setFont();
		}

		else if (tempPressure >= -6) {

			tft.setFont(&FreeSans9pt7b);
			tft.setTextColor(BLACK, WHITE);
			tft.setCursor(150, 20);
			tft.println("Falling rapidly");
			tft.setFont();
		}
	}

	else if (tempPressure > 0) {

		// Serial.println("");
		// Serial.print("Pressure is rising function");

		if (tempPressure == 1) {

			tft.setFont(&FreeSans9pt7b);
			tft.setTextColor(BLACK, WHITE);
			tft.setCursor(150, 20);
			tft.println("Steady");
			tft.setFont();
		}

		else if (tempPressure == 2 || tempPressure == 3 || tempPressure == 4 || tempPressure == 5) {

			tft.setFont(&FreeSans9pt7b);
			tft.setTextColor(BLACK, WHITE);
			tft.setCursor(150, 20);
			tft.println("Rising");
			tft.setFont();
		}

		else if (tempPressure >= 6) {

			tft.setFont(&FreeSans9pt7b);
			tft.setTextColor(BLACK, WHITE);
			tft.setCursor(150, 20);
			tft.println("Rising rapidly");
			tft.setFont();
		}
	}

	tft.setTextSize(0);

	int loVal = 900;
	int hiVal = 1100;


	if (pressureHr1 > 1020) {

		loVal = 980;
		hiVal = 1060;
	}

	else if (pressureHr1 > 960 && pressureHr1 < 1020) {

		loVal = 950;
		hiVal = 1050;
	}

	else if (pressureHr1 < 961) {

		loVal = 900;
		hiVal = 1000;
	}

	drawBarChartV1(tft, 4, 20, 210, 10, 170, loVal, hiVal, 20, pressureHr1, 2, 0, BLUE, WHITE, BLACK, BLACK, WHITE, "C", 0, graph_1);
	drawBarChartV1(tft, 4, 40, 210, 10, 170, loVal, hiVal, 20, pressureHr2, 2, 0, BLUE, WHITE, BLACK, BLACK, WHITE, "+1", 0, graph_2);
	drawBarChartV1(tft, 4, 60, 210, 10, 170, loVal, hiVal, 20, pressureHr3, 2, 0, BLUE, WHITE, BLACK, BLACK, WHITE, "+2", 0, graph_3);
	drawBarChartV1(tft, 4, 80, 210, 10, 170, loVal, hiVal, 20, pressureHr4, 2, 0, BLUE, WHITE, BLACK, BLACK, WHITE, "+3", 0, graph_4);
	drawBarChartV1(tft, 4, 100, 210, 10, 170, loVal, hiVal, 20, pressureHr5, 2, 0, BLUE, WHITE, BLACK, BLACK, WHITE, "+4", 0, graph_5);
	drawBarChartV1(tft, 4, 120, 210, 10, 170, loVal, hiVal, 20, pressureHr6, 2, 0, BLUE, WHITE, BLACK, BLACK, WHITE, "+5", 0, graph_6);
	drawBarChartV1(tft, 4, 140, 210, 10, 170, loVal, hiVal, 20, pressureHr7, 2, 0, BLUE, WHITE, BLACK, BLACK, WHITE, "+6", 0, graph_7);
	drawBarChartV2(tft, 4, 160, 210, 10, 170, loVal, hiVal, 20, pressureHr8, 2, 0, BLUE, WHITE, BLACK, BLACK, WHITE, "+7", 0, graph_8);

	disableVSPIScreens();

} // Close function.

/*-----------------------------------------------------------------*/

// Draw main compass screen.

void drawCompassChart(Adafruit_ILI9341& tft) {

	// Blank screen then title.

	tft.fillScreen(WHITE);

	tft.setFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setTextColor(BLACK, WHITE);
	tft.setCursor(5, 20);
	tft.print("Wind Direction");
	tft.setFont();

	// Draw weathervain.

	drawBitmap(tft, 40, 228, weatherVainLrgBlk, 64, 64);

	// Display wind direction degrees, aligning text.

	if (windDirectionNow > 99) {

		tft.setFont(&FreeSans12pt7b);
		tft.setTextColor(BLACK, WHITE);
		tft.setCursor(245, 135);
		tft.println(windDirectionNow);
		tft.setFont();
	}

	else if (windDirectionNow > 9) {

		tft.setFont(&FreeSans12pt7b);
		tft.setTextColor(BLACK, WHITE);
		tft.setCursor(251, 135);
		tft.println(windDirectionNow);
		tft.setFont();
	}

	else {

		tft.setFont(&FreeSans12pt7b);
		tft.setTextColor(BLACK, WHITE);
		tft.setCursor(256, 135);
		tft.println(windDirectionNow);
		tft.setFont();
	}

	tft.setTextSize(1);
	tft.setTextColor(BLACK, WHITE);
	tft.setCursor(245, 145);
	tft.println("Degrees");
	tft.setFont();

	// Draw compass & needle.

	drawCompass(tft);
	drawNeedle(tft, windDirectionNow);

	// Clear previous heading text.

	tft.fillRect(242, 162, 47, 30, WHITE);

	// Align and display heading text.

	// Array { "N", "NE", "E", "SE", "S", "SW", "W", "NW" }

	heading = getHeadingReturn(windDirectionNow);

	if (heading == 1 || heading == 3 || heading == 5 || heading == 7) {

		tft.setFont(&FreeSans12pt7b);
		tft.setTextSize(1);
		tft.setCursor(248, 185);
		tft.print(headingArray[heading]);

	}

	else if (heading == 6) {

		tft.setFont(&FreeSans12pt7b);
		tft.setTextSize(1);
		tft.setCursor(253, 185);
		tft.print(headingArray[heading]);

	}

	else if (heading == 0) {

		tft.setFont(&FreeSans12pt7b);
		tft.setTextSize(1);
		tft.setCursor(255, 185);
		tft.print(headingArray[heading]);

	}

	else {

		tft.setFont(&FreeSans12pt7b);
		tft.setTextSize(1);
		tft.setCursor(257, 185);
		tft.print(headingArray[heading]);

	}

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

// Reset all displays after switching from metric to imperial or vice versa.

void metricImperialSwitch() {

	// Update temperature by rerunning functions.

	dataDisplayCurrentWeatherDayX(moonPhaseNow);

	dataDisplayForecastAlternateData(tft, 2, forecastTimeFc1, weatherDesFc1, tempDayFc1, tempNightFc1, tempMinFc1, tempMaxFc1, rainLevelFc1, rainPopFc1, windSpeedFc1, snowLevelFc1);
	dataDisplayForecastAlternateData(tft, 3, forecastTimeFc2, weatherDesFc2, tempDayFc2, tempNightFc2, tempMinFc2, tempMaxFc2, rainLevelFc2, rainPopFc2, windSpeedFc2, snowLevelFc2);
	dataDisplayForecastAlternateData(tft, 4, forecastTimeFc3, weatherDesFc3, tempDayFc3, tempNightFc3, tempMinFc3, tempMaxFc3, rainLevelFc3, rainPopFc3, windSpeedFc3, snowLevelFc3);

	// Update hourly view if set.

	if (dailyHourlyState == true) {

		enableScreen5();
		tft.fillScreen(WHITE);
		drawHourlyTempChart(tft);
		disableVSPIScreens();

		enableScreen6();
		drawHourlyRainChart(tft);
		disableVSPIScreens();

		enableScreen7();
		drawHourlyPressureChart(tft);
		disableVSPIScreens();

		enableScreen8();
		drawCompassChart(tft);
		disableVSPIScreens();

		dailyHourlyToggled = false;

	}

	// Otherwise update daily view.

	else {

		dataDisplayForecastAlternateData(tft, 5, forecastTimeFc4, weatherDesFc4, tempDayFc4, tempNightFc4, tempMinFc4, tempMaxFc4, rainLevelFc4, rainPopFc4, windSpeedFc4, snowLevelFc4);
		dataDisplayForecastAlternateData(tft, 6, forecastTimeFc5, weatherDesFc5, tempDayFc5, tempNightFc5, tempMinFc5, tempMaxFc5, rainLevelFc5, rainPopFc5, windSpeedFc5, snowLevelFc5);
		dataDisplayForecastAlternateData(tft, 7, forecastTimeFc6, weatherDesFc6, tempDayFc6, tempNightFc6, tempMinFc6, tempMaxFc6, rainLevelFc6, rainPopFc6, windSpeedFc6, snowLevelFc6);
		dataDisplayForecastAlternateData(tft, 8, forecastTimeFc7, weatherDesFc7, tempDayFc7, tempNightFc7, tempMinFc7, tempMaxFc7, rainLevelFc7, rainPopFc7, windSpeedFc7, snowLevelFc7);

	}

} // Close function.

/*-----------------------------------------------------------------*/

// Draw daily moon phase.

void drawDailyMoonPhase(double moonPhaseFc) {

	if (moonPhaseFc == 0 || moonPhaseFc == 1) {

		drawBitmap(tft, MOONPHASE_Y, MOONPHASE_X, newMoon, MOONPHASE_W, MOONPHASE_H);
	}

	else if (moonPhaseFc > 0 && moonPhaseFc < 0.15) {

		drawBitmap(tft, MOONPHASE_Y, MOONPHASE_X, waxingCrescentMoon1, MOONPHASE_W, MOONPHASE_H);

	}

	else if (moonPhaseFc >= 0.15 && moonPhaseFc < 0.25) {

		drawBitmap(tft, MOONPHASE_Y, MOONPHASE_X, waxingCrescentMoon2, MOONPHASE_W, MOONPHASE_H);

	}

	else if (moonPhaseFc == 0.25) {

		drawBitmap(tft, MOONPHASE_Y, MOONPHASE_X, firstQuarterMoon, MOONPHASE_W, MOONPHASE_H);

	}

	else if (moonPhaseFc > 0.25 && moonPhaseFc < 0.35) {

		drawBitmap(tft, MOONPHASE_Y, MOONPHASE_X, waxingGibbousMoon1, MOONPHASE_W, MOONPHASE_H);

	}

	else if (moonPhaseFc >= 0.35 && moonPhaseFc < 0.5) {

		drawBitmap(tft, MOONPHASE_Y, MOONPHASE_X, waxingGibbousMoon2, MOONPHASE_W, MOONPHASE_H);

	}

	else if (moonPhaseFc == 0.5) {

		drawBitmap(tft, MOONPHASE_Y, MOONPHASE_X, fullMoon, MOONPHASE_W, MOONPHASE_H);

	}

	else if (moonPhaseFc > 0.50 && moonPhaseFc < 0.65) {

		drawBitmap(tft, MOONPHASE_Y, MOONPHASE_X, waningGibbousMoon1, MOONPHASE_W, MOONPHASE_H);

	}

	else if (moonPhaseFc >= 0.65 && moonPhaseFc < 0.75) {

		drawBitmap(tft, MOONPHASE_Y, MOONPHASE_X, waningGibbousMoon2, MOONPHASE_W, MOONPHASE_H);

	}

	else if (moonPhaseFc == 0.75) {

		drawBitmap(tft, MOONPHASE_Y, MOONPHASE_X, lastQuarterMoon, MOONPHASE_W, MOONPHASE_H);

	}

	else if (moonPhaseFc > 0.75 && moonPhaseFc < 0.85) {

		drawBitmap(tft, MOONPHASE_Y, MOONPHASE_X, waningCrescentMoon1, MOONPHASE_W, MOONPHASE_H);

	}

	else if (moonPhaseFc >= 0.85 && moonPhaseFc < 1) {

		drawBitmap(tft, MOONPHASE_Y, MOONPHASE_X, waningCrescentMoon2, MOONPHASE_W, MOONPHASE_H);

	}

} // Close function.

/*-----------------------------------------------------------------*/

void controlBackLight() {

	// Control back lights

	if (backLightState == true) {

		mcp.digitalWrite(TFT_LED4_CTRL, HIGH);
		mcp.digitalWrite(TFT_LED8_CTRL, HIGH);
		delay(100);
		mcp.digitalWrite(TFT_LED3_CTRL, HIGH);
		mcp.digitalWrite(TFT_LED7_CTRL, HIGH);
		delay(100);
		mcp.digitalWrite(TFT_LED2_CTRL, HIGH);
		mcp.digitalWrite(TFT_LED6_CTRL, HIGH);
		delay(100);
		mcp.digitalWrite(TFT_LED1_CTRL, HIGH);
		mcp.digitalWrite(TFT_LED5_CTRL, HIGH);

		sleepScreenTime = millis();

	}

	else if (backLightState == false) {

		mcp.digitalWrite(TFT_LED1_CTRL, LOW);
		mcp.digitalWrite(TFT_LED5_CTRL, LOW);
		delay(100);
		mcp.digitalWrite(TFT_LED2_CTRL, LOW);
		mcp.digitalWrite(TFT_LED6_CTRL, LOW);
		delay(100);
		mcp.digitalWrite(TFT_LED3_CTRL, LOW);
		mcp.digitalWrite(TFT_LED7_CTRL, LOW);
		delay(100);
		mcp.digitalWrite(TFT_LED4_CTRL, LOW);
		mcp.digitalWrite(TFT_LED8_CTRL, LOW);

	}

} // Close function.

/*-----------------------------------------------------------------*/

void factoryReset() {

	// Blank settings.

	ssid = "";
	pass = "";
	openwk = "";
	latitude = "";
	longitude = "";

	// Write to SPIFFS.

	writeFile(SPIFFS, ssidPath, ssid.c_str());
	writeFile(SPIFFS, passPath, pass.c_str());
	writeFile(SPIFFS, openWKPath, openwk.c_str());
	writeFile(SPIFFS, latitudePath, latitude.c_str());
	writeFile(SPIFFS, longitudePath, longitude.c_str());

	// Clear screens.

	mcp.digitalWrite(TFT_LED1_CTRL, HIGH);
	mcp.digitalWrite(TFT_LED2_CTRL, HIGH);
	mcp.digitalWrite(TFT_LED3_CTRL, HIGH);
	mcp.digitalWrite(TFT_LED4_CTRL, HIGH);
	mcp.digitalWrite(TFT_LED5_CTRL, HIGH);
	mcp.digitalWrite(TFT_LED6_CTRL, HIGH);
	mcp.digitalWrite(TFT_LED7_CTRL, HIGH);
	mcp.digitalWrite(TFT_LED8_CTRL, HIGH);

	enableScreen1();
	tft.fillScreen(WHITE);
	drawBitmap(tft, 48, 96, rainbow, 128, 128);

	tft.setFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setTextColor(BLACK);
	tft.setCursor(96, 200);
	tft.println("Factory Reset");
	tft.setFont();

	disableVSPIScreens();

	enableScreen2();
	tft.fillScreen(WHITE);
	drawBitmap(tft, 48, 96, fswindsock, 128, 128);
	disableVSPIScreens();

	enableScreen3();
	tft.fillScreen(WHITE);
	drawBitmap(tft, 48, 96, fsatmospheric, 128, 128);
	disableVSPIScreens();

	enableScreen4();
	tft.fillScreen(WHITE);
	drawBitmap(tft, 48, 96, fshumidity, 128, 128);
	disableVSPIScreens();

	enableScreen5();
	tft.fillScreen(WHITE);
	drawBitmap(tft, 48, 96, fsrainfall, 128, 128);
	disableVSPIScreens();

	enableScreen6();
	tft.fillScreen(WHITE);
	drawBitmap(tft, 48, 96, fstemperature, 128, 128);
	disableVSPIScreens();

	enableScreen7();
	tft.fillScreen(WHITE);
	drawBitmap(tft, 48, 96, fsuv, 128, 128);
	disableVSPIScreens();

	enableScreen8();
	tft.fillScreen(WHITE);
	drawBitmap(tft, 48, 96, fsweathervain, 128, 128);
	disableVSPIScreens();

	delay(4000);

	ESP.restart();

} // Close function.

/*-----------------------------------------------------------------*/

// Draw sensor circle.

void drawHttpError() {

	// HTTP Response codes:
	// 0 = Blue = 429 = Calls exceeds account limit
	// 1 = Red = 401 = API key problem
	// 2 = Yellow = 404 = Incorrect city name, ZIP or city ID
	// 3 = Orange = 50X = You have to contact Open Weather
	// 4 = Green = 200 = Correct reponse
	// Else = Purple = Unknown

	// Draw sensor icon.

	enableScreen1();

	if (sensorT == 0) {

		tft.fillCircle(SENSOR_ICON_X, SENSOR_ICON_Y, SENSOR_ICON_R, BLUE);
	}

	else if (sensorT == 1) {

		tft.fillCircle(SENSOR_ICON_X, SENSOR_ICON_Y, SENSOR_ICON_R - 1, RED);
	}

	else if (sensorT == 2) {

		tft.fillCircle(SENSOR_ICON_X, SENSOR_ICON_Y, SENSOR_ICON_R - 1, YELLOW);
	}

	else if (sensorT == 3) {

		tft.fillCircle(SENSOR_ICON_X, SENSOR_ICON_Y, SENSOR_ICON_R - 1, ORANGE);
	}

	else if (sensorT == 4) {

		tft.fillCircle(SENSOR_ICON_X, SENSOR_ICON_Y, SENSOR_ICON_R - 1, GREEN);
	}

	else if (sensorT == 5) {

		tft.fillCircle(SENSOR_ICON_X, SENSOR_ICON_Y, SENSOR_ICON_R - 1, PURPLE);
	}

	disableVSPIScreens();

} // Close function.

/*-----------------------------------------------------------------*/