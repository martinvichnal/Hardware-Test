#include "SerialHandler.h"

/**
* @brief Constructor for the SerialHandler class
* @details Opens the serial port and sets the parameters for the serial port
* @param portName The name of the port to open
*
*/
SerialHandler::SerialHandler(const char* portName)
{
	_portName = portName;
}

/**
* @brief Destructor for the SerialHandler class
* @details Closes the serial port if it is still open
*/
SerialHandler::~SerialHandler()
{
	if (_connected)
	{
		_connected = false;
		CloseHandle(_serialHandler);
	}
}

bool SerialHandler::begin()
{
	_connected = false;

	_serialHandler = CreateFileA(static_cast<LPCSTR>(_portName), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, 0);

	// Checking if the serial port was opened successfully
	if (_serialHandler == INVALID_HANDLE_VALUE)
	{
		if (GetLastError() == ERROR_FILE_NOT_FOUND)
		{
			std::cerr << "[ Serial ERR ]: " << _portName << " not available\n";
			return 1;
		}
		std::cerr << "[ Serial ERR ]: could not connect to serial port\n";
		return 1;
	}


	// Setting parameters for the serial port using DCB struct
	DCB serialParam = { 0 };
	serialParam.DCBlength = sizeof(serialParam);

	if (!GetCommState(_serialHandler, &serialParam)) {
		std::cerr << "[ Serial ERR ]: could not get serial port parameters\n";
		return 1;
	}

	// Setting up parameters: Baud 9600, 8 bits, 1 stop bit, no parity
	serialParam.BaudRate = CBR_9600;
	serialParam.ByteSize = 8;
	serialParam.StopBits = ONESTOPBIT;
	serialParam.Parity = NOPARITY;

	if (!SetCommState(_serialHandler, &serialParam))
	{
		std::cout << "[ Serial ERR ]: could not set serial port parameters\n";
		return 1;
	}

	// Setting timeouts for the serial port
	COMMTIMEOUTS timeout = { 0 };
	timeout.ReadIntervalTimeout = 60;
	timeout.ReadTotalTimeoutConstant = 60;
	timeout.ReadTotalTimeoutMultiplier = 15;
	timeout.WriteTotalTimeoutConstant = 60;
	timeout.WriteTotalTimeoutMultiplier = 8;

	if (!SetCommTimeouts(_serialHandler, &timeout))
	{
		std::cerr << "[ Serial ERR ]: could not set timeouts\n";
		return 1;
	}

	// The port has been successfully opened
	this->_connected = true;
	std::cout << "[ Serial OK ]: Connection established at port " << _portName << "\n" << std::endl;
	PurgeComm(_serialHandler, PURGE_RXCLEAR | PURGE_TXCLEAR);
	Sleep(2000);
	return 0;
}

/**
* @brief Closes down the serial port
*/
void SerialHandler::close()
{
	_connected = false;
	CloseHandle(_serialHandler);
}

/**
* @brief Reads from the serial port
* @param buffer The buffer to read into
* @param bufferSize The size of the buffer
* @return Returns the number of bytes read
*/
int SerialHandler::read(const char* buffer, unsigned int bufferSize)
{
	unsigned int toRead = 0;

	ClearCommError(_serialHandler, &_dwByte, &_status);

	if (_status.cbInQue > 0)
	{
		if (_status.cbInQue > bufferSize)
		{
			toRead = bufferSize;
		}
		else
		{
			toRead = _status.cbInQue;
		}
	}

	if (ReadFile(_serialHandler, (void*)buffer, toRead, &_dwByte, NULL))
	{
		return _dwByte;
	}
	return 0;
}

/**
* @brief Writes to the serial port
* @param buffer The buffer to write to the serial port
* @param bufferSize The size of the buffer
* @return Returns 1 if the write was successful
*/
bool SerialHandler::write(const char* buffer, unsigned int bufferSize)
{
	if (!WriteFile(_serialHandler, buffer, bufferSize, &_dwByte, NULL))
	{
		std::cerr << "[Serial ERR]: could not write to serial port\n";
		return 1;
	}
	return 0;
}

/**
* @brief Checks if the serial port is connected
* @return Returns true if the serial port is connected
* */
bool SerialHandler::isConnected()
{
	return _connected;
}