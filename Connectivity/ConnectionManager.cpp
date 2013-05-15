using namespace System::Net;

#include "Stdafx.h"
#include "ConnectionManager.h"

void Connectivity::ConnectionManager::InitWinsock()
{
	if (!WinsockInitialized)
	{
		WinsockInitialized = true;
		WSADATA socketVersionData;
		int errorCode = WSAStartup(WSOCK_VER, &socketVersionData);

		if (errorCode != 0)
		{
			WSASetLastError(errorCode);
			throw gcnew Exception("Invalid socket version installed");
		}
	}
}

Connectivity::ConnectionManager::ConnectionManager(const int port, const int maxConnections, IDataParser ^ parser, bool disableNaglesAlgorithm)
	: parser(parser),
	  maxConnections(maxConnections),
	  isListening(false),
	  acceptedConnections(0)
{
  	this->callback				= gcnew WaitOrTimerCallback(this, &Connectivity::ConnectionManager::connectionReceived);
	this->connections			= gcnew array<ConnectionInformation ^>(maxConnections);
	this->invalidConnection		= gcnew ConnectionInformation();

	InitWinsock();

	int errorCode;
	sockaddr_in acceptAddress;

	this->listener					= WSASocket(AF_INET, SOCK_STREAM, PROTOCOL_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	acceptAddress.sin_family		= AF_INET;				//Create the accept address
	acceptAddress.sin_addr.s_addr	= htonl(INADDR_ANY);
	acceptAddress.sin_port			= htons(port);

	errorCode = bind(this->listener, (SOCKADDR*)&acceptAddress, sizeof(acceptAddress));
	if (errorCode != 0)
	{
		WSASetLastError(errorCode);
		throw gcnew Exception("Unable to bind socket. Make sure there is no program listening on port " + port);
	}
}

void Connectivity::ConnectionManager::StartListening()  
{
	if (isListening)
		throw gcnew Exception("Socket is already listening");

	this->isListening = true;
	
	listen(this->listener, BACKLOG);
	continueListen();
}

void Connectivity::ConnectionManager::continueListen()
{
	if (!isListening)
		return;

	this->waitHandle			= gcnew ManualResetEvent(false);
	this->waitHandlePointer		= waitHandle->SafeWaitHandle->DangerousGetHandle();
	//To-do: remove this (code above)

	WSAEventSelect(listener, (void*)waitHandlePointer, FD_ACCEPT);
	ThreadPool::UnsafeRegisterWaitForSingleObject(waitHandle, callback, nullptr, -1, true);
}

Connectivity::ConnectionInformation ^ Connectivity::ConnectionManager::getFreeConnection()
{
	for (unsigned int i = 0; i < maxConnections; i++)
	{
		if (connections[i] == nullptr)
			connections[i] = gcnew ConnectionInformation(i + 1, this, this->parser, new unsigned char [BUFFER_SIZE], gcnew array<unsigned char>(BUFFER_SIZE));

		if (connections[i]->isFree)
			return connections[i];
	}

	return invalidConnection;
}

void Connectivity::ConnectionManager::connectionReceived(Object ^ obj, bool timeOut)
{
	ConnectionInformation ^ connection;
	bool wouldntBlock;

	do
	{
		connection = getFreeConnection();
		wouldntBlock = connection->AcceptConnection(listener, (IDataParser^)parser->Clone());

		if (wouldntBlock)
		{
			if (connection->isValid)
				connectionEvent(connection);
			else
				connection->Destroy();
		}
	}
	while (wouldntBlock && isListening);

	continueListen();
}

void Connectivity::ConnectionManager::Destroy()  
{
	closesocket(listener);
	this->isListening = false;
}

bool Connectivity::ConnectionManager::IsConnected()
{
	return this->isListening;
}