//-----------------------------------------------------------------------------
// Copyright (c) 2015 Chris Calef
//-----------------------------------------------------------------------------

#ifndef _DATASOURCE_H_
#define _DATASOURCE_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <iostream>
//#include "winsock2.h" //Need an ifdef windows here...
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>


#include "../I2Cdev.h"
#define OPCODE_BASE

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

//Temp, trouble compiling this class in its own file due to windows.h needs, see if it works here. //
/////////////////////////////////////////////
//#define ARDUINO_WAIT_TIME 2000
/*
class Serial
{
private:
	//Serial comm handler
	HANDLE hSerial;
	//Connection status
	bool connected;
	//Get various information about the connection
	COMSTAT status;
	//Keep track of last error
	DWORD errors;

public:
	//Initialize Serial communication with the given COM port
	Serial(const char* portName);
	//Close the connection
	~Serial();
	//Read data in a buffer, if nbChar is greater than the
	//maximum number of bytes available, it will return only the
	//bytes available. The function return -1 when nothing could
	//be read, the number of bytes actually read.
	int ReadData(char *buffer, unsigned int nbChar);
	//Writes data from a buffer through the Serial connection
	//return true on success.
	bool WriteData(const char *buffer, unsigned int nbChar);
	//Check if we are actually connected
	bool IsConnected();
	};*/
/////////////////////////////////////////////

/// Base class for various kinds of data sources, first one being worldDataSource, for terrain, sky, weather and map information.
class dataSource 
{
   public:
	   char mSourceIP[16];
	   unsigned int mPort;

	   unsigned int mCurrentTick;
	   unsigned int mLastSendTick;//Last time we sent a packet.
	   unsigned int mLastSendTimeMS;//Last time we sent a packet.
	   unsigned int mTickInterval;
	   unsigned int mTalkInterval;
	   unsigned int mStartDelay;
	   unsigned int mPacketCount;
	   unsigned int mMaxPackets;
	   unsigned int mPacketSize;

	   bool mReadyForRequests;//flag to user class (eg terrainPager) that we can start adding requests.

	   int mListenSockfd;
	   int mWorkSockfd;

	   //fd_set mMasterFDS;
	   //fd_set mReadFDS;

	   int mSocketTimeout;
	   
	   bool mServer;
	   bool mListening;
	   bool mAlternating;
	   bool mConnectionEstablished;

	   char *mReturnBuffer;
	   char *mSendBuffer;
	   char *mStringBuffer;

	   short mSendControls;
	   short mReturnByteCounter;
	   short mSendByteCounter;

	   dataSource(bool listening=false);
	   ~dataSource();

	   void tick();
	   
	   void openListenSocket();
	   void connectListenSocket();
	   void listenForPacket();
	   void readPacket();
	   void clearReturnPacket();

	   void connectSendSocket();
	   void sendPacket();
	   void clearSendPacket();
	   
	   void trySockets();
	   void disconnectSockets();	  

	   void writeShort(short);
	   void writeInt(int);
	   void writeFloat(float);
	   void writeDouble(double);
	   void writeString(char *);
	   //void writePointer(void *);//Someday? Using boost?

	   short readShort();
	   int readInt();
	   float readFloat();
	   double readDouble();
	   char *readString();
	   //void *readPointer();
	   
	   void clearString();

	   void addBaseRequest();
	   void handleBaseRequest();
};

#endif // _DATASOURCE_H_
