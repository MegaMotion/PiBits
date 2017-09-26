//-----------------------------------------------------------------------------
// Copyright (c) 2015 Chris Calef
//-----------------------------------------------------------------------------

#include "dataSource.h"

//#include "console/consoleTypes.h"//Torque specific, should do #ifdef TORQUE or something

//////////////////////////////////////////////////////////////////////////
//// Testing  ////
/*
Serial::Serial(const char* portName)
{
	//We're not yet connected
	this->connected = false;

	//Try to connect to the given port throuh CreateFile

	wchar_t wPortName[4096] = { 0 };
	MultiByteToWideChar(CP_ACP, 0, portName, -1, wPortName, 4096);

	this->hSerial = CreateFile(wPortName,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	//Check if the connection was successfull
	if (this->hSerial == INVALID_HANDLE_VALUE)
	{
		//If not success full display an Error
		if (GetLastError() == ERROR_FILE_NOT_FOUND) {

			//Print Error if neccessary
			printf("ERROR: Handle was not attached. Reason: %s not available.\n", portName);

		}
		else
		{
			printf("ERROR!!!");
		}
	}
	else
	{
		//If connected we try to set the comm parameters
		DCB dcbSerialParams = { 0 };

		//Try to get the current
		if (!GetCommState(this->hSerial, &dcbSerialParams))
		{
			//If impossible, show an error
			printf("failed to get current serial parameters!");
		}
		else
		{
			//Define serial connection parameters for the arduino board
			dcbSerialParams.BaudRate = CBR_115200;
			dcbSerialParams.ByteSize = 8;
			dcbSerialParams.StopBits = ONESTOPBIT;
			dcbSerialParams.Parity = NOPARITY;
			//Setting the DTR to Control_Enable ensures that the Arduino is properly
			//reset upon establishing a connection
			dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;

			//Set the parameters and check for their proper application
			if (!SetCommState(hSerial, &dcbSerialParams))
			{
				printf("ALERT: Could not set Serial Port parameters");
			}
			else
			{
				//If everything went fine we're connected
				this->connected = true;
				//Flush any remaining characters in the buffers 
				PurgeComm(this->hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);
				//We wait 2s as the arduino board will be reseting
				Sleep(ARDUINO_WAIT_TIME);
			}
		}
	}

}

Serial::~Serial()
{
	//Check if we are connected before trying to disconnect
	if (this->connected)
	{
		//We're no longer connected
		this->connected = false;
		//Close the serial handler
		CloseHandle(this->hSerial);
	}
}

int Serial::ReadData(char *buffer, unsigned int nbChar)
{
	//Number of bytes we'll have read
	DWORD bytesRead;
	//Number of bytes we'll really ask to read
	unsigned int toRead;

	//Use the ClearCommError function to get status info on the Serial port
	ClearCommError(this->hSerial, &this->errors, &this->status);

	//Check if there is something to read
	if (this->status.cbInQue>0)
	{
		//If there is we check if there is enough data to read the required number
		//of characters, if not we'll read only the available characters to prevent
		//locking of the application.
		if (this->status.cbInQue>nbChar)
		{
			toRead = nbChar;
		}
		else
		{
			toRead = this->status.cbInQue;
		}

		//Try to read the require number of chars, and return the number of read bytes on success
		if (ReadFile(this->hSerial, buffer, toRead, &bytesRead, NULL))
		{
			return bytesRead;
		}

	}

	//If nothing has been read, or that an error was detected return 0
	return 0;
}


bool Serial::WriteData(const char *buffer, unsigned int nbChar)
{
	DWORD bytesSend;

	//Try to write the buffer on the Serial port
	if (!WriteFile(this->hSerial, (void *)buffer, nbChar, &bytesSend, 0))
	{
		//In case it don't work get comm error and return false
		ClearCommError(this->hSerial, &this->errors, &this->status);

		return false;
	}
	else
		return true;
}

bool Serial::IsConnected()
{
	//Simply return the connection status
	return this->connected;
}
*/

//// Testing  ////
//////////////////////////////////////////////////////////////////////////






dataSource::dataSource(bool server)
{
  mPacketSize = 1024;
  mSocketTimeout = 0;
  mPort = 9938;
  mCurrentTick = 0;
  mLastSendTick = 0;
  mLastSendTimeMS = 0;
  mTickInterval = 1;//45;
  mTalkInterval = 20;
  mStartDelay = 50;
  mPacketCount = 0;
  mMaxPackets = 20;
  mSendControls = 0;
  mReturnByteCounter = 0;
  mSendByteCounter = 0;
  sprintf(mSourceIP,"10.0.0.146");
  mListenSockfd = INVALID_SOCKET;
  mWorkSockfd = INVALID_SOCKET;
  mReturnBuffer = NULL;
  mSendBuffer = NULL;
  mStringBuffer = NULL;
  mReadyForRequests = false;
  mServer = false;
  mListening = false;	
  mAlternating = true;
  mConnectionEstablished = false;
  if (server)
    {
      mServer = true;
      mListening = true;
    }
  printf("New data source object!!! Source IP: %s listening %d\n",mSourceIP,server);
}

dataSource::~dataSource()
{
  disconnectSockets();
}

////////////////////////////////////////////////////////////////////////////////

void dataSource::tick()
{	
  printf("datasource tick %d",mCurrentTick);
  if ((mCurrentTick++ % mTickInterval == 0)&&
      (mCurrentTick > mStartDelay)) 
    { 
      if (mConnectionEstablished == false)
	{
	  trySockets();
	} else {
	if (mListening) {
	  listenForPacket();
	  if (mAlternating) {
	    mListening = false;
	    addBaseRequest();
	    if (mServer) tick();
	  }
	} else {				
	  sendPacket();
	  if (mAlternating) {
	    mListening = true;
	    if (!mServer) tick();
	  } else 
	    addBaseRequest();
	}
      }
    }
}

/////////////////////////////////////////////////////////////////////////////////

void dataSource::openListenSocket()
{
  struct sockaddr_in source_addr;
  //int n;

  printf("connecting listen socket\n");
  mListenSockfd = socket(AF_INET, SOCK_STREAM,IPPROTO_TCP);
  if (mListenSockfd < 0) {
    printf("ERROR opening listen socket \n");
    return;
  } else {
    printf("SUCCESS opening listen socket \n");
  }
	
  int optVal = 1;
  char optVal2[12];
  
  //// lose the pesky "Address already in use" error message  - only for listen socket?
  if (setsockopt(mListenSockfd,SOL_SOCKET,SO_REUSEADDR,&optVal,sizeof(optVal)) == -1) {
    printf("FAILED to set socket option SO_REUSADDR\n");
    //return;
  } else printf("SUCCEEDED at setting socket option SO_REUSADDR\n");
  
  ////sprintf(optVal2,"wlan0");
  ////optVal2 = "wlan0";
  //strcpy(optVal2,"wlan0");
  //if (setsockopt(mListenSockfd,SOL_SOCKET,SO_BINDTODEVICE,optVal2,strlen(optVal2)) == -1) {
  //  printf("FAILED to set socket option SO_BINDTODEVICE\n");
    //return;
  //} else printf("SUCCEEDED at setting socket option SO_BINDTODEVICE\n");
		
  u_long iMode=1;
  //ioctlsocket(mListenSockfd,FIONBIO,&iMode);//Make it a non-blocking socket.//Pi - FIONBIO not defined
  fcntl(mListenSockfd,O_NONBLOCK,&iMode);

  //ZeroMemory((char *) &source_addr, sizeof(source_addr));
  memset(&source_addr,0,sizeof(source_addr));
  source_addr.sin_family = AF_INET;
  //source_addr.sin_addr.s_addr = inet_addr( mSourceIP );
  source_addr.sin_addr.s_addr = INADDR_ANY;
  source_addr.sin_port = htons(mPort);

  if (bind(mListenSockfd, (struct sockaddr *) &source_addr,
	   sizeof(source_addr)) < 0) 
    printf("ERROR on binding mListenSockfd\n");
  else printf("SUCCESS binding mListenSockfd\n");
 
}

void dataSource::connectListenSocket()
{
  int n;
	
  n = listen(mListenSockfd,10);
  if (n == SOCKET_ERROR)
  {
    //printf(" listen socket error!\n");
    return;
  }
	
  mWorkSockfd = accept(mListenSockfd,NULL,NULL);
	
  if (mWorkSockfd == INVALID_SOCKET)
  {
    printf(".");
    return;
  } else {
    
    printf("\nSUCCESS dataSource::connectListen mWorkSockfd!\n");
    
    mReturnBuffer = new char[mPacketSize];
    memset((void *)(mReturnBuffer),NULL,mPacketSize);	
    mSendBuffer = new char[mPacketSize];
    memset((void *)(mSendBuffer),NULL,mPacketSize);
    mStringBuffer = new char[mPacketSize];
    memset((void *)(mStringBuffer),NULL,mPacketSize);	
  }
}

void dataSource::listenForPacket()
{
  printf("dataSource listenForPacket");
  mPacketCount = 0;

  int n = recv(mWorkSockfd,mReturnBuffer,mPacketSize,0);
  if (n<=0) {
    printf(".");
    return;
  }

  readPacket();
}

//for (int j=0;j<mPacketCount;j++)
//{
//	n = recv(mWorkSockfd,mReturnBuffer,mPacketSize,0);
//	if (n<=0) j--;
//	else {
//		readPacket();
//	}
//}


void dataSource::readPacket()
{
  short opcode,controlCount;//,packetCount;

  controlCount = readShort();
  for (short i=0;i<controlCount;i++)
    {		
      opcode = readShort();
      if (opcode==1) {   ////  keep contact, but no request /////////////////////////
	int tick = readInt();				
	//if (mServer) Con::printf("dataSource clientTick = %d, my tick %d",tick,mCurrentTick);
	//else Con::printf("dataSource serverTick = %d, my tick %d",tick,mCurrentTick);
      }// else if (opcode==22) { // send us some number of packets after this one
      //	packetCount = readShort();
      //	if ((packetCount>0)&&(packetCount<=mMaxPackets))
      //		mPacketCount = packetCount;
      //}
    }
	
  clearReturnPacket();
}

/////////////////////////////////////////////////////////////////////////////////

void dataSource::connectSendSocket()
{
  struct sockaddr_in source_addr;
	
  mReturnBuffer = new char[mPacketSize];	
  mSendBuffer = new char[mPacketSize];		
  mStringBuffer = new char[mPacketSize];
  memset((void *)(mReturnBuffer),NULL,mPacketSize);
  memset((void *)(mSendBuffer),NULL,mPacketSize);
  memset((void *)(mStringBuffer),NULL,mPacketSize);		

  mReadyForRequests = true;

  addBaseRequest();

  mWorkSockfd = socket(AF_INET, SOCK_STREAM,IPPROTO_TCP);
  if (mWorkSockfd < 0) {
    printf("ERROR opening send socket\n");
    return;
  }
	
  u_long iMode=1;
  //ioctlsocket(mWorkSockfd,FIONBIO,&iMode);//Make it a non-blocking socket. // (windows version)
  fcntl(mWorkSockfd,O_NONBLOCK,&iMode);// (linux, or both, version)

  //ZeroMemory((char *) &source_addr, sizeof(source_addr));
  memset(&source_addr,0,sizeof(source_addr));
  source_addr.sin_family = AF_INET;
  source_addr.sin_addr.s_addr = inet_addr( mSourceIP );
  source_addr.sin_port = htons(mPort)
    ;

  int connectCode = connect(mWorkSockfd,(struct sockaddr *) &source_addr,sizeof(source_addr));
  if (connectCode < 0) 
  {
      printf("dataSource: ERROR connecting send socket: %d\n",connectCode);
      return;
  } else {		
    printf("dataSource: SUCCESS connecting send socket: %d\n",connectCode);
  }
}

void dataSource::sendPacket()
{ 
  memset((void *)(mStringBuffer),NULL,mPacketSize);	
  memcpy((void*)mStringBuffer,reinterpret_cast<void*>(&mSendControls),sizeof(short));
  memcpy((void*)&mStringBuffer[sizeof(short)],(void*)mSendBuffer,mPacketSize-sizeof(short));
  send(mWorkSockfd,mStringBuffer,mPacketSize,0);	
  mLastSendTick = mCurrentTick;

  clearSendPacket();
}

void dataSource::clearSendPacket()
{	
  memset((void *)(mSendBuffer),NULL,mPacketSize);
  memset((void *)(mStringBuffer),NULL,mPacketSize);	

  mSendControls = 0;
  mSendByteCounter = 0;
}

void dataSource::clearReturnPacket()
{
  memset((void *)(mReturnBuffer),NULL,mPacketSize);	
  memset((void *)(mStringBuffer),NULL,mPacketSize);	

  mReturnByteCounter = 0;	
}

/////////////////////////////////////////////////////////////////////////////////

void dataSource::trySockets()
{
  if (mServer)
    {
      if (mListenSockfd == INVALID_SOCKET) {
	openListenSocket();
      } else if (mWorkSockfd == INVALID_SOCKET) {
	connectListenSocket();
      } else {
	listenForPacket();
	mConnectionEstablished = true;
      }
    } else {
    if (mWorkSockfd == INVALID_SOCKET) {
      connectSendSocket();
    } else {
      sendPacket();
      mConnectionEstablished = true;
    }
  }
}

void dataSource::disconnectSockets()
{
  //closesocket(mListenSockfd);//windows
  //closesocket(mWorkSockfd);
  close(mListenSockfd);//linux
  close(mWorkSockfd);
}

/////////////////////////////////////////////////////////////////////////////////

void dataSource::writeShort(short value)
{
  memcpy((void*)&mSendBuffer[mSendByteCounter],reinterpret_cast<void*>(&value),sizeof(short));
  mSendByteCounter += sizeof(short);	
}

void dataSource::writeInt(int value)
{
  memcpy((void*)&mSendBuffer[mSendByteCounter],reinterpret_cast<void*>(&value),sizeof(int));
  mSendByteCounter += sizeof(int);	
}

void dataSource::writeFloat(float value)
{
  memcpy((void*)&mSendBuffer[mSendByteCounter],reinterpret_cast<void*>(&value),sizeof(float));
  mSendByteCounter += sizeof(float);	
}

void dataSource::writeDouble(double value)
{
  memcpy((void*)&mSendBuffer[mSendByteCounter],reinterpret_cast<void*>(&value),sizeof(double));
  mSendByteCounter += sizeof(double);	
}

void dataSource::writeString(char *content)
{
  int length = strlen(content);
  writeInt(length);
  strncpy(&mSendBuffer[mSendByteCounter],content,length);
  mSendByteCounter += length;
}

//void dataSource::writePointer(void *pointer) //Maybe, someday? using boost, shared pointer or shared memory? 
//{  //Or can it be done in a more brute force way with global scale pointers? 
//}

//////////////////////////////////////////////

short dataSource::readShort()
{
  char bytes[sizeof(short)];
  for (int i=0;i<sizeof(short);i++) bytes[i] = mReturnBuffer[mReturnByteCounter+i];
  mReturnByteCounter+=sizeof(short);
  short *ptr = reinterpret_cast<short*>(bytes);
  return *ptr;	
}

int dataSource::readInt()
{
  char bytes[sizeof(int)];
  for (int i=0;i<sizeof(int);i++) bytes[i] = mReturnBuffer[mReturnByteCounter+i];
  mReturnByteCounter+=sizeof(int);
  int *ptr = reinterpret_cast<int*>(bytes);
  return *ptr;
}

float dataSource::readFloat()
{
  char bytes[sizeof(float)];
  for (int i=0;i<sizeof(float);i++) bytes[i] = mReturnBuffer[mReturnByteCounter+i];
  mReturnByteCounter+=sizeof(float);
  float *ptr = reinterpret_cast<float*>(bytes);
  return *ptr;
}

double dataSource::readDouble()
{
  char bytes[sizeof(double)];
  for (int i=0;i<sizeof(double);i++) bytes[i] = mReturnBuffer[mReturnByteCounter+i];
  mReturnByteCounter+=sizeof(double);
  double *ptr = reinterpret_cast<double*>(bytes);
  return *ptr;
}

char *dataSource::readString()
{
  int length = readInt();
  strncpy(mStringBuffer,&mReturnBuffer[mReturnByteCounter],length);
  mReturnByteCounter += length;
  return mStringBuffer;
}

void dataSource::clearString()
{
  if (mStringBuffer!=NULL)	
    memset((void *)(mStringBuffer),NULL,mPacketSize);
}

//void dataSource::readPointer()
//{
//}

/////////////////////////////////////////////////////////////////////////////////

void dataSource::addBaseRequest()
{
  short opcode = 1;//base request
  mSendControls++;//Increment this every time you add a control.
  writeShort(opcode);
  writeInt(mCurrentTick);//For a baseRequest, do nothing but send a tick value to make sure there's a connection.
}

void dataSource::handleBaseRequest()
{
  int tick = readInt();				
  //if (mServer) Con::printf("dataSource clientTick = %d, my tick %d",tick,mCurrentTick);
  //else Con::printf("dataSource serverTick = %d, my tick %d",tick,mCurrentTick);
}
