#include "SerialHandler.h"

/**
 * @brief Construct a new serial handler object
 *
 */
SerialHandler::SerialHandler()
{
	_connected = false;
}

/**
 * @brief Deconstruct the Serial serial handler object
 *
 */
SerialHandler::~SerialHandler()
{
	if (_connected)
	{
		_connected = false;
		CloseHandle(_serialHandler);
	}
}

/**
 * @brief Initializes the serial port connection.
 *
 * @details The function initializes the serial port connection with the given port, sets the parameters for the serial port, and sets the timeouts for the serial port.
 * PORT parameters:
 * - Baud rate: 9600
 * - Data bits: 8
 * - Stop bits: 1
 * - Parity: None
 *
 * @param portName
 * 
 * @return	1 - Connection established
 * @return -1 - Serial port not available
 * @return -2 - Could not connect to serial port
 * @return -3 - Could not get serial port parameters
 * @return -4 - Could not set serial port parameters
 * @return -5 - Could not set timeouts
 */
int SerialHandler::begin(const char* portName)
{
	_portName = portName;
	_connected = false;

	_serialHandler = CreateFileA(static_cast<LPCSTR>(_portName), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, 0);

	// Checking if the serial port was opened successfully
	if (_serialHandler == INVALID_HANDLE_VALUE)
	{
		if (GetLastError() == ERROR_FILE_NOT_FOUND)
		{
			std::cerr << "[ Serial ERR ]: " << _portName << " not available\n";
			return -1;
		}
		std::cerr << "[ Serial ERR ]: could not connect to serial port\n";
		return -2;
	}

	// Setting parameters for the serial port using DCB struct
	DCB serialParam = { 0 };
	serialParam.DCBlength = sizeof(serialParam);

	if (!GetCommState(_serialHandler, &serialParam))
	{
		std::cerr << "[ Serial ERR ]: could not get serial port parameters\n";
		return -3;
	}

	// Setting up parameters: Baud 9600, 8 bits, 1 stop bit, no parity
	serialParam.BaudRate = CBR_9600;
	serialParam.ByteSize = 8;
	serialParam.StopBits = ONESTOPBIT;
	serialParam.Parity = NOPARITY;

	if (!SetCommState(_serialHandler, &serialParam))
	{
		std::cout << "[ Serial ERR ]: could not set serial port parameters\n";
		return -4;
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
		return -5;
	}

	// The port has been successfully opened
	this->_connected = true;
	std::cout << "[ Serial OK ]: Connection established at port " << _portName << "\n"<< std::endl;
	PurgeComm(_serialHandler, PURGE_RXCLEAR | PURGE_TXCLEAR);
	Sleep(2000);
	return 1;
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
 * @brief Reads data from the serial port into the specified buffer.
 *
 * @param buffer The buffer to store the read data.
 * @param bufferSize The size of the buffer.
 *
 * @return _dwByte - The number of bytes read from the serial port.
 * @return -1 - Could not read from the serial port.
 */
int SerialHandler::read(const char* buffer, unsigned int bufferSize)
{
	unsigned int readSize = 0;

	ClearCommError(_serialHandler, &_dwByte, &_status);

	if (_status.cbInQue > 0)
	{
		if (_status.cbInQue > bufferSize)
		{
			readSize = bufferSize;
		}
		else
		{
			readSize = _status.cbInQue;
		}
	}

	if (ReadFile(_serialHandler, (void*)buffer, readSize, &_dwByte, NULL))
	{
		return _dwByte;
	}
	return -1;
}

/**
 * @brief Writes to the serial port
 *
 * @param buffer The buffer to write to the serial port
 * @param bufferSize The size of the buffer
 * 
 * @return 1 - Write successful
 * @return -1 - Could not write to the serial port
 */
bool SerialHandler::write(const char* buffer, unsigned int bufferSize)
{
	if (!WriteFile(_serialHandler, buffer, bufferSize, &_dwByte, NULL))
	{
		std::cerr << "[Serial ERR]: could not write to serial port\n";
		return -1;
	}
	return 1;
}

/**
 * @brief Checks if the serial port is connected
 *
 * @return Returns true if the serial port is connected
 */
bool SerialHandler::isConnected()
{
	return _connected;
}