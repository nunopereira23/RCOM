#ifndef APP_LAYER_H
#define APP_LAYER_H

#include <time.h>

//PACKET_SIZE = FRAME_SIZE - 6
// #define PACKET_SIZE FRAME_SIZE-6
unsigned int PACKET_SIZE;
typedef struct{
  int fileFD;
  int serialPortFD;
  char* fileName;
  unsigned int fileSize;
  unsigned char* packet;
  unsigned int packetSize;
} AppLayer;

int sendControlPacket(AppLayer* appLayer, unsigned char control);
int receiveStartPacket(AppLayer* appLayer);
unsigned int receiveFile(AppLayer* appLayer);
unsigned int sendFile(AppLayer* appLayer);

void getFileSize(AppLayer* appLayer);
double getElapsedTimeSecs(struct timespec* start, struct timespec* end);

//PACKET CONTROL
#define DATA_PACKET 1
#define START_PACKET 2
#define END_PACKET 3

//PACKT Type
#define T_SIZE 0
#define T_NAME 1
#endif
