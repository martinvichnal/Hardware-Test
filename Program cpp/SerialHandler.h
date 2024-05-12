#pragma once
#include <windows.h>
#include <iostream>


class SerialHandler
{
public:
	SerialHandler(const char* portName);
	~SerialHandler();

	int begin();
	void close();

	int read(const char* buffer, unsigned int bufferSize);
	bool write(const char* buffer, unsigned int bufferSize);

	bool isConnected();

private:
	const char* _portName;
	bool _connected;
	HANDLE _serialHandler;
	COMSTAT _status;
	DWORD _dwByte;
};

