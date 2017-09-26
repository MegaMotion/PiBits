//-----------------------------------------------------------------------------
// Copyright (c) 2015 Chris Calef
//-----------------------------------------------------------------------------

#include "motionDataSource.h"



//Swap function for undoing big endian values
//float float_swap(float value){
//   int temp = htonl(*(unsigned int*)&value);
//   return *(float*)&temp;
//};

//double double_swap(double value){
  // int temp = htonll(*(unsigned __int64*)&value);
   //return *(double*)&temp;
//};

motionDataSource::motionDataSource(bool listening)
{
  mPacketSize = 1024;
  mSocketTimeout = 0;
  mPort = 9938;
  mCurrentTick = 0;
  mLastSendTick = 0;
  mTickInterval = 1;
  sprintf(mSourceIP,"10.0.0.146");
  //sprintf(mSourceIP,"127.0.0.1");
  mListening = false;
  if (listening) {
    mServer = true;
    mListening = true;
  }
  printf("Initialized motionDataSource! IP: %s  Port: %d  listening %d \n\n",mSourceIP,mPort,mListening);
	
}

motionDataSource::~motionDataSource()
{
	
}

void motionDataSource::tick()
{
  //printf("calling motionDatSource::tick()!!!\n");
  
  if ((mCurrentTick++ % mTickInterval == 0)&&
      (mCurrentTick > mStartDelay)) 
    { 
      if (mListening)
	{
	  if (mListenSockfd == INVALID_SOCKET)
	    {
	      openListenSocket();
	    }
	  else if (mWorkSockfd == INVALID_SOCKET)
	    {
	      connectListenSocket();
	    }
	  else
	    {
	      //addBaseRequest();
	      sendPacket();
	    }
	}
    
      else
	{ //This doesn't matter on the pi, it's for the receiving side.
	  //if (mWorkSockfd == INVALID_SOCKET)
	  //  openSendSocket();
	  //else
	  //  listenForPacket();
	}
    }
}


void motionDataSource::addSensorPacket(int part_id,float w,float x,float y,float z)
{
  short opcode = 1;
  mSendControls++;//Increment this every time you add a control.
  writeShort(opcode);
  writeInt(part_id);
  writeFloat(w);
  writeFloat(x);
  writeFloat(y);
  writeFloat(z);
}

/*
void motionDataSource::openListenSocket()
{
  struct sockaddr_in source_addr,client_addr;
  int n;

  mListenSockfd = socket(AF_INET, SOCK_DGRAM,IPPROTO_UDP);
  if (mListenSockfd < 0) {
    printf("ERROR opening listen socket \n");
    return;
  } else {
    printf("SUCCESS opening listen socket \n");
  }
   
  u_long iMode=1;
  //ioctlsocket(mListenSockfd,FIONBIO,&iMode);//Make it a non-blocking socket.
  fcntl(mListenSockfd,O_NONBLOCK,&iMode);
    
  //ZeroMemory((char *) &client_addr, sizeof(client_addr));
  memset(&client_addr,0,sizeof(client_addr));
    
  client_addr.sin_family = AF_INET;
  client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  client_addr.sin_port = htons(mPort);

  if (bind(mListenSockfd, (struct sockaddr *) &client_addr, sizeof(client_addr)) < 0)
    printf("ERROR on binding mListenSockfd \n");//,WSAGetLastError());
  else
    {
      printf("SUCCESS binding motionDataSource mListenSockfd\n");
    }
}
*/
/* //But wait... if I'm just going to copy the whole function and not change anything, 
   //then there is no reason not to just use the parent class' version.

void dataSource::connectListenSocket()
{
  int n;
	
  n = listen(mListenSockfd,10);
  if (n == SOCKET_ERROR)
    {
      printf(" listen socket error!\n");
      return;
    }
	
  mWorkSockfd = accept(mListenSockfd,NULL,NULL);
	
  if (mWorkSockfd == INVALID_SOCKET)
    {
      printf(".");
      return;
    } else {
    printf("\nlisten accept succeeded!\n");
    mReturnBuffer = new char[mPacketSize];
    memset((void *)(mReturnBuffer),NULL,mPacketSize);	
    mSendBuffer = new char[mPacketSize];
    memset((void *)(mSendBuffer),NULL,mPacketSize);
    mStringBuffer = new char[mPacketSize];
    memset((void *)(mStringBuffer),NULL,mPacketSize);	
  }
}
*/




///////////////////////////////////////////////////////////////////////
// HMMM
/*

void motionDataSource::listenForPacket()
{

  struct sockaddr_in source_addr;
  int n=-1;

  //ZeroMemory((char *) &source_addr, sizeof(source_addr));
  memset(&source_addr,0,sizeof(source_addr));
  unsigned int addrSize = sizeof(source_addr);

  sensor_packet buff;
  memset(&buff, 0, sizeof(buff));

  n = recvfrom(mListenSockfd,&buff,sizeof(buff),0,(struct sockaddr*)&source_addr,&addrSize);
  //Con::printf("receiving bytes: %d",n);
  while (n>0)
  {//Do a loop here in case we have multiple packets piled up waiting.
    //Con::printf("lat %3.8f long %3.8f",buff.latitude,buff.longitude);
    if (1) 
      {
	mMotionPacket.part_id = buff.part_id;//float_swap(buff.latitude);
	mMotionPacket.w = buff.w;
	mMotionPacket.x = buff.x;
	mMotionPacket.y = buff.y;
	mMotionPacket.z = buff.z;
			
	//int latency =  Platform::getRealMilliseconds() - mLastSendTimeMS;      
	//mLastSendTimeMS = mLastSendTimeMS = Platform::getRealMilliseconds();

      }
    n = recvfrom(mListenSockfd,&buff,sizeof(buff),0,(struct sockaddr*)&source_addr,&addrSize);
  }

  return;
}

void motionDataSource::openSendSocket()
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

  mWorkSockfd = socket(AF_INET, SOCK_DGRAM,IPPROTO_UDP);
  if (mWorkSockfd < 0)
  {
    printf("ERROR creating UDP send socket\n");
    return;
  }
  else
  {
    printf("SUCCESS creating UDP send socket.");
  }
  
  //assign local values
  source_addr.sin_addr.s_addr = inet_addr("172.21.8.178");
  source_addr.sin_family = AF_INET;
  source_addr.sin_port = htons( mPort );

  //binds connection
  if (bind(mWorkSockfd, (struct sockaddr *)&source_addr, sizeof(source_addr)) < 0) {    
    perror("Connection error");
    return;
  }
  
}

void motionDataSource::sendPacket()
{

  
  printf("Trying to send UDP packet... but not really.\n");

  
}
*/
