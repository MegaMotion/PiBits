//-----------------------------------------------------------------------------
// Copyright (c) 2015 Chris Calef
//-----------------------------------------------------------------------------

#ifndef _MOTIONDATASOURCE_H_
#define _MOTIONDATASOURCE_H_

#include <vector>

#include "./dataSource.h"

///////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    int part_id;
    float w;
    float x;
    float y;	
    float z;
} sensor_packet;

///////////////////////////////////////////////////////////////////////////////////

class motionDataSource : public dataSource 
{
public:

	sensor_packet mMotionPacket;
	//std::vector mBodyparts<mMotionPacket>

	motionDataSource(bool listening);
	~motionDataSource();

	//void openListenSocket();
	//void listenForPacket();
	
	//void openSendSocket();
	//void sendPacket(); 
	void addSensorPacket(int,float,float,float,float);
	
	void tick();
};

#endif // _MOTIONDATASOURCE_H_
