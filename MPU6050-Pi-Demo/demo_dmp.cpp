#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <chrono>
#include <iostream>
#include <fstream>
#include <vector>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"

#include "dataSource/motionDataSource.h"
//#include "dataSource/dataSource.h"

// class default I2C address is 0x68
// specific I2C addresses may be passed as a parameter here
// AD0 low = 0x68 (default for SparkFun breakout and InvenSense evaluation board)
// AD0 high = 0x69

MPU6050 mpu_right(0x68);
MPU6050 mpu_left(0x69); // <-- use for AD0 high

#define TCAADDR0 0x70 //Multiplexer Zero
#define TCAADDR1 0x71 //Multiplexer One

int16_t min_port,max_port;

bool writeToFile = false;

// uncomment "OUTPUT_READABLE_QUATERNION" if you want to see the actual
// quaternion components in a [w, x, y, z] format (not best for parsing
// on a remote host such as Processing or something though)
#define OUTPUT_READABLE_QUATERNION
//#define OUTPUT_READABLE_GYRO
//#define OUTPUT_READABLE_EULER
//#define OUTPUT_READABLE_YAWPITCHROLL
//#define OUTPUT_READABLE_REALACCEL
//#define OUTPUT_READABLE_WORLDACCEL

std::ofstream myfile;

motionDataSource gDataSource(true);

int16_t ax, ay, az;
int16_t gx, gy, gz;

// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

// orientation/motion vars
Quaternion q;           // [w, x, y, z]         quaternion container
VectorInt16 aa;         // [x, y, z]            accel sensor measurements
VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
VectorFloat gravity;    // [x, y, z]            gravity vector
float euler[3];         // [psi, theta, phi]    Euler angle container
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

unsigned long current_time,last_time;
int frame;

//Multiplexer: Pi version
void tcaselect(int address,uint8_t i) 
{  
  if (i > 7) return;

  I2Cdev::writeByte(address, 0, 1 << i);

}


void setup() {
  
    min_port = 7;
    max_port = 7;

    frame = 0;
    current_time = 0;
    last_time = 0;

    printf("\n\nData Source packetSize: %d  port %d \n\n",gDataSource.mPacketSize,gDataSource.mPort);
    
    for (uint8_t t = min_port; t <= max_port; t++)
    { 
      //select multiplexer port
      tcaselect(TCAADDR0,t);
    
      //initialize right side MPU
      mpu_right.initialize();
      printf(mpu_right.testConnection() ? "MPU6050 right connection successful\n" : "MPU6050 right connection failed\n");
      devStatus = mpu_right.dmpInitialize();
    
      if (devStatus == 0) {
        mpu_right.setDMPEnabled(true);
        mpuIntStatus = mpu_right.getIntStatus();//no longer necessary?
        dmpReady = true;
        packetSize = mpu_right.dmpGetFIFOPacketSize();
      } else {
        // ERROR!
        // 1 = initial memory load failed
        // 2 = DMP configuration updates failed
        // (if it's going to break, usually the code will be 1)
        printf("DMP Initialization failed (code %d)\n", devStatus);
      }
      
      //initialize left side MPU
      mpu_left.initialize();
      printf(mpu_left.testConnection() ? "MPU6050 left connection successful\n" : "MPU6050 left connection failed\n");
      devStatus = mpu_left.dmpInitialize();
    
      if (devStatus == 0) {
        mpu_left.setDMPEnabled(true);
        mpuIntStatus = mpu_left.getIntStatus();//no longer necessary?
        dmpReady = true;
        packetSize = mpu_left.dmpGetFIFOPacketSize();
      } else {
        // ERROR!
        // 1 = initial memory load failed
        // 2 = DMP configuration updates failed
        // (if it's going to break, usually the code will be 1)
        printf("DMP Initialization failed (code %d)\n", devStatus);
      }      
    }      
}

uint16_t timeStep()
{
  using namespace std::chrono;

  uint16_t delta_time = 0;
    
  auto now   = high_resolution_clock::now();
  auto now_ms = time_point_cast<milliseconds>(now);
  auto epoch = now_ms.time_since_epoch();    
  auto value = duration_cast<milliseconds>(epoch);
  current_time = value.count();
  if (last_time == 0)
    {
      last_time = current_time;   
    } else {
    delta_time = current_time - last_time;
    last_time = current_time;
  }
  
  return delta_time;
}

uint16_t timeMS()
{
  using namespace std::chrono;

  auto now   = high_resolution_clock::now();
  auto now_ms = time_point_cast<milliseconds>(now);
  auto epoch = now_ms.time_since_epoch();    
  auto value = duration_cast<milliseconds>(epoch);

  return value.count();
}

// ================================================================
// ===                    MAIN PROGRAM LOOP                     ===
// ================================================================
int last_tick_time = 0;
int tick_interval = 50;//MS

void loop() {
    // if programming failed, don't try to do anything
    if (!dmpReady) return;
  
    /////////////////////////

    std::vector<sensor_packet> bodypartData;
    sensor_packet sPacket;
    
    int part_id;
    int temp_part_id;
    int part_count = 0;
    
    if (last_tick_time == 0)
    {
      last_tick_time = timeMS();
      return;
    }

    if (timeMS() - last_tick_time > tick_interval)
    {
      printf("Timestep: %d ",timeMS() - last_tick_time);
      last_tick_time = timeMS();
      for (uint8_t t = min_port; t <= max_port; t++)
	{
	  tcaselect(TCAADDR0,t);
    
	  for (uint8_t addr = 0x68; addr<=0x69; addr++) 
	    {
	      switch (t) 
		{
		case 0: part_id = (addr == 0x68) ? 0 : 4;  break;    // lower spine 0, head 4
		case 1: part_id = (addr == 0x68) ? 5 : 9;  break;    // right clavicle 5, left clavicle 9
		case 2: part_id = (addr == 0x68) ? 15 : 18;  break;  // right foot 15, left foot 18
		case 3: part_id = (addr == 0x68) ? 14 : 17;  break;  // right shin 14, left shin 17
		case 4: part_id = (addr == 0x68) ? 13 : 16;  break;  // right thigh 13, left thigh 16
		case 5: part_id = (addr == 0x68) ? 8 : 12;  break;   // right hand 8, left hand 12
		case 6: part_id = (addr == 0x68) ? 7 : 11;  break;   // right forearm 7, left forearm 11
		case 7: part_id = (addr == 0x68) ? 6 : 10;  break;   // right upper arm 6, left upper arm 10
		default: part_id=0;
		}
	      part_count++;
	      
	      uint8_t data;
	      int32_t gyro[3];
	      if (addr == 0x68)
		{
		  fifoCount = mpu_right.getFIFOCount();
		  if (fifoCount  == 1024)
		    {
		      // reset so we can continue cleanly
		      mpu_right.resetFIFO(); 
		      printf("right FIFO %d cleared!\n",fifoCount);
		      //return;
		    }
		  else if (fifoCount % packetSize != 0)
		    {
		      mpu_right.resetFIFO();
		    }
		  else if (fifoCount > 0)
		    {
		      //printf("right fifo count: %d packages %d \n",fifoCount,fifoCount/42);
		      while (fifoCount >= packetSize)
			{
			  mpu_right.getFIFOBytes(fifoBuffer, packetSize);
			  fifoCount -= packetSize;

			}
#ifdef OUTPUT_READABLE_QUATERNION
		      // display quaternion values in easy matrix form: w x y z
		      mpu_right.dmpGetQuaternion(&q, fifoBuffer);
		      printf(" part %d quat %7.2f %7.2f %7.2f %7.2f ",part_id,q.w,q.x,q.y,q.z);
		      //timeStep(),part_id,q.w,q.x,q.y,q.z);

		      mpu_right.resetFIFO();
	    
		      sPacket.part_id = part_id;
		      sPacket.w = q.w;
		      sPacket.x = q.x;
		      sPacket.y = q.y;
		      sPacket.z = q.z;
		      bodypartData.push_back(sPacket);
	  
		      if (writeToFile)
			myfile << q.w << " " << q.x << " " << q.y << " " << q.z;
	    
#endif
		    }
	  
		  //mpu_right.resetFIFO();
	
		}
	      else if (addr == 0x69)
		{
		  fifoCount = mpu_left.getFIFOCount();
		  if (fifoCount == 1024)
		    {
		      // reset so we can continue cleanly
		      mpu_left.resetFIFO();
		      printf("left FIFO %d cleared\n",fifoCount);
		      //return;
		    }
		  else if (fifoCount % packetSize != 0)
		    {
		      mpu_left.resetFIFO();
		    }
		  else if (fifoCount > 0)
		    {
		      //printf("left fifo count: %d  packages %d \n",fifoCount,fifoCount/42);
		      while (fifoCount >= packetSize)
			{
			  mpu_left.getFIFOBytes(fifoBuffer, packetSize);
			  fifoCount -= packetSize;
			}
	  

#ifdef OUTPUT_READABLE_QUATERNION
		      // display quaternion values in easy matrix form: w x y z
		      mpu_left.dmpGetQuaternion(&q, fifoBuffer);
		      printf(" part %d quat %7.2f %7.2f %7.2f %7.2f ",part_id,q.w,q.x,q.y,q.z);
		      //	   timeStep(),part_id,q.w,q.x,q.y,q.z);

		      sPacket.part_id = part_id;
		      sPacket.w = q.w;
		      sPacket.x = q.x;
		      sPacket.y = q.y;
		      sPacket.z = q.z;
		      bodypartData.push_back(sPacket);
	  
		      if (writeToFile)
			myfile << " " << q.w << " " << q.x << " " << q.y << " " << q.z << "\n";
#endif

		    }
		  //mpu_left.resetFIFO();
		}
	    }
	}
    
      if (gDataSource.mWorkSockfd != INVALID_SOCKET)
	{
	  if (bodypartData.size()>0)
	    {
	      for (int i=0;i<bodypartData.size();i++)
		{
		  sPacket = bodypartData[i];
		  gDataSource.addSensorPacket(sPacket.part_id,sPacket.w,sPacket.x,sPacket.y,sPacket.z);
		}
	    }
	}
      gDataSource.tick();

      printf("partcount %d\n",part_count);
    }
}


 
int main()
{

  if (writeToFile)
    myfile.open ("./output.dat");
    
  setup();
  usleep(10000);
    
  for (;;)
    loop();

  if (writeToFile)
    myfile.close();
  
  return 0;
  
}


/*
        #ifdef OUTPUT_READABLE_EULER
            // display Euler angles in degrees
            mpu.dmpGetQuaternion(&q, fifoBuffer);
            mpu.dmpGetEuler(euler, &q);
            printf("euler %7.2f %7.2f %7.2f    ", euler[0] * 180/M_PI, euler[1] * 180/M_PI, euler[2] * 180/M_PI);
        #endif

        #ifdef OUTPUT_READABLE_YAWPITCHROLL
            // display Euler angles in degrees
            mpu.dmpGetQuaternion(&q, fifoBuffer);
            mpu.dmpGetGravity(&gravity, &q);
            mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
            printf("ypr  %7.2f %7.2f %7.2f    ", ypr[0] * 180/M_PI, ypr[1] * 180/M_PI, ypr[2] * 180/M_PI);
        #endif

        #ifdef OUTPUT_READABLE_REALACCEL
            // display real acceleration, adjusted to remove gravity
            mpu.dmpGetQuaternion(&q, fifoBuffer);
            mpu.dmpGetAccel(&aa, fifoBuffer);
            mpu.dmpGetGravity(&gravity, &q);
            mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
            printf("areal %6d %6d %6d    ", aaReal.x, aaReal.y, aaReal.z);
        #endif

        #ifdef OUTPUT_READABLE_WORLDACCEL
            // display initial world-frame acceleration, adjusted to remove gravity
            // and rotated based on known orientation from quaternion
            mpu.dmpGetQuaternion(&q, fifoBuffer);
            mpu.dmpGetAccel(&aa, fifoBuffer);
            mpu.dmpGetGravity(&gravity, &q);
            mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q);
            printf("aworld %6d %6d %6d    ", aaWorld.x, aaWorld.y, aaWorld.z);
        #endif
    
        #ifdef OUTPUT_TEAPOT
            // display quaternion values in InvenSense Teapot demo format:
            teapotPacket[2] = fifoBuffer[0];
            teapotPacket[3] = fifoBuffer[1];
            teapotPacket[4] = fifoBuffer[4];
            teapotPacket[5] = fifoBuffer[5];
            teapotPacket[6] = fifoBuffer[8];
            teapotPacket[7] = fifoBuffer[9];
            teapotPacket[8] = fifoBuffer[12];
            teapotPacket[9] = fifoBuffer[13];
            Serial.write(teapotPacket, 14);
            teapotPacket[11]++; // packetCount, loops at 0xFF on purpose
        #endif
        printf("\n");
    
    */
