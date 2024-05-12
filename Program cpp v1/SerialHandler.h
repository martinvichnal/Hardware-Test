#pragma once
#include <windows.h>
#include <iostream>


class SerialHandler
{
public:
	SerialHandler();
	~SerialHandler();

	int begin(const char* portName);
	void close();

	int read(const char* buffer, unsigned int bufferSize);
	bool write(const char* buffer, unsigned int bufferSize);

	bool isConnected();

private:
	const char* _portName = "";
	bool _connected = false;
	HANDLE _serialHandler;
	COMSTAT _status;
	DWORD _dwByte;
};

