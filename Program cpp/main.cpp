#include <iostream>
#include "SerialPort.hpp"
#include <stdio.h>
#include <string.h>
using namespace std;

typedef struct
{
	uint8_t start;		  // 1 byte - const 0x55
	uint8_t sonicData[4]; // 4 bytes - Sonic sensor data (float)
	uint8_t photoData[4]; // 4 bytes - Photo cell data (int)
	uint8_t cs;			  // 1 byte - Check sum error handling
	uint8_t end;		  // 1 byte - const 0xAA
} Message;

void handleIncommingData(void);
void convertToMessage(float fSonicData, int iPhotoData, Message* buffer);
void decodeMessage(Message* buffer, float* fSonicData, int* iPhotoData);
bool parseMessage(const char* input, Message* buffer);
uint8_t calculateCheckSum(Message* msg);

SerialPort* arduino;
Message buffer;

#define MAX_DATA_LENGTH 11

const char* portName = "\\\\.\\COM15";
char incomingData[MAX_DATA_LENGTH];
float fSonicData = 0.0;
int iPhotoData = 0;


int main()
{
	arduino = new SerialPort(portName);

	while (1) {
		// ui - searching
		std::cout << "Searching in progress";
		// wait connection
		while (!arduino->isConnected()) {
			Sleep(100);
			std::cout << ".";
			arduino = new SerialPort(portName);
		}

		//Checking if arduino is connected or not
		if (arduino->isConnected()) {
			std::cout << std::endl << "Connection established at port " << portName << std::endl;
		}

		while (arduino->isConnected())
		{
			handleIncommingData();
		}
	}
}

void handleIncommingData(void)
{
	int readResult = arduino->readSerialPort(incomingData, MAX_DATA_LENGTH);
	bool readIsSuccessfull = parseMessage(incomingData, &buffer);
	if (readIsSuccessfull == 0)
	{
		for (int i = 0; i < sizeof(Message); i++)
		{
			printf("%02X ", ((uint8_t*)&buffer)[i]);
		}
		std::cout << std::endl;
		decodeMessage(&buffer, &fSonicData, &iPhotoData);
		std::cout << "Sonic data: " << fSonicData << std::endl;
		std::cout << "Photo data: " << iPhotoData << std::endl;
	}

	Sleep(10);
}

void convertToMessage(float fSonicData, int iPhotoData, Message* buffer)
{
	buffer->start = 0x55;
	*(float*)buffer->sonicData = fSonicData;
	*(int*)buffer->photoData = iPhotoData;
	buffer->cs = calculateCheckSum(buffer);
	buffer->end = 0xAA;
}

void decodeMessage(Message* buffer, float* fSonicData, int* iPhotoData)
{
	*fSonicData = *(float*)buffer->sonicData;
	*iPhotoData = *(int*)buffer->photoData;
}

bool parseMessage(const char* input, Message* buffer)
{
	Message tmp;
	// tmp buffer to check if the start byte is correct
	memcpy(&tmp, input, sizeof(Message));

	if (tmp.start == 0x55)
	{
		memcpy(buffer, input, sizeof(Message));

		// Error checking on incomming buffer
		uint8_t checkSum = calculateCheckSum(buffer);
		printf("Check sum: %d\n", checkSum);
		if (checkSum != buffer->cs)
		{
			printf("Check sum error on buffer!");
			return 1;
		}
		return 0;
	}
	return 1;
}

uint8_t calculateCheckSum(Message* msg)
{
	uint8_t checkSum = 0;
	uint8_t res = 0;
	uint8_t* ptr = (uint8_t*)msg;
	for (int i = 0; i < (sizeof(Message) - (2 * sizeof(uint8_t))); i++)
	{
		checkSum ^= *ptr;
		ptr++;
	}
	res = checkSum;
	return res;
}
