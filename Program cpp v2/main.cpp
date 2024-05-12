#define OLC_PGE_APPLICATION

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <cmath>
#include "olcPixelGameEngine.h"
#include "SerialHandler.h"
using namespace std;

#define DATA_FRAME_SIZE 11

#define SCREE_WIDTH 500
#define SCREE_HEIGHT 500
#define SCREE_PIXEL_SIZE 2
#define REAL_SCREEN_WIDTH (SCREE_WIDTH / SCREE_PIXEL_SIZE)	 // 255
#define REAL_SCREEN_HEIGHT (SCREE_HEIGHT / SCREE_PIXEL_SIZE) // 255

#define dataX1 0
#define dataY1 0
#define dataX2 REAL_SCREEN_WIDTH
#define dataY2 40

#define sensorX1 0
#define sensorY1 40
#define sensorX2 REAL_SCREEN_WIDTH
#define sensorY2 REAL_SCREEN_HEIGHT

// Message structure
typedef struct
{
	uint8_t start;		  // 1 byte - const 0x55
	uint8_t sonicData[4]; // 4 bytes - Sonic sensor data (float)
	uint8_t photoData[4]; // 4 bytes - Photo cell data (int)
	uint8_t cs;			  // 1 byte - Check sum error handling
	uint8_t end;		  // 1 byte - const 0xAA
} Message;

/*
Screen: 500x500 pixel size: 2x2 => !250x250!
Screen Data: (0, 0) - (250, 40)
Sensor Data: (0, 40) - (250, 250)
*/

/*     /$$                                                  /$$
	  | $$                                                 | $$
  /$$$$$$$  /$$$$$$  /$$$$$$  /$$  /$$  /$$        /$$$$$$$| $$  /$$$$$$   /$$$$$$$ /$$$$$$$
 /$$__  $$ /$$__  $$|____  $$| $$ | $$ | $$       /$$_____/| $$ |____  $$ /$$_____//$$_____/
| $$  | $$| $$  \__/ /$$$$$$$| $$ | $$ | $$      | $$      | $$  /$$$$$$$|  $$$$$$|  $$$$$$
| $$  | $$| $$      /$$__  $$| $$ | $$ | $$      | $$      | $$ /$$__  $$ \____  $$\____  $$
|  $$$$$$$| $$     |  $$$$$$$|  $$$$$/$$$$/      |  $$$$$$$| $$|  $$$$$$$ /$$$$$$$//$$$$$$$/
 \_______/|__/      \_______/ \_____/\___/        \_______/|__/ \_______/|_______/|_______/*/
class Draw : public olc::PixelGameEngine
{
private:
	SerialHandler port;
	Message buffer;

	const char *_portName = "\\\\.\\COM15";

	char incomingData[DATA_FRAME_SIZE];

	float fSonicData = 0.0f;
	int iPhotoData = 0;

	int window = 200;

	std::vector<float> sonicReadingVector;
	std::vector<float> photoReadingVector;

	// Function prototypes
	void handleIncommingData(void);
	void convertToMessage(float fSonicData, int iPhotoData, Message *buffer);
	void decodeMessage(Message *buffer, float *fSonicData, int *iPhotoData);
	bool parseMessage(const char *input, Message *buffer);
	uint8_t calculateCheckSum(Message *msg);

public:
	Draw()
	{
		sAppName = "Sensor Diagram";
	}

	~Draw()
	{
		port.close(); // Closing connection on port
	}

	// Writing out the data in hex format
	std::string hex(uint32_t n, uint8_t d)
	{
		std::string s(d, '0');
		for (int i = d - 1; i >= 0; i--, n >>= 4)
			s[i] = "0123456789ABCDEF"[n & 0xF];
		return s;
	};

	// DRAW SETUP
	bool OnUserCreate() override
	{
		std::cout << "[ port INFO ]: Starting a new port on: " << _portName << std::endl;
		port.begin(_portName); // Starting connection on port

		// Wait for connection
		while (port.isConnected() == false)
		{
			std::cout << "[ port ERR ]: Connection failed!" << std::endl;
			Sleep(1000);
			port.begin(_portName);
		}

		std::cout << "[ port OK ]: Connection established at port " << _portName << std::endl;

		return true;
	}

	// DRAW UPDATE
	bool OnUserUpdate(float fElapsedTime) override
	{
		// Clear screen
		Clear(olc::BLACK);

		// Read sensor data from UART
		int readResult = port.read(incomingData, DATA_FRAME_SIZE); // Reading from port into incomingData
		// The got packet is the right size
		if (readResult == DATA_FRAME_SIZE)
			handleIncommingData();

		// Add new sensor data to the beginning of the vector
		sonicReadingVector.push_back(fSonicData);
		photoReadingVector.push_back(iPhotoData);

		// Draw x and y axes
		DrawLine(20, ScreenHeight() - 20, ScreenWidth() - 20, ScreenHeight() - 20, olc::WHITE); // X-axis
		DrawLine(20, 50, 20, ScreenHeight() - 20, olc::WHITE);									// Y-axis

		DrawData(2, 2);
		DrawLine(0, 40, ScreenWidth(), 40);

		// Draw sensor 1 readings
		DrawSonic(sonicReadingVector, olc::GREEN);

		// Draw sensor 2 readings
		DrawPhoto(photoReadingVector, olc::BLUE);

		return true;
	}

	/**
	 * @brief Draws RAW HEX, Sonic and Photo data on the screen.
	 *
	 * @param x The x-coordinate of the starting position.
	 * @param y The y-coordinate of the starting position.
	 */
	void DrawData(int x, int y)
	{
		DrawString(x, y, "Raw data: ", olc::WHITE);
		int rawDataX = x, rawDataY = y + 10;
		for (int i = 0; i < (sizeof(incomingData) / sizeof(incomingData[0])); i++)
		{
			std::string sOffset = "0x" + hex(incomingData[i], 2);
			DrawString(rawDataX, rawDataY, sOffset, olc::WHITE);
			rawDataX += 40;
		}

		DrawString(x, y + 20, "Sonic data: " + std::to_string(fSonicData), olc::WHITE);
		DrawString(x, y + 30, "Photo data: " + std::to_string(iPhotoData), olc::WHITE);
	}

	/**
	 * @brief Draws the Sonic graph.
	 *
	 * @param sensorVector The vector containing the sensor data.
	 * @param color The color of the line.
	 */
	void DrawSonic(const std::vector<float> &sensorVector, const olc::Pixel &color)
	{
		int sensorSize = sensorVector.size();

		if (sensorSize < 2)
			return;

		float xIncrement = (ScreenWidth() - 100) / static_cast<float>(sensorSize - 1);
		float yIncrement = (ScreenHeight() - 100) / 35.0f;

		for (size_t i = 0; i < sensorSize - 1; ++i)
		{
			int x1 = static_cast<int>(i * xIncrement);
			int y1 = ScreenHeight() - 50 - static_cast<int>(sensorVector[i] * yIncrement);
			int x2 = static_cast<int>((i + 1) * xIncrement);
			int y2 = ScreenHeight() - 50 - static_cast<int>(sensorVector[i + 1] * yIncrement);

			DrawLine(x1 + 20, y1, x2 + 20, y2, color);
		}
	}

	/**
	 * @brief Draws the Photo graph.
	 *
	 * @param sensorVector The vector containing the sensor data.
	 * @param color The color of the line.
	 */
	void DrawPhoto(const std::vector<float> &sensorVector, const olc::Pixel &color)
	{
		int sensorSize = sensorVector.size();

		if (sensorSize < 2)
			return;

		float xIncrement = (ScreenWidth() - 100) / static_cast<float>(sensorSize - 1);
		float yIncrement = (ScreenHeight() - 100) / 1024.0f;

		for (size_t i = 0; i < sensorSize - 1; ++i)
		{
			int x1 = static_cast<int>(i * xIncrement);
			int y1 = ScreenHeight() - 50 - static_cast<int>(sensorVector[i] * yIncrement);
			int x2 = static_cast<int>((i + 1) * xIncrement);
			int y2 = ScreenHeight() - 50 - static_cast<int>(sensorVector[i + 1] * yIncrement);

			DrawLine(x1 + 20, y1, x2 + 20, y2, color);
		}
	}
};

/*						 /$$
						|__/
 /$$$$$$/$$$$   /$$$$$$  /$$ /$$$$$$$
| $$_  $$_  $$ |____  $$| $$| $$__  $$
| $$ \ $$ \ $$  /$$$$$$$| $$| $$  \ $$
| $$ | $$ | $$ /$$__  $$| $$| $$  | $$
| $$ | $$ | $$|  $$$$$$$| $$| $$  | $$
|__/ |__/ |__/ \_______/|__/|__/  |__/*/
int main()
{
	Draw diagrams;
	if (diagrams.Construct(SCREE_WIDTH, SCREE_WIDTH, SCREE_PIXEL_SIZE, SCREE_PIXEL_SIZE))
	{
		diagrams.Start();
	}
}

/*/$$$$$$                                 /$$     /$$
 /$$__  $$                               | $$    |__/
| $$  \__//$$   /$$ /$$$$$$$   /$$$$$$$ /$$$$$$   /$$  /$$$$$$  /$$$$$$$   /$$$$$$$
| $$$$   | $$  | $$| $$__  $$ /$$_____/|_  $$_/  | $$ /$$__  $$| $$__  $$ /$$_____/
| $$_/   | $$  | $$| $$  \ $$| $$        | $$    | $$| $$  \ $$| $$  \ $$|  $$$$$$
| $$     | $$  | $$| $$  | $$| $$        | $$ /$$| $$| $$  | $$| $$  | $$ \____  $$
| $$     |  $$$$$$/| $$  | $$|  $$$$$$$  |  $$$$/| $$|  $$$$$$/| $$  | $$ /$$$$$$$/
|__/      \______/ |__/  |__/ \_______/   \___/  |__/ \______/ |__/  |__/|_______/*/
//==================================================================================================
void Draw::handleIncommingData(void)
{
	bool parseResult = parseMessage(incomingData, &buffer); // Parsing incomming data into buffer

	// Writing out the data from the buffer
	if (parseResult == 0)
	{
		for (int i = 0; i < sizeof(Message); i++)
		{
			printf("0x%02x ", ((uint8_t *)&buffer)[i]);
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
void Draw::convertToMessage(float fSonicData, int iPhotoData, Message *buffer)
{
	buffer->start = 0x55;
	*(float *)buffer->sonicData = fSonicData;
	*(int *)buffer->photoData = iPhotoData;
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
void Draw::decodeMessage(Message *buffer, float *fSonicData, int *iPhotoData)
{
	*fSonicData = *(float *)buffer->sonicData;
	*iPhotoData = *(int *)buffer->photoData;
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
bool Draw::parseMessage(const char *input, Message *buffer)
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
uint8_t Draw::calculateCheckSum(Message *msg)
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
