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
	const char* m_portName = "";
	bool m_connected = false;
	HANDLE m_serialHandler;
	COMSTAT m_status;
	DWORD m_dwByte;
};

