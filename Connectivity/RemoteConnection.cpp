#include "Stdafx.h"
#include "ConnectionManager.h"

Connectivity::RemoteConnection::RemoteConnection()
{
	ConnectionManager::InitWinsock();
	this->connection = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (this->connection == INVALID_SOCKET)
		throw gcnew Exception("Unable to create socket");
}

void Connectivity::RemoteConnection::Connect(const long ip, const int port)
{
	sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = ip;
	address.sin_port = htons(port);

	if (connect(this->connection, (SOCKADDR*)&address, sizeof(address)) == SOCKET_ERROR)
		throw gcnew Exception("Unable to connect to the specified host: " + WSAGetLastError());
}

void Connectivity::RemoteConnection::Close()
{
	shutdown(this->connection, SD_BOTH);
	closesocket(this->connection);
}

bool Connectivity::RemoteConnection::Send(array<unsigned char> ^ buffer, const int offset, const int size)
{
	pin_ptr<unsigned char> array_pin = &buffer[0];
	const char * data = (char*)array_pin + offset;

	return (send(this->connection, data, size, 0) != SOCKET_ERROR);
}

int Connectivity::RemoteConnection::Receive(array<unsigned char> ^ buffer, const int offset, const int size)
{
	pin_ptr<unsigned char> array_pin = &buffer[0];
	char * data = (char*)array_pin + offset;

	int state = recv(this->connection, data, size, 0);

	if (state > 0)			//Data sent
		return state;	
	else if (state == 0)	//Connection closed
		throw gcnew Exception("Conenction closed");
	else
		throw gcnew Exception("Receive method failed: " + WSAGetLastError());
}