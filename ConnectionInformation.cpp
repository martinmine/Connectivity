#include "ConnectionManager.h"
#include "Stdafx.h"

void __stdcall EmptyRoutine(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	Console::WriteLine("routine");
}

Connectivity::ConnectionInformation::ConnectionInformation(unsigned int connectionID, ConnectionManager ^ manager, 
														   IDataParser ^ parser, unsigned char * buffer, 
														   array<unsigned char> ^ managedBuffer)
	: connectionID(connectionID),
	manager(manager),
	Parser(parser),
	receiveBuffer(buffer),
	managedBuffer(managedBuffer),

	bufferInfo(new WSABUF()),
	flags(new DWORD(0)),
	bytesReceived(new DWORD(0)),
	bytesSent(new DWORD(0)),
	receiveOverlapped(new _OVERLAPPED()),
	sendOverlapped(new _OVERLAPPED()),
	sendInfo(new WSABUF()),
	address(new sockaddr_in()),

	isValid(true),
	isClosed(true),
	isFree(true)
{
	this->sendSyncRoot		= gcnew Object();
	this->receivedCallback	= gcnew WaitOrTimerCallback(this, &Connectivity::ConnectionInformation::receivedData);
	this->receiveHandle		= gcnew ManualResetEvent(false);
 	this->handlePointer		= receiveHandle->SafeWaitHandle->DangerousGetHandle();

	this->bufferInfo->buf = (char*)receiveBuffer;
	this->bufferInfo->len = BUFFER_SIZE;

	this->receiveOverlapped->hEvent = (void*)handlePointer;
}

Connectivity::ConnectionInformation::ConnectionInformation()
	: address(new sockaddr_in()),
	  isValid(false)
{ }

Connectivity::ConnectionInformation::~ConnectionInformation()
{
	delete bufferInfo;
	delete flags;
	delete bytesReceived;
	delete bytesSent;
	delete receiveOverlapped;
	delete sendOverlapped;
	delete sendInfo;
	delete address;
}

void Connectivity::ConnectionInformation::StartPacketProcessing()  
{
	continueReceive();
}

void Connectivity::ConnectionInformation::continueReceive()
{
	if (isClosed || sendError)
	{
		close();
	}
	else
	{
		bufferInfo->buf = (char*)receiveBuffer;
		bufferInfo->len = BUFFER_SIZE;
	
		if (WSARecv(connection, bufferInfo, 1, bytesReceived, flags, receiveOverlapped, NULL) == 0) // 0 means send completed instantly
		{
			receivedData(nullptr, false);
		}
		else
		{
			if (WSAGetLastError() == WSA_IO_PENDING) //Pending IO
				ThreadPool::UnsafeRegisterWaitForSingleObject(receiveHandle, receivedCallback, nullptr, -1, true);
			else
			{
				if (this->manager->WriteDebugLines)
					Console::WriteLine("Connection {0} closed because__ " + WSAGetLastError(), connectionID);
				close();
			}
		}
	}
}

void Connectivity::ConnectionInformation::SendData(unsigned char data[], const int length)
{
	if (!isClosed && !sendError)
	{
		Monitor::Enter(this->sendSyncRoot);

		try
		{
			sendInfo->buf = (char*)data;
			sendInfo->len = length;
		
			if (WSASend(connection, sendInfo, 1, bytesSent, 0, sendOverlapped, NULL) == SOCKET_ERROR)
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					if (this->manager->WriteDebugLines)
						Console::WriteLine("Connection {0} closed because_ " + WSAGetLastError(), connectionID);
					this->sendError = true;
					//The send error bool makes sure that the close function doesn't get called
					//during packet processing which could lead to a lot of null reffernce exceptions.
				}
			}
		}
		finally
		{
			Monitor::Exit(this->sendSyncRoot);
		}
	}
}	

void Connectivity::ConnectionInformation::receivedData(Object ^ state, bool timeout)
{
	if (!isClosed)
	{
		BOOL operationSucceeded = WSAGetOverlappedResult(connection, receiveOverlapped, bytesReceived, FALSE, flags);
		if (!operationSucceeded || *bytesReceived < 1 || sendError)
		{
			if (WSAGetLastError() != WSA_IO_INCOMPLETE)
			{
				if (this->manager->WriteDebugLines)
					Console::WriteLine("Connection {0} closed because " + WSAGetLastError(), connectionID);
				close();
			}
		}
		else
		{
			Marshal::Copy((IntPtr)receiveBuffer, managedBuffer, 0, (int)*bytesReceived);

			try
			{
				if (Parser != nullptr)
					Parser->handlePacketData(managedBuffer, (int)*bytesReceived);
			}
			catch (Exception ^ e)
			{
				if (this->manager->WriteDebugLines)
					Console::WriteLine("Warning: " + e->ToString());
				Destroy();
			}

			continueReceive();
		}
	}
}


void Connectivity::ConnectionInformation::close()
{
	if (!isClosed)
	{

		if (isValid && this->manager->WriteDebugLines && connection != INVALID_SOCKET)
			Console::WriteLine("Connection {0} closed", connectionID);
		this->isClosed = true;

		shutdown(connection, SD_BOTH);
		closesocket(this->connection);
		delete this->Parser;
		this->Parser = nullptr;

		this->OnConnectionClosed(this);
		this->isFree = true;
	}
}

bool Connectivity::ConnectionInformation::AcceptConnection(SOCKET listener, IDataParser ^ parser)
{
	if (!this->isValid)
		return true;

	int size = sizeof(*address);
	this->connection = accept(listener, (SOCKADDR*)address, &size);

	if (connection == INVALID_SOCKET)
		return false;

	if (this->manager->WriteDebugLines)
		Console::WriteLine("Connection {0} {1}", connectionID, isValid ? "accepted" : "denied");

	this->isFree = false;
	this->sendError = false;
	this->isClosed = false;
	this->Parser = parser;

	return true;
}

void Connectivity::ConnectionInformation::Destroy()
{
	if (!isClosed)
	{
		if (isValid && this->manager->WriteDebugLines)
			Console::WriteLine("Connection {0} closed because external dc call", connectionID);
		close();
	}
}

String ^ Connectivity::ConnectionInformation::GetIP()  
{
	if (isClosed)
		return String::Empty;

	return String::Format("{0}.{1}.{2}.{3}", 
		address->sin_addr.S_un.S_un_b.s_b1, 
		address->sin_addr.S_un.S_un_b.s_b2, 
		address->sin_addr.S_un.S_un_b.s_b3, 
		address->sin_addr.S_un.S_un_b.s_b4);
}

unsigned int Connectivity::ConnectionInformation::GetConnectionID()  
{
	return this->connectionID;
}

