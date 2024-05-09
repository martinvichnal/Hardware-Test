#include <Arduino.h>
#include <stdbool.h>
#include "AntiDelay.h"

#define DEBUG 1

#define TRIGGER_PIN 5
#define ECHO_PIN 4
#define LED1 8
#define LED2 7
#define LED3 6
#define PHOTOCELL A0

typedef struct
{
	uint8_t start;		  // 1 byte - const 0x55
	uint8_t sonicData[4]; // 4 bytes - Sonic sensor data (float)
	uint8_t photoData[4]; // 2 bytes - Photo cell data (int)
	uint8_t cs;			  // 1 byte - Check sum error handling
	uint8_t end;		  // 1 byte - const 0xAA
} Message;

class Sonic
{
private:
	int _trigPin;
	int _echoPin;

public:
	Sonic(int trigPin, int echoPin)
	{
		_trigPin = trigPin;
		_echoPin = echoPin;
		pinMode(_trigPin, OUTPUT);
		pinMode(_echoPin, INPUT);
	}

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

bool sendUARTMessage(Message *msg);
void convertToMessage(float fSonicData, int iPhotoData, Message *buffer);
bool decodeMessage(Message *buffer, float *fSonicData, int *iPhotoData);
uint8_t calculateCheckSum(Message *msg);
void handleLEDs();

Sonic sonicSensor(TRIGGER_PIN, ECHO_PIN);
AntiDelay sensorReadings(500);
Message buffer;

int photoCellValue = 0;
float sonicDistance = 0;
const float ledUpperLimit = 15.00f;	 // [cm]
const float ledBottomLimit = 10.00f; // [cm]

void setup()
{
	pinMode(LED1, OUTPUT);
	pinMode(LED2, OUTPUT);
	pinMode(LED3, OUTPUT);
	pinMode(PHOTOCELL, INPUT);

	Serial.begin(9600);

	delay(500);
}

void loop()
{
	if (sensorReadings)
	{
		photoCellValue = analogRead(PHOTOCELL);
		sonicDistance = sonicSensor.getDistance();
		convertToMessage(sonicDistance, photoCellValue, &buffer);
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
		sendUARTMessage(&buffer);
	}
	handleLEDs();
}

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

void convertToMessage(float fSonicData, int iPhotoData, Message *buffer)
{
	buffer->start = 0x55;
	*(float *)buffer->sonicData = fSonicData;
	*(int *)buffer->photoData = iPhotoData;
	buffer->cs = calculateCheckSum(buffer);
	buffer->end = 0xAA;
}

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
