#include "stdafx.h"
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
#include <stdio.h>
#include <iostream>

#include <windows.h>    // for Win32 APIs and types
#include <strsafe.h>    // for safe versions of string functions

#include <iostream>

#include "IDataParser.h"

using namespace System;
using namespace System::Collections;
using namespace System::Collections::Generic;
using namespace System::Runtime::InteropServices;
using namespace System::Threading;

using namespace std;

const int WSOCK_VER = 0x0202;		//Our winsock supported version
const int PROTOCOL_TCP = 0;			//TCP
const int BACKLOG = 500;			//Backlog queue length

const int BUFFER_SIZE = 2048;		//Buffer size for receiving data
const int ADDRESS_SIZE = 16;		//Size of the address struct

//void __stdcall EmptyRoutine(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);


namespace Connectivity  
{
	  ref class ConnectionInformation;
	  public delegate void ConnectionEvent(ConnectionInformation ^);

	  public ref class ConnectionManager  
	  {
  
	  private:
			unsigned int acceptedConnections;				//Amount of accepted connections (not in use)
			unsigned int maxConnections;					//Max connections we can accept
			bool isListening;								//State for the socket if it is accepting connections

			SOCKET listener;								//Native listening socket
			
			WaitOrTimerCallback ^ callback;					//Callback when a connection is accepted
			IDataParser ^ parser;							//Packet parser
			array<ConnectionInformation ^> ^ connections;	//Holds all free and used connection classes
			ConnectionInformation ^	invalidConnection;		//Invalid socket
			ConnectionInformation ^ getFreeConnection();	//Returns a free connection is available, if not returns invalidConnection

			void continueListen();							//Initializes a wait handle and waits for new connections
			void connectionReceived(Object ^, bool);		//Callback when theres connections to be accepted
			
			WaitHandle ^ waitHandle;
			IntPtr waitHandlePointer;


			static bool WinsockInitialized;					//Indicates whether the winsock has been initialized

	  public:
			ConnectionManager(const int portID, const int maxConnections, IDataParser ^ parser, bool disableNaglesAlgorithm);
			//~ConnectionManager();

			void Destroy();									//Stops accepting connections and releases all resources
			bool IsConnected();								//Returns if the socket is listening
			void StartListening();							//Calls continueListen to wait for connections
			bool WriteDebugLines;							//If true, data will be written to console about states etc
			
			static void InitWinsock();						//Makes sure winsock is initialized
			event ConnectionEvent ^ connectionEvent;		//Event gets called when a connection is accepted
	  };

	  public ref class ConnectionInformation  {
	  private:

			int						connectionID;			//The connections ID
			bool					isClosed;				//Specifies if close
			bool					sendError;				//Indicates if an send operation was failed
			unsigned char *			receiveBuffer;			//The receive buffer

			SOCKET					connection;				//Socket for the connection
			sockaddr_in	*			address;				//The address of the remote end point
			
			//Members for data receiving:
			WSABUF *				bufferInfo;				//Holds data describing the receive buffer
			LPWSAOVERLAPPED			receiveOverlapped;		//Holds pointer to callback event (receive)
			LPDWORD					flags;					//Flags for receiving
			LPDWORD					bytesReceived;			//Amount of bytes we receive per call
			array<unsigned char> ^	managedBuffer;			//Managed buffer (same size as receive buffer)
			WaitOrTimerCallback ^	receivedCallback;		//Delegate which gets called when data is received
			WaitHandle ^			receiveHandle;			//Wait handle for the threading
			IntPtr					handlePointer;			//Pointer to the receiveHandle
			
			//Members for data sending
			LPWSAOVERLAPPED			sendOverlapped;			//Overlapped information
			LPDWORD					bytesSent;				//Amount of bytes we sent
			Object ^				sendSyncRoot;			//Lock object for solving concurrency issues
			LPWSABUF				sendInfo;				//Struct describing the data we're sending

			ConnectionManager ^ manager;					//The connection manager

			void continueReceive();							//Continues receiving data
			void close();									//Closes the connection
			void receivedData(Object ^, bool);				//Gets called when data is received

	  public:
			ConnectionInformation(unsigned int connectionID, ConnectionManager ^ manager, 
				IDataParser ^ parser, unsigned char * buffer, array<unsigned char> ^ managedBuffer);
			ConnectionInformation();
			~ConnectionInformation();

			bool isFree;									//Says if the connection is in use or not
			bool isValid;									//If the connection is a valid connection
			unsigned int  GetConnectionID();				//Returns the ID of the connection

			
			event ConnectionEvent ^ OnConnectionClosed;

			bool AcceptConnection(SOCKET, IDataParser ^);	//Accepts a connection, returns true if there was a connection to accept
			String ^ GetIP();								//Returns the connections IP address
			IDataParser ^ Parser;							//Packet processor

			void StartPacketProcessing();					//Starts receiving data
			void SendData(unsigned char [], const int);		//Sends data async to the connection
			void Destroy();									//Destroys the connection
	  };

	  public ref class RemoteConnection
	  {
	  private:
		  SOCKET connection;
		  bool isClosed;

	  public:
		  RemoteConnection();
		  void Connect(const long ip, const int port);
		  void Close();
		  bool Send(array<unsigned char> ^ buffer, const int offset, const int size);
		  int Receive(array<unsigned char> ^ buffer, const int offset, const int size);
	  };
}
