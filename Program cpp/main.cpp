#include <iostream>
#include "SerialHandler.h"
#include <stdio.h>
#include <string.h>
using namespace std;

// Message structure
typedef struct
{
	uint8_t start;		  // 1 byte - const 0x55
	uint8_t sonicData[4]; // 4 bytes - Sonic sensor data (float)
	uint8_t photoData[4]; // 4 bytes - Photo cell data (int)
	uint8_t cs;			  // 1 byte - Check sum error handling
	uint8_t end;		  // 1 byte - const 0xAA
} Message;

// Function prototypes
void handleIncommingData(void);
void convertToMessage(float fSonicData, int iPhotoData, Message* buffer);
void decodeMessage(Message* buffer, float* fSonicData, int* iPhotoData);
bool parseMessage(const char* input, Message* buffer);
uint8_t calculateCheckSum(Message* msg);

// Class declarations
// SerialPort* arduino;
Message buffer;

// Global variables
#define DATA_FRAME_SIZE 11

const char* portName = "\\\\.\\COM15";
char incomingData[DATA_FRAME_SIZE];
float fSonicData = 0.0;
int iPhotoData = 0;

/*
						 /$$
						|__/
 /$$$$$$/$$$$   /$$$$$$  /$$ /$$$$$$$
| $$_  $$_  $$ |____  $$| $$| $$__  $$
| $$ \ $$ \ $$  /$$$$$$$| $$| $$  \ $$
| $$ | $$ | $$ /$$__  $$| $$| $$  | $$
| $$ | $$ | $$|  $$$$$$$| $$| $$  | $$
|__/ |__/ |__/ \_______/|__/|__/  |__/
*/
int main()
{
	SerialHandler port(portName);

	std::cout << "[ port INFO ]: Starting a new port on: " << portName << std::endl;
	port.begin();	// Starting connection on port

	// Wait for connection
	while (port.isConnected() == false)
	{
		std::cout << "[ port ERR ]: Connection failed!" << std::endl;
		Sleep(1000);
		port.begin();
	}

	std::cout << "[ port OK ]: Connection established at port " << portName << std::endl;

	while (1)
	{
		int readResult = port.read(incomingData, DATA_FRAME_SIZE); // Reading from port into incomingData
		// The got packet is the right size
		if (readResult == DATA_FRAME_SIZE)	handleIncommingData();

	}
	port.close(); // Closing connection on port
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
//==================================================================================================
void handleIncommingData(void)
{
	bool parseResult = parseMessage(incomingData, &buffer); // Parsing incomming data into buffer

	// Writing out the data from the buffer
	if (parseResult == 0)
	{
		for (int i = 0; i < sizeof(Message); i++)
		{
			printf("%02x", ((uint8_t*)&buffer)[i]);
		}
		std::cout << std::endl;
		decodeMessage(&buffer, &fSonicData, &iPhotoData);
		std::cout << "Sonic data: " << fSonicData << std::endl;
		std::cout << "Photo data: " << iPhotoData << std::endl;
	}
}

//==================================================================================================
/**
 * @brief Converts the given float and int data into a Message object.
 *
 * @param fSonicData The float value representing sonic data.
 * @param iPhotoData The int value representing photo data.
 * @param buffer Pointer to the Message object to be populated.
 */
void convertToMessage(float fSonicData, int iPhotoData, Message* buffer)
{
	buffer->start = 0x55;
	*(float*)buffer->sonicData = fSonicData;
	*(int*)buffer->photoData = iPhotoData;
	buffer->cs = calculateCheckSum(buffer);
	buffer->end = 0xAA;
}

//==================================================================================================
/**
 * @brief Decodes the given Message object into float and int data.
 *
 * @param buffer Pointer to the Message object to be decoded.
 * @param fSonicData Pointer to the float variable to be populated.
 * @param iPhotoData Pointer to the int variable to be populated.
 */
void decodeMessage(Message* buffer, float* fSonicData, int* iPhotoData)
{
	*fSonicData = *(float*)buffer->sonicData;
	*iPhotoData = *(int*)buffer->photoData;
}

//==================================================================================================
/**
 * @brief Parses the given input buffer into a Message object.
 *
 * @param input The input buffer to be parsed.
 * @param buffer Pointer to the Message object to be populated.
 * @return true If the start byte is not correct.
 * @return false If the start byte is correct.
 */
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

//==================================================================================================
/**
 * @brief Calculates the check sum of the given Message object.
 *
 * @param msg Pointer to the Message object to calculate the check sum for.
 * @return uint8_t The calculated check sum.
 */
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
