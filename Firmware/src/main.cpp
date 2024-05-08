#include <Arduino.h>
#include <stdbool.h>
#include "AntiDelay.h"
#include "Sonic.h"

#define DEBUG 0

#define TRIGGER_PIN 5
#define ECHO_PIN 4
#define SOUND_OF_SPEED 0.0343f // [cm / microsecond]

#define LED1 8
#define LED2 7
#define LED3 6
const float ledUpperLimit = 15.00f;	 // [cm]
const float ledBottomLimit = 10.00f; // [cm]

#define PHOTOCELL A0

typedef struct
{
	uint8_t start;		  // 1 byte - const 0x55
	uint8_t sonicData[4]; // 4 bytes - Sonic sensor data (float)
	uint8_t photoData[4]; // 2 bytes - Photo cell data (int)
	uint8_t cs;			  // 1 byte - Check sum error handling
	uint8_t end;		  // 1 byte - const 0xAA
} Message;

bool sendUARTMessage(Message *msg);
void convertToMessage(float fSonicData, int iPhotoData, Message *buffer);
void decodeMessage(Message *buffer, float *fSonicData, int *iPhotoData);
uint8_t calculateCheckSum(Message *msg);

float getDistance();
void handleLEDs();

// Sonic sonicSensor(trigPin, echoPin);
AntiDelay sensorReadings(500);
Message buffer;

int photoCellValue = 0;
float sonicDistance = 0;

void setup()
{
	pinMode(LED1, OUTPUT);
	pinMode(LED2, OUTPUT);
	pinMode(LED3, OUTPUT);
	pinMode(PHOTOCELL, INPUT);
	pinMode(TRIGGER_PIN, OUTPUT);
	pinMode(ECHO_PIN, INPUT);

	Serial.begin(9600);

	delay(500);
}

void loop()
{
	if (sensorReadings)
	{
		photoCellValue = analogRead(PHOTOCELL);
		sonicDistance = getDistance();
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
	uint8_t *ptr = (uint8_t *)buffer;
	*ptr = 0x55;
	ptr++;
	*(float *)ptr = fSonicData;
	// ptr += sizeof(float);
	ptr += 4;
	*(int *)ptr = iPhotoData;
	// ptr += sizeof(int);
	ptr += 4;
	*ptr = calculateCheckSum(buffer);
	ptr++;
	*ptr = 0xAA;
}

void decodeMessage(Message *buffer, float *fSonicData, int *iPhotoData)
{
	uint8_t *ptr = (uint8_t *)buffer;
	ptr++;
	*fSonicData = *(float *)ptr;
	ptr += 4;
	*iPhotoData = *(int *)ptr;
	ptr += 2;
}

uint8_t calculateCheckSum(Message *msg)
{
	uint8_t checkSum = 0;
	uint8_t res = 0;
	uint8_t *ptr = (uint8_t *)msg;
	for (int i = 0; i < (sizeof(Message) - 2); i++)
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

float getDistance()
{
	float res = 0;
	long time = 0;

	digitalWrite(TRIGGER_PIN, LOW);
	delayMicroseconds(2);
	digitalWrite(TRIGGER_PIN, HIGH);
	delayMicroseconds(10);
	digitalWrite(TRIGGER_PIN, LOW);
	time = pulseIn(ECHO_PIN, HIGH);
	res = (time / 2) * SOUND_OF_SPEED;
	return res;
}