#include <Arduino.h>
#include <stdbool.h>
#include <LiquidCrystal_I2C.h>
#include "AntiDelay.h"

#define DEBUG 0

#define TRIGGER_PIN 5
#define ECHO_PIN 4
#define LED1 8
#define LED2 7
#define LED3 6
#define PHOTOCELL A0

#define LCD_COLS 16
#define LCD_ROWS 4
#define LCD_1_ADDR 0x70
#define LCD_2_ADDR 0x7E

// Message structure
typedef struct
{
	uint8_t start;		  // 1 byte - const 0x55
	uint8_t sonicData[4]; // 4 bytes - Sonic sensor data (float)
	uint8_t photoData[4]; // 4 bytes - Photo cell data (int)
	uint8_t cs;			  // 1 byte - Check sum error handling
	uint8_t end;		  // 1 byte - const 0xAA
} Message;

// Sonic sensor class
class Sonic
{
private:
	int _trigPin;
	int _echoPin;

public:
	/**
	 * @brief Constructor
	 * @note Sets pinMode for trigger and echo pins
	 *
	 * @param trigPin
	 * @param echoPin
	 */
	Sonic(int trigPin, int echoPin)
	{
		_trigPin = trigPin;
		_echoPin = echoPin;
		pinMode(_trigPin, OUTPUT);
		pinMode(_echoPin, INPUT);
	}

	/**
	 * @brief Get distance from ultrasonic sensor
	 *
	 * @return float
	 */
	float getDistance()
	{
		float res = 0;
		long time = 0;

		digitalWrite(_trigPin, LOW);
		delayMicroseconds(2);
		digitalWrite(_trigPin, HIGH);
		delayMicroseconds(10);
		digitalWrite(_trigPin, LOW);
		time = pulseIn(_echoPin, HIGH);
		res = (time / 2) * 0.0343f; // [cm] || 0.0343f [cm / microsecond]
		return res;
	}
};

// Fucntions declarations

bool sendUARTMessage(Message *msg);
void convertToMessage(float fSonicData, int iPhotoData, Message *buffer);
bool decodeMessage(Message *buffer, float *fSonicData, int *iPhotoData);
uint8_t calculateCheckSum(Message *msg);
void handleLEDs();
void initLCD();
void writeLCD();

// Class declarations
LiquidCrystal_I2C lcd1(LCD_1_ADDR, LCD_COLS, LCD_ROWS);
LiquidCrystal_I2C lcd2(LCD_2_ADDR, LCD_COLS, LCD_ROWS);
Sonic sonicSensor(TRIGGER_PIN, ECHO_PIN);
AntiDelay sensorReadings(500);
Message buffer;

// Global variable declarations
int photoCellValue = 0;
float sonicDistance = 0;
const float ledUpperLimit = 15.00f;	 // [cm]
const float ledBottomLimit = 10.00f; // [cm]

//==================================================================================================
// Setup
void setup()
{
	pinMode(LED1, OUTPUT);
	pinMode(LED2, OUTPUT);
	pinMode(LED3, OUTPUT);
	pinMode(PHOTOCELL, INPUT);

	Serial.begin(9600);

	delay(500);
}

/*
 /$$
| $$
| $$  /$$$$$$   /$$$$$$   /$$$$$$
| $$ /$$__  $$ /$$__  $$ /$$__  $$
| $$| $$  \ $$| $$  \ $$| $$  \ $$
| $$| $$  | $$| $$  | $$| $$  | $$
| $$|  $$$$$$/|  $$$$$$/| $$$$$$$/
|__/ \______/  \______/ | $$____/
						| $$
						| $$
						|__/
*/
void loop()
{
	if (sensorReadings)
	{
		photoCellValue = analogRead(PHOTOCELL);
		sonicDistance = sonicSensor.getDistance();
		convertToMessage(sonicDistance, photoCellValue, &buffer);
		sendUARTMessage(&buffer);
		writeLCD();
#if DEBUG
		Serial.print("Photo cell value: ");
		Serial.println(photoCellValue);
		Serial.print("Sonic distance: ");
		Serial.println(sonicDistance);
		Serial.print("Sending message: ");
		for (int i = 0; i < sizeof(Message); i++)
		{
			uint8_t *ptr = (uint8_t *)&buffer;
			Serial.print(ptr[i], HEX);
			Serial.print(" ");
			ptr++;
		}
		Serial.println();
#endif
	}
	handleLEDs();
}

/*
  /$$$$$$                                 /$$     /$$
 /$$__  $$                               | $$    |__/
| $$  \__//$$   /$$ /$$$$$$$   /$$$$$$$ /$$$$$$   /$$  /$$$$$$  /$$$$$$$   /$$$$$$$
| $$$$   | $$  | $$| $$__  $$ /$$_____/|_  $$_/  | $$ /$$__  $$| $$__  $$ /$$_____/
| $$_/   | $$  | $$| $$  \ $$| $$        | $$    | $$| $$  \ $$| $$  \ $$|  $$$$$$
| $$     | $$  | $$| $$  | $$| $$        | $$ /$$| $$| $$  | $$| $$  | $$ \____  $$
| $$     |  $$$$$$/| $$  | $$|  $$$$$$$  |  $$$$/| $$|  $$$$$$/| $$  | $$ /$$$$$$$/
|__/      \______/ |__/  |__/ \_______/   \___/  |__/ \______/ |__/  |__/|_______/
*/
/**
 * @brief Send message over UART
 *
 * @param Message*
 * @return true
 */
bool sendUARTMessage(Message *msg)
{
	uint8_t *msgPtr = (uint8_t *)msg;
	uint8_t msgSize = sizeof(Message);

	for (uint8_t i = 0; i < msgSize; i++)
	{
		Serial.write(msgPtr[i]);
	}

	return true;
}

//==================================================================================================
/**
 * @brief Convert data to message
 *
 * @param float fSonicData
 * @param int iPhotoData
 * @param Message* buffer
 */
void convertToMessage(float fSonicData, int iPhotoData, Message *buffer)
{
	buffer->start = 0x55;
	*(float *)buffer->sonicData = fSonicData;
	*(int *)buffer->photoData = iPhotoData;
	buffer->cs = calculateCheckSum(buffer);
	buffer->end = 0xAA;
}

//==================================================================================================
/**
 * @brief Decode message into sonic and photo data address
 *
 * @param Message* buffer
 * @param float* fSonicData
 * @param int* iPhotoData
 * @return bool - 1 if check sum error, 0 if no error
 */
bool decodeMessage(Message *buffer, float *fSonicData, int *iPhotoData)
{
	*fSonicData = *(float *)buffer->sonicData;
	*iPhotoData = *(int *)buffer->photoData;

	// Error checking on incomming buffer
	uint8_t checkSum = calculateCheckSum(buffer);
	if (checkSum != buffer->cs)
	{
#if DEBUG
		Serial.println("Check sum error on buffer!");
#endif
		return 1;
	}
	return 0;
}

//==================================================================================================
/**
 * @brief Calculate check sum
 *
 * @param Message*
 * @return uint8_t
 */
uint8_t calculateCheckSum(Message *msg)
{
	uint8_t checkSum = 0;
	uint8_t res = 0;
	uint8_t *ptr = (uint8_t *)msg;
	for (int i = 0; i < (sizeof(Message) - (2 * sizeof(uint8_t))); i++)
	{
		checkSum ^= *ptr;
		ptr++;
	}
	res = checkSum;
	return res;
}

//==================================================================================================
/**
 * @brief Handle LEDs
 *
 */
void handleLEDs()
{
	if (sonicDistance < ledBottomLimit)
	{
		digitalWrite(LED1, HIGH);
		digitalWrite(LED2, LOW);
		digitalWrite(LED3, LOW);
	}
	else if ((sonicDistance > ledBottomLimit) && (sonicDistance < ledUpperLimit))
	{
		digitalWrite(LED1, LOW);
		digitalWrite(LED2, HIGH);
		digitalWrite(LED3, LOW);
	}
	else if (sonicDistance > ledUpperLimit)
	{
		digitalWrite(LED1, LOW);
		digitalWrite(LED2, LOW);
		digitalWrite(LED3, HIGH);
	}
	else
	{
		digitalWrite(LED1, LOW);
		digitalWrite(LED2, LOW);
		digitalWrite(LED3, LOW);
	}
}

//==================================================================================================
/**
 * @brief Initialize LCD
 *
 */
void initLCD()
{
	lcd1.init();		  // Initialize the LCD
	lcd1.backlight();	  // Turn on the backlight
	lcd1.clear();		  // Clear the screen
	lcd1.setCursor(0, 0); // Set cursor to the begining

	lcd2.init();		  // Initialize the LCD
	lcd2.backlight();	  // Turn on the backlight
	lcd2.clear();		  // Clear the screen
	lcd2.setCursor(0, 0); // Set cursor to the begining
}

//==================================================================================================
/**
 * @brief Write to LCD
 *
 */
void writeLCD()
{
	lcd1.clear();
	lcd1.setCursor(0, 0);
	lcd1.print("Photo cell: ");
	lcd1.setCursor(0, 1);
	lcd1.print(photoCellValue);

	lcd1.clear();
	lcd2.setCursor(0, 0);
	lcd2.print("Sonic distance: ");
	lcd2.setCursor(0, 1);
	lcd2.print(sonicDistance);
}